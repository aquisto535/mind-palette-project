"""POST /analyze 엔드포인트.

이미지 파일을 업로드 받아 HFD 분류 추론을 수행한다.
비정상 입력(0바이트, 비이미지, 손상된 파일)은 400 Bad Request로 거부한다.
"""

from fastapi import APIRouter, HTTPException, UploadFile

router = APIRouter()

_ALLOWED_CONTENT_TYPES = {"image/jpeg", "image/png", "image/bmp", "image/webp"}


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

    # 매직 바이트 기반 실제 이미지 형식 검증
    if not _is_valid_image_bytes(content):
        raise HTTPException(
            status_code=400,
            detail="파일 내용이 올바른 이미지 형식이 아닙니다.",
        )


def _is_valid_image_bytes(data: bytes) -> bool:
    """매직 바이트로 실제 이미지 파일인지 확인한다."""
    if len(data) < 4:
        return False

    # JPEG: FF D8 FF
    if data[:3] == bytes([0xFF, 0xD8, 0xFF]):
        # 최소 유효 JPEG는 SOI + 최소 마커 세트 필요 (실제 파싱 시도)
        try:
            from PIL import Image
            import io
            Image.open(io.BytesIO(data)).verify()
            return True
        except Exception:
            return False

    # PNG: 89 50 4E 47
    if data[:4] == bytes([0x89, 0x50, 0x4E, 0x47]):
        try:
            from PIL import Image
            import io
            Image.open(io.BytesIO(data)).verify()
            return True
        except Exception:
            return False

    # BMP: 42 4D
    if data[:2] == bytes([0x42, 0x4D]):
        try:
            from PIL import Image
            import io
            Image.open(io.BytesIO(data)).verify()
            return True
        except Exception:
            return False

    # WebP: 52 49 46 46 ... 57 45 42 50
    if data[:4] == bytes([0x52, 0x49, 0x46, 0x46]) and data[8:12] == bytes([0x57, 0x45, 0x42, 0x50]):
        try:
            from PIL import Image
            import io
            Image.open(io.BytesIO(data)).verify()
            return True
        except Exception:
            return False

    return False


@router.post("/analyze")
async def analyze_image(file: UploadFile):
    """이미지 파일을 분석하여 HFD 분류 결과를 반환한다.

    Args:
        file: 업로드된 이미지 파일 (JPEG, PNG, BMP, WebP)

    Returns:
        분류 결과 JSON

    Raises:
        400: 유효하지 않은 파일
        503: 모델 미로드 상태
    """
    content = await file.read()
    _validate_image_file(file, content)

    # TODO: 실제 추론 로직 (Phase 4 Step 3에서 연결)
    return {"status": "accepted", "message": "분석 요청이 수신되었습니다."}
