"""POST /analyze 엔드포인트.

이미지 파일을 업로드 받아 HFD 분류 추론을 수행한다.
비정상 입력(0바이트, 비이미지, 손상된 파일)은 400 Bad Request로 거부한다.
"""

import io
import os
import time
from datetime import datetime
from typing import Annotated, Literal, Tuple

import numpy as np
import structlog
from fastapi import APIRouter, Form, HTTPException, UploadFile, Request, Response
from PIL import Image

from src.config import ModelConfig
from src.core.augmentation import get_val_transform
from src.core.iq_scorer import score_to_result
from src.core.item_mapping import HEAD_A_ITEMS, HEAD_B_ITEMS, HEAD_C_ITEMS, HEAD_D_ITEMS
from src.infra.logger import get_logger

router = APIRouter()
logger = get_logger(__name__)

# ── 타입 별칭 ──────────────────────────────────────────────────────────────────
Gender = Literal["male", "female"]

# ── 상수 ──────────────────────────────────────────────────────────────────────
_ALLOWED_CONTENT_TYPES = {"image/jpeg", "image/png", "image/bmp", "image/webp"}
_THRESHOLD = 0.5
_HEAD_ITEM_MAP = [
    ("head_a", HEAD_A_ITEMS),
    ("head_b", HEAD_B_ITEMS),
    ("head_c", HEAD_C_ITEMS),
    ("head_d", HEAD_D_ITEMS),
]

# ── 모듈 수준 설정 (요청마다 재생성하지 않음: Late Import 제거) ───────────────
_config = ModelConfig()


# ── 이미지 유효성 검사 ────────────────────────────────────────────────────────

_MAGIC_SIGNATURES = [
    (bytes([0xFF, 0xD8, 0xFF]), 3),        # JPEG
    (bytes([0x89, 0x50, 0x4E, 0x47]), 4),  # PNG
    (bytes([0x42, 0x4D]), 2),              # BMP
]
_WEBP_RIFF = bytes([0x52, 0x49, 0x46, 0x46])
_WEBP_MARK = bytes([0x57, 0x45, 0x42, 0x50])


def _is_valid_image_bytes(data: bytes) -> bool:
    """매직 바이트로 실제 이미지 파일인지 확인한다."""
    if len(data) < 12:
        return False

    matched = any(data[:length] == magic for magic, length in _MAGIC_SIGNATURES)
    is_webp = data[:4] == _WEBP_RIFF and data[8:12] == _WEBP_MARK

    if not (matched or is_webp):
        return False

    try:
        Image.open(io.BytesIO(data)).verify()
        return True
    except Exception:
        return False


def _validate_image_file(file: UploadFile, content: bytes) -> None:
    """업로드된 파일의 유효성을 검증한다.

    Raises:
        HTTPException: 유효하지 않은 파일인 경우 400
    """
    if len(content) == 0:
        raise HTTPException(status_code=400, detail="빈 파일은 처리할 수 없습니다.")

    if file.content_type not in _ALLOWED_CONTENT_TYPES:
        raise HTTPException(
            status_code=400,
            detail=f"지원하지 않는 파일 형식입니다: {file.content_type}. "
                   f"허용 형식: {', '.join(sorted(_ALLOWED_CONTENT_TYPES))}",
        )

    if not _is_valid_image_bytes(content):
        raise HTTPException(
            status_code=400,
            detail="파일 내용이 올바른 이미지 형식이 아닙니다.",
        )


# ── 추출된 헬퍼 함수들 (Extract Method 적용) ─────────────────────────────────

def _preprocess_image(content: bytes, config: ModelConfig) -> np.ndarray:
    """이미지 바이트 → ONNX 입력용 numpy 배열 변환.

    Args:
        content: 업로드된 원본 이미지 바이트
        config: 모델 설정 (input_size 참조)

    Returns:
        shape (1, 3, H, W)의 float32 numpy 배열
    """
    pil_image = Image.open(io.BytesIO(content)).convert("RGB")
    transform = get_val_transform(config.input_size)
    img_tensor = transform(pil_image).unsqueeze(0)  # (1, 3, H, W)
    return img_tensor.numpy()


def _run_inference(engine, img_np: np.ndarray) -> Tuple[tuple, float]:
    """추론 엔진을 실행하고 (출력, 소요시간_ms) 를 반환한다.

    Args:
        engine: OnnxInferenceEngine 또는 TensorRtNativeEngine
        img_np: (1, 3, H, W) 형태의 numpy 배열

    Returns:
        (outputs_tuple, duration_ms): ONNX head 출력 tuple과 추론 소요 시간(ms)
    """
    t0 = time.perf_counter()
    outputs = engine.run(img_np)  # (head_a, head_b, head_c, head_d)
    duration_ms = (time.perf_counter() - t0) * 1000
    return outputs, duration_ms


def _logits_to_item_results(outputs: tuple) -> dict:
    """ONNX 출력 로짓(tuple) → 60문항 결과 dict.

    Args:
        outputs: (head_a, head_b, head_c, head_d) logit 배열 tuple

    Returns:
        {str(항목번호): 0|1} 형태의 60문항 결과
    """
    items = {}
    for (head_name, item_list), logit_arr in zip(_HEAD_ITEM_MAP, outputs):
        probs = 1 / (1 + np.exp(-logit_arr.flatten()))  # sigmoid
        for item_no, prob in zip(item_list, probs):
            items[str(item_no)] = int(prob >= _THRESHOLD)
    return items


def _compute_head_scores(items: dict) -> dict:
    """60문항 결과 → 4개 Head별 점수 합계.

    Args:
        items: {str(항목번호): 0|1} 형태의 60문항 결과

    Returns:
        {head_name: 합계_점수} dict
    """
    return {
        head_name: sum(items[str(n)] for n in item_list)
        for head_name, item_list in _HEAD_ITEM_MAP
    }


def _build_analysis_result(
    outputs: tuple,
    age: int,
    child_gender: Gender,
    figure_gender: Gender,
) -> dict:
    """추론 출력 → 최종 분석 결과 JSON 조립.

    Args:
        outputs: ONNX head 출력 tuple
        age: 아동 만 나이
        child_gender: 아동 성별
        figure_gender: 그림 속 인물 성별

    Returns:
        API 응답에 담길 완전한 분석 결과 dict
    """
    items = _logits_to_item_results(outputs)
    head_scores = _compute_head_scores(items)
    raw_score = sum(items.values())

    try:
        iq_result = score_to_result(raw_score, age, child_gender, figure_gender)
    except ValueError:
        iq_result = {"iq": None, "percentile": None, "raw_score": raw_score}

    return {
        "items": items,
        "head_scores": head_scores,
        "raw_score": raw_score,
        "iq": iq_result["iq"],
        "percentile": iq_result["percentile"],
        "child_info": {
            "age": age,
            "child_gender": child_gender,
            "figure_gender": figure_gender,
        },
        "date": datetime.now().strftime("%Y-%m-%d"),
    }


# ── 라우터 ────────────────────────────────────────────────────────────────────

@router.post(
    "/analyze",
    responses={
        400: {"description": "Bad Request - 유효하지 않은 이미지 파일 (0바이트, 비이미지 등)"},
        500: {"description": "Internal Server Error - 서버 내부 오류"},
        503: {"description": "Service Unavailable - 모델 미로드 또는 GPU 리소스(OOM) 부족"}
    }
)
async def analyze_image(
    request: Request,
    response: Response,
    file: UploadFile,
    age: Annotated[int, Form()] = 10,
    child_gender: Annotated[Gender, Form()] = "male",
    figure_gender: Annotated[Gender, Form()] = "male",
):
    """이미지 파일을 분석하여 HFD 분류 결과를 반환한다.

    Args:
        file: 업로드된 이미지 파일 (JPEG, PNG, BMP, WebP)
        age: 아동 만 나이 (기본 10)
        child_gender: 아동 성별 "male" | "female" (기본 "male")
        figure_gender: 그림 성별 "male" | "female" (기본 "male")

    Returns:
        분류 결과 JSON (60문항 + IQ + 백분위)

    Raises:
        400: 유효하지 않은 파일
        500: 서버 내부 오류
        503: 모델 미로드 상태 또는 GPU OOM
    """
    # 1. 이미지 유효성 검사
    content = await file.read()
    _validate_image_file(file, content)

    logger.info("Image received for analysis",
                filename=file.filename,
                content_type=file.content_type,
                size=len(content))

    try:
        # 2. 모델 상태 확인
        model_state = getattr(request.app.state, "model_state", None)
        if not model_state or getattr(model_state, "engine_type", "none") == "none":
            raise HTTPException(status_code=503, detail="모델이 로드되지 않았습니다.")

        engine = model_state.male_engine if figure_gender == "male" else model_state.female_engine
        if engine is None:
            raise HTTPException(status_code=503, detail=f"해당 성별({figure_gender}) 모델이 로드되지 않았습니다.")

        # 3. 이미지 전처리 (책임: _preprocess_image)
        img_np = _preprocess_image(content, _config)

        # 4. 추론 실행 (책임: _run_inference)
        outputs, duration_ms = _run_inference(engine, img_np)

        # 5. Server-Timing 헤더 (관리자 전용 프로파일링)
        admin_key = os.getenv("ADMIN_PROFILE_KEY")
        client_key = request.headers.get("x-admin-profile-key")
        if admin_key and client_key == admin_key:
            response.headers["Server-Timing"] = f"ai_inference;dur={duration_ms:.1f}"

    except HTTPException as e:
        raise e
    except RuntimeError as e:
        if "out of memory" in str(e).lower():
            logger.error("GPU Out of Memory during inference", error=str(e))
            raise HTTPException(
                status_code=503,
                detail="서버 리소스(GPU) 부족으로 요청을 처리할 수 없습니다. 잠시 후 다시 시도해주세요."
            )
        raise e
    except Exception as e:
        logger.error("Unexpected error during inference", error=str(e), exc_info=True)
        raise HTTPException(status_code=500, detail=f"서버 내부 오류: {str(e)}")

    # 6. 결과 조립 (책임: _build_analysis_result)
    analysis_result = _build_analysis_result(outputs, age, child_gender, figure_gender)

    logger.info("Inference completed successfully",
                request_id=structlog.contextvars.get_contextvars().get("request_id"),
                result=analysis_result)

    return analysis_result
