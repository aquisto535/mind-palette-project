from pydantic_settings import BaseSettings
from typing import Tuple


class ModelConfig(BaseSettings):
    """HFD 분류 모델 설정. 하드코딩 방지를 위해 모든 파라미터를 중앙 관리."""

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

    # 정규화 파라미터 (ImageNet 기본값, 추후 스케치 데이터셋 통계로 교체)
    normalize_mean: Tuple[float, float, float] = (0.485, 0.456, 0.406)
    normalize_std: Tuple[float, float, float] = (0.229, 0.224, 0.225)

    # 모델 가중치 경로 (남녀 별도)
    male_model_path: str = "models/mind_palette_male.pt"
    female_model_path: str = "models/mind_palette_female.pt"

    # 디바이스
    device: str = "cpu"


class ServerConfig(BaseSettings):
    """FastAPI 서버 설정."""

    host: str = "0.0.0.0"
    port: int = 8082
