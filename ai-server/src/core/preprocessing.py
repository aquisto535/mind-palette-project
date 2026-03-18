"""이미지 전처리 파이프라인.

C++ 전처리 결과물(512x512 RGB) → 모델 입력(260x260 정규화 텐서) 변환.
정규화 파라미터는 config에서 로드 (하드코딩 방지).
"""

from torchvision import transforms

from src.config import ModelConfig


def create_transform_pipeline(config: ModelConfig) -> transforms.Compose:
    return transforms.Compose(
        [
            transforms.Resize((config.input_size, config.input_size)),
            transforms.ToTensor(),
            transforms.Normalize(mean=config.normalize_mean, std=config.normalize_std),
        ]
    )
