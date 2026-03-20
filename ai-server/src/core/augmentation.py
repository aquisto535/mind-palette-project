"""스케치 이미지 전용 데이터 증강 파이프라인.

스케치(아동화) 특성에 맞는 증강만 적용:
- RandomAffine: 스캔 각도/위치 차이 보정
- RandomHorizontalFlip: 왼손잡이 아동의 거울상
- RandomPerspective: 종이 평면 아닌 경우
- fill=255: 스케치 배경은 흰색

사용하지 않는 증강:
- VerticalFlip: 뒤집힌 사람은 다른 그림
- ColorJitter: 스케치는 흑백, 색 변환 무의미
"""

from __future__ import annotations

import random
from typing import TYPE_CHECKING

import torch
from torchvision import transforms

if TYPE_CHECKING:
    from src.config import ModelConfig


class ChannelDropout:
    """학습 강건성을 위한 채널 드롭아웃 증강.

    확률 p로 RGB 3채널 중 하나를 0으로 만든다.
    ToTensor() 이후, Normalize() 이전에 적용해야 한다.

    Args:
        p: 드롭아웃 적용 확률 (기본 0.1)
    """

    def __init__(self, p: float = 0.1) -> None:
        self.p = p

    def __call__(self, tensor: torch.Tensor) -> torch.Tensor:
        if random.random() < self.p:
            ch = random.randint(0, 2)
            tensor = tensor.clone()
            tensor[ch] = 0.0
        return tensor

_DEFAULT_MEAN = (0.972, 0.031, 0.012)
_DEFAULT_STD = (0.156, 0.174, 0.074)


def get_train_transform(
    input_size: int = 260,
    config: ModelConfig | None = None,
) -> transforms.Compose:
    """학습용 증강 파이프라인 반환.

    Args:
        input_size: 모델 입력 크기 (기본 260 = EfficientNet-B2)
        config: 정규화 파라미터 제공용 ModelConfig. None이면 스케치 데이터셋 기본값 사용.

    Returns:
        torchvision transforms.Compose 객체
    """
    mean = config.normalize_mean if config is not None else _DEFAULT_MEAN
    std = config.normalize_std if config is not None else _DEFAULT_STD

    return transforms.Compose([
        transforms.Resize((input_size, input_size)),
        transforms.RandomHorizontalFlip(p=0.5),
        transforms.RandomAffine(
            degrees=15,
            translate=(0.10, 0.10),
            scale=(0.85, 1.15),
            fill=255,  # 흰색 배경
        ),
        transforms.RandomPerspective(
            distortion_scale=0.10,
            p=0.3,
            fill=255,
        ),
        transforms.ToTensor(),
        ChannelDropout(p=0.1),
        transforms.Normalize(mean=mean, std=std),
    ])


def get_val_transform(
    input_size: int = 260,
    config: ModelConfig | None = None,
) -> transforms.Compose:
    """검증/추론용 변환 파이프라인 반환 (증강 없음).

    Args:
        input_size: 모델 입력 크기
        config: 정규화 파라미터 제공용 ModelConfig. None이면 스케치 데이터셋 기본값 사용.

    Returns:
        torchvision transforms.Compose 객체
    """
    mean = config.normalize_mean if config is not None else _DEFAULT_MEAN
    std = config.normalize_std if config is not None else _DEFAULT_STD

    return transforms.Compose([
        transforms.Resize((input_size, input_size)),
        transforms.ToTensor(),
        transforms.Normalize(mean=mean, std=std),
    ])
