"""POST /analyze 엔드포인트 비정상 입력 처리 테스트 (Step 1 L3).

L3: 제약과 검증 — "경계에서도 안전한가?"
- 손상된 파일, 0바이트 파일, 비이미지 파일 입력 시 400/422 반환
- 서버 무중단(서버는 여전히 /health 200 OK 응답)
"""

import io
from unittest.mock import patch

import pytest
from httpx import ASGITransport, AsyncClient
from PIL import Image


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
