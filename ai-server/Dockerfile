# ─────────────────────────────────────────────────────────────────────────────
# AI Server — Python 3.10-slim (c5.large CPU-only, ONNX Runtime 사용)
# ADR-027: c5.large에서 PyTorch 대신 ONNX Runtime으로 메모리 절감 (~1.2GB → ~500MB)
# ─────────────────────────────────────────────────────────────────────────────
FROM python:3.10-slim

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PIP_NO_CACHE_DIR=1

WORKDIR /app

# 시스템 의존성 (OpenCV headless용)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libglib2.0-0 \
    libgl1-mesa-glx \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# 의존성 설치 — GPU 패키지(onnxruntime-gpu, tensorrt) 제외, CPU용 onnxruntime 사용
# pyproject.toml의 core 의존성만 선택적으로 설치
COPY pyproject.toml ./
RUN pip install --upgrade pip && \
    pip install \
        "fastapi>=0.115.0" \
        "uvicorn[standard]>=0.34.0" \
        "Pillow>=10.0.0" \
        "pydantic>=2.0.0" \
        "pydantic-settings>=2.0.0" \
        "numpy>=1.24.0" \
        "opencv-python-headless>=4.8.0" \
        "tqdm>=4.66.0" \
        "onnx>=1.15.0" \
        "onnxruntime>=1.19.0" \
        "onnxscript>=0.1.0" \
        "python-multipart>=0.0.9" \
        "structlog>=24.0.0" \
        "psutil>=5.0.0"

COPY src ./src
COPY models ./models

EXPOSE 8082

# ADR-027: c5.large에서 ONNX Runtime CPU 추론 사용
ENV INFERENCE_BACKEND=onnx

CMD ["uvicorn", "src.main:app", "--host", "0.0.0.0", "--port", "8082"]
