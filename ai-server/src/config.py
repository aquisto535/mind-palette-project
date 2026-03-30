from pydantic_settings import BaseSettings
from typing import Tuple


class ModelConfig(BaseSettings):
    """HFD 분류 모델 설정. 하드코딩 방지를 위해 모든 파라미터를 중앙 관리."""

    ##EfficientNet-B2라는 엔진을 가져다가 'HFD 인물화 지능 검사'라는 전용 도구로 개조하기 위한 정밀 설계도"

    # 입력 차원 (ADR-018: EfficientNet-B2 원논문 = 260x260)
    input_size: int = 260
    input_channels: int = 3

    # EfficientNet-B2 feature dimension
    backbone_feature_dim: int = 1408

    # Head 출력 크기 (ADR-018: PDF 원본 60문항 기준)
    head_a_size: int = 19  # 머리/얼굴
    head_b_size: int = 14  # 몸통/연결/비례
    head_c_size: int = 16  # 사지/말단
    head_d_size: int = 11  # 의복/질적

    # 정규화 파라미터 (스케치 데이터셋 통계 기반 최적화)
    # R(Gray): ~0.97, G(Binary): ~0.03, B(Dist): ~0.01
    normalize_mean: Tuple[float, float, float] = (0.972, 0.031, 0.012)
    normalize_std: Tuple[float, float, float] = (0.156, 0.174, 0.074)

    # 모델 가중치 경로 (남녀 별도)
    male_model_path: str = "models/mind_palette_male.pt"
    female_model_path: str = "models/mind_palette_female.pt"

    # 디바이스
    device: str = "cpu"

    # ONNX 모델 경로 (남녀 별도)
    male_onnx_path: str = "models/mind_palette_male.onnx"
    female_onnx_path: str = "models/mind_palette_female.onnx"

    # 추론 백엔드 ("pytorch" | "onnx" | "tensorrt_native" | "tensorrt_ort")
    inference_backend: str = "pytorch"

    # ONNX 변환 설정
    onnx_opset_version: int = 17

    # TensorRT 설정 (RTX 3050 Laptop, CUDA 12.6 기준)
    male_trt_engine_path: str = "models/mind_palette_male.engine"
    female_trt_engine_path: str = "models/mind_palette_female.engine"
    trt_fp16_enable: bool = True
    trt_engine_cache_dir: str = "models/trt_cache"
    trt_workspace_gb: int = 2  # 4GB VRAM에서 2GB 빌드 워크스페이스


class ServerConfig(BaseSettings):
    """FastAPI 서버 설정."""

    host: str = "0.0.0.0"
    port: int = 8082


class TrainingConfig(BaseSettings):
    """학습 파이프라인 설정."""

    # 데이터 경로
    data_dir: str = "data"
    annotations_path: str = "data/labels/annotations.json"
    synthetic_dir: str = "data/synthetic_sketches"

    # Phase A: 합성 데이터 사전학습
    phase_a_epochs: int = 50
    phase_a_lr: float = 1e-3
    phase_a_batch_size: int = 4
    phase_a_synthetic_count: int = 500

    # Phase B: 실제 데이터 미세조정
    phase_b_epochs: int = 100
    phase_b_lr: float = 1e-4
    phase_b_batch_size: int = 4
    phase_b_early_stop_patience: int = 15

    # 공통
    weight_decay: float = 1e-4
    checkpoint_dir: str = "models/checkpoints"
    output_dir: str = "models"
    device: str = "cpu"
