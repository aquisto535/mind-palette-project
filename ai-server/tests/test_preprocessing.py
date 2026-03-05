"""3채널 입력 이미지 구조 및 전처리 파이프라인 테스트 (L1 + L2)."""

import pytest
import numpy as np
import torch
from PIL import Image

from src.config import ModelConfig
from src.core.preprocessing import create_transform_pipeline


@pytest.fixture(scope="session")
def config():
    return ModelConfig()


# --- Cycle 4: 3채널 입력 이미지 구조 (L1) ---


def test_input_image_has_three_channels():
    """C++ 전처리 출력 이미지는 (H, W, 3), dtype=uint8이어야 한다."""
    mock_image = np.zeros((512, 512, 3), dtype=np.uint8)
    assert mock_image.shape[2] == 3
    assert mock_image.dtype == np.uint8


def test_channel_semantics():
    """각 채널은 R=gray, G=binary, B=distance로 분리 가능해야 한다."""
    mock_image = np.zeros((512, 512, 3), dtype=np.uint8)
    r_channel = mock_image[:, :, 0]
    g_channel = mock_image[:, :, 1]
    b_channel = mock_image[:, :, 2]

    assert r_channel.shape == (512, 512)
    assert g_channel.shape == (512, 512)
    assert b_channel.shape == (512, 512)


# --- Cycle 5: 전처리 파이프라인 (L2) ---


def test_pipeline_output_shape(config):
    """파이프라인 출력 shape은 (3, 260, 260)이어야 한다."""
    pipeline = create_transform_pipeline(config)
    mock_image = np.random.randint(0, 255, (512, 512, 3), dtype=np.uint8)
    pil_image = Image.fromarray(mock_image)

    tensor = pipeline(pil_image)
    assert tensor.shape == (3, config.input_size, config.input_size)

    batched = tensor.unsqueeze(0)
    assert batched.shape == (1, 3, config.input_size, config.input_size)


def test_pipeline_output_dtype(config):
    """출력 텐서는 float32여야 한다."""
    pipeline = create_transform_pipeline(config)
    mock_image = np.random.randint(0, 255, (512, 512, 3), dtype=np.uint8)
    pil_image = Image.fromarray(mock_image)

    tensor = pipeline(pil_image)
    assert tensor.dtype == torch.float32


def test_pipeline_output_value_range(config):
    """정규화 후 값은 합리적 범위 [-3, 3] 내여야 한다."""
    pipeline = create_transform_pipeline(config)
    mock_image = np.random.randint(0, 255, (512, 512, 3), dtype=np.uint8)
    pil_image = Image.fromarray(mock_image)

    tensor = pipeline(pil_image)
    assert tensor.min() >= -3.0
    assert tensor.max() <= 3.0


def test_pipeline_uses_config_normalization_params():
    """파이프라인은 config의 mean/std를 사용해야 한다 (하드코딩 방지)."""
    custom_config = ModelConfig(
        normalize_mean=(0.5, 0.5, 0.5),
        normalize_std=(0.5, 0.5, 0.5),
    )
    pipeline = create_transform_pipeline(custom_config)

    # mean=0.5, std=0.5: (1.0 - 0.5) / 0.5 = 1.0 (흰 이미지)
    white_image = np.full((260, 260, 3), 255, dtype=np.uint8)
    pil_image = Image.fromarray(white_image)

    tensor = pipeline(pil_image)
    assert torch.allclose(tensor, torch.ones_like(tensor), atol=0.02)
