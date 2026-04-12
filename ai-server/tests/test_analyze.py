"""POST /analyze 엔드포인트 비정상 입력 처리 테스트 (Step 1 L3).

L3: 제약과 검증 — "경계에서도 안전한가?"
- 손상된 파일, 0바이트 파일, 비이미지 파일 입력 시 400/422 반환
- 서버 무중단(서버는 여전히 /health 200 OK 응답)
- ADR-033 Tier 2: Confidence Score 기반 비연필 이미지 422 반환
"""

import io
import unittest.mock
from unittest.mock import MagicMock, patch

import numpy as np
import pytest
from httpx import ASGITransport, AsyncClient
from PIL import Image

from src.routes.analyze import _compute_mean_confidence


# ──────────────────────────────────────────────
# L3: 비정상 입력 처리
# ──────────────────────────────────────────────


class TestAnalyzeInvalidInput:
    """비정상 입력에 대해 서버가 안전하게 거부해야 한다."""

    @pytest.mark.asyncio
    async def test_no_file_returns_422(self, app):
        """파일 없이 POST /analyze 요청 시 422 Unprocessable Entity를 반환해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post("/analyze")
        assert response.status_code == 422

    @pytest.mark.asyncio
    async def test_empty_file_returns_400(self, app):
        """0바이트 파일 업로드 시 400 Bad Request를 반환해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("empty.jpg", b"", "image/jpeg")},
            )
        assert response.status_code == 400

    @pytest.mark.asyncio
    async def test_non_image_file_returns_400(self, app):
        """이미지가 아닌 파일(.txt) 업로드 시 400 Bad Request를 반환해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("test.txt", b"hello world", "text/plain")},
            )
        assert response.status_code == 400

    @pytest.mark.asyncio
    async def test_corrupted_image_returns_400(self, app):
        """손상된 이미지 파일(헤더만 있고 내용 없음) 업로드 시 400 Bad Request를 반환해야 한다."""
        # JPEG 매직 바이트만 있고 실제 이미지 데이터 없음
        corrupted_jpeg = bytes([0xFF, 0xD8, 0xFF, 0xE0]) + b"\x00" * 10
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("corrupted.jpg", corrupted_jpeg, "image/jpeg")},
            )
        assert response.status_code == 400

    @pytest.mark.asyncio
    async def test_fake_image_extension_returns_400(self, app):
        """텍스트 파일을 .jpg로 위장하여 업로드 시 400 Bad Request를 반환해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("fake.jpg", b"I am not an image", "image/jpeg")},
            )
        assert response.status_code == 400


class TestAnalyzeServerStability:
    """비정상 입력 후에도 서버가 정상 동작해야 한다."""

    @pytest.mark.asyncio
    async def test_server_remains_healthy_after_bad_input(self, app):
        """손상된 파일 업로드 후에도 /health는 200 OK를 반환해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            # 비정상 요청 먼저
            await client.post(
                "/analyze",
                files={"file": ("bad.jpg", b"not an image", "image/jpeg")},
            )
            # 서버 상태 확인
            health_response = await client.get("/health")

        assert health_response.status_code == 200
        assert health_response.json()["status"] == "ok"

    @pytest.mark.asyncio
    async def test_multiple_bad_requests_dont_crash_server(self, app):
        """여러 번의 비정상 요청 후에도 서버가 정상 응답해야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            # 5번 연속 비정상 요청
            for _ in range(5):
                await client.post(
                    "/analyze",
                    files={"file": ("bad.jpg", b"garbage", "image/jpeg")},
                )
            # 여전히 /health 정상 응답
            response = await client.get("/health")
        assert response.status_code == 200

    @pytest.mark.asyncio
    async def test_error_response_has_detail_field(self, app):
        """400 에러 응답에 'detail' 필드가 포함되어야 한다."""
        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("bad.jpg", b"garbage", "image/jpeg")},
            )
        assert response.status_code == 400
        data = response.json()
        assert "detail" in data


# ──────────────────────────────────────────────
# L3: 제약과 검증 (Extreme Cases)
# ──────────────────────────────────────────────

@pytest.mark.asyncio
async def test_analyze_gpu_oom_returns_503(app):
    """GPU OOM 발생 시 503 Service Unavailable을 반환해야 한다 (L3)."""
    img = Image.new('RGB', (260, 260), color='red')
    buf = io.BytesIO()
    img.save(buf, format='JPEG')

    import unittest.mock
    
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        engine = app.state.model_state.male_engine
        if engine is None:
            engine = unittest.mock.MagicMock()
            app.state.model_state.male_engine = engine
            app.state.model_state.engine_type = "mock"
            
        original_run = engine.run
        engine.run = unittest.mock.MagicMock(side_effect=RuntimeError("CUDA out of memory"))
        try:
            response = await client.post(
                "/analyze",
                files={"file": ("test.jpg", buf.getvalue(), "image/jpeg")}
            )
        finally:
            engine.run = original_run

    assert response.status_code == 503
    assert "리소스" in response.json()["detail"] or "GPU" in response.json()["detail"]


# ──────────────────────────────────────────────
# ADR-033 Tier 2: Confidence Score 기반 비연필 감지
# ──────────────────────────────────────────────

class TestComputeMeanConfidence:
    """_compute_mean_confidence 헬퍼 함수 단위 테스트."""

    # 실제 head별 항목 수: A=19, B=14, C=16, D=11 (총 60항목)
    _SHAPES = ((1, 19), (1, 14), (1, 16), (1, 11))

    def test_all_uncertain_outputs_return_zero_confidence(self):
        """logit=0 (sigmoid=0.5) 이면 confidence=0.0 이어야 한다."""
        outputs = tuple(np.zeros(shape, dtype=np.float32) for shape in self._SHAPES)
        result = _compute_mean_confidence(outputs)
        assert result == pytest.approx(0.0, abs=1e-6)

    def test_all_certain_outputs_return_high_confidence(self):
        """logit=10 (sigmoid≈1.0) 이면 confidence≈1.0 이어야 한다."""
        outputs = tuple(np.full(shape, 10.0, dtype=np.float32) for shape in self._SHAPES)
        result = _compute_mean_confidence(outputs)
        assert result >= 0.9

    def test_mixed_outputs_return_intermediate_confidence(self):
        """절반은 확실(logit=10), 절반은 불확실(logit=0) 이면 confidence가 중간값이어야 한다."""
        def mixed(shape):
            half = shape[1] // 2
            certain = np.full((1, half), 10.0, dtype=np.float32)
            uncertain = np.zeros((1, shape[1] - half), dtype=np.float32)
            return np.concatenate([certain, uncertain], axis=1)

        outputs = tuple(mixed(shape) for shape in self._SHAPES)
        result = _compute_mean_confidence(outputs)
        assert 0.3 < result < 0.7


class TestAnalyzeLowConfidence:
    """Confidence Score가 낮을 때 422를 반환해야 한다 (ADR-033 Tier 2)."""

    def _make_jpeg_bytes(self) -> bytes:
        img = Image.new("RGB", (260, 260), color="white")
        buf = io.BytesIO()
        img.save(buf, format="JPEG")
        return buf.getvalue()

    # 실제 head별 항목 수: A=19, B=14, C=16, D=11 (총 60항목)
    _SHAPES = ((1, 19), (1, 14), (1, 16), (1, 11))

    def _make_low_confidence_engine(self):
        """logit=0 반환 → confidence=0 → 422 트리거."""
        engine = MagicMock()
        engine.run.return_value = tuple(
            np.zeros(shape, dtype=np.float32) for shape in self._SHAPES
        )
        return engine

    def _make_high_confidence_engine(self):
        """logit=10 반환 → confidence≈1.0 → 200 OK."""
        engine = MagicMock()
        engine.run.return_value = tuple(
            np.full(shape, 10.0, dtype=np.float32) for shape in self._SHAPES
        )
        return engine

    @pytest.mark.asyncio
    async def test_analyze_returns_422_on_low_confidence(self, app):
        """모든 logit=0 (confidence=0) 인 엔진 주입 시 422를 반환해야 한다."""
        engine = self._make_low_confidence_engine()
        app.state.model_state.male_engine = engine
        app.state.model_state.engine_type = "mock"

        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("test.jpg", self._make_jpeg_bytes(), "image/jpeg")},
            )

        assert response.status_code == 422
        assert "판독 불가" in response.json()["detail"]

    @pytest.mark.asyncio
    async def test_analyze_returns_200_on_high_confidence(self, app):
        """logit=10 (confidence≈1.0) 인 엔진 주입 시 200을 반환해야 한다."""
        engine = self._make_high_confidence_engine()
        app.state.model_state.male_engine = engine
        app.state.model_state.engine_type = "mock"

        transport = ASGITransport(app=app)
        async with AsyncClient(transport=transport, base_url="http://test") as client:
            response = await client.post(
                "/analyze",
                files={"file": ("test.jpg", self._make_jpeg_bytes(), "image/jpeg")},
            )

        assert response.status_code == 200
