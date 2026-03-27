"""TDD: augmentation 모듈 테스트."""

import numpy as np
import pytest
import torch
from PIL import Image
from torchvision import transforms

from src.core.augmentation import ChannelDropout, get_train_transform, get_val_transform
from src.config import ModelConfig


def _make_white_image(size: int = 64) -> Image.Image:
    """테스트용 흰색 RGB 이미지 생성."""
    arr = np.full((size, size, 3), 255, dtype=np.uint8)
    return Image.fromarray(arr)


class TestTrainTransform:
    def test_output_shape(self):
        """출력 텐서 shape: (3, 260, 260)."""
        transform = get_train_transform(input_size=260)
        img = _make_white_image(64)
        out = transform(img)
        assert out.shape == (3, 260, 260)

    def test_output_is_tensor(self):
        transform = get_train_transform()
        img = _make_white_image()
        assert isinstance(transform(img), torch.Tensor)

    def test_no_vertical_flip(self):
        """VerticalFlip이 포함되지 않는다."""
        transform = get_train_transform()
        for t in transform.transforms:
            assert not isinstance(t, transforms.RandomVerticalFlip)

    def test_no_color_jitter(self):
        """ColorJitter가 포함되지 않는다."""
        transform = get_train_transform()
        for t in transform.transforms:
            assert not isinstance(t, transforms.ColorJitter)

    def test_has_horizontal_flip(self):
        """RandomHorizontalFlip이 포함된다."""
        transform = get_train_transform()
        has_flip = any(
            isinstance(t, transforms.RandomHorizontalFlip)
            for t in transform.transforms
        )
        assert has_flip

    def test_custom_input_size(self):
        """다른 input_size 설정 시 해당 크기로 출력."""
        transform = get_train_transform(input_size=128)
        img = _make_white_image(64)
        out = transform(img)
        assert out.shape == (3, 128, 128)


class TestValTransform:
    def test_output_shape(self):
        """검증용 변환 출력 shape: (3, 260, 260)."""
        transform = get_val_transform(input_size=260)
        img = _make_white_image(64)
        out = transform(img)
        assert out.shape == (3, 260, 260)

    def test_no_augmentation(self):
        """동일한 이미지를 두 번 변환하면 동일한 결과 (결정론적)."""
        transform = get_val_transform()
        img = _make_white_image()
        out1 = transform(img)
        out2 = transform(img)
        assert torch.allclose(out1, out2)


class TestNormalizationFromConfig:
    """정규화 파라미터가 config에서 주입되는지 검증 (하드코딩 방지)."""

    def _get_normalize(self, transform: transforms.Compose) -> transforms.Normalize:
        return next(t for t in transform.transforms if isinstance(t, transforms.Normalize))

    def test_train_transform_uses_config_mean(self):
        """get_train_transform에 커스텀 config 전달 시 mean이 반영된다."""
        config = ModelConfig(normalize_mean=(0.5, 0.5, 0.5), normalize_std=(0.5, 0.5, 0.5))
        transform = get_train_transform(config=config)
        normalize = self._get_normalize(transform)
        assert tuple(normalize.mean) == (0.5, 0.5, 0.5)

    def test_train_transform_uses_config_std(self):
        """get_train_transform에 커스텀 config 전달 시 std가 반영된다."""
        config = ModelConfig(normalize_mean=(0.1, 0.2, 0.3), normalize_std=(0.4, 0.5, 0.6))
        transform = get_train_transform(config=config)
        normalize = self._get_normalize(transform)
        assert tuple(normalize.std) == (0.4, 0.5, 0.6)

    def test_val_transform_uses_config_mean(self):
        """get_val_transform에 커스텀 config 전달 시 mean이 반영된다."""
        config = ModelConfig(normalize_mean=(0.5, 0.5, 0.5), normalize_std=(0.5, 0.5, 0.5))
        transform = get_val_transform(config=config)
        normalize = self._get_normalize(transform)
        assert tuple(normalize.mean) == (0.5, 0.5, 0.5)

    def test_val_transform_uses_config_std(self):
        """get_val_transform에 커스텀 config 전달 시 std가 반영된다."""
        config = ModelConfig(normalize_mean=(0.1, 0.2, 0.3), normalize_std=(0.4, 0.5, 0.6))
        transform = get_val_transform(config=config)
        normalize = self._get_normalize(transform)
        assert tuple(normalize.std) == (0.4, 0.5, 0.6)

    def test_train_transform_default_config_uses_sketch_stats(self):
        """config 미전달 시 스케치 데이터셋 통계값이 기본값으로 사용된다."""
        transform = get_train_transform()
        normalize = self._get_normalize(transform)
        assert tuple(normalize.mean) == (0.972, 0.031, 0.012)
        assert tuple(normalize.std) == (0.156, 0.174, 0.074)

    def test_val_transform_default_config_uses_sketch_stats(self):
        """config 미전달 시 스케치 데이터셋 통계값이 기본값으로 사용된다."""
        transform = get_val_transform()
        normalize = self._get_normalize(transform)
        assert tuple(normalize.mean) == (0.972, 0.031, 0.012)
        assert tuple(normalize.std) == (0.156, 0.174, 0.074)


class TestChannelDropout:
    """ChannelDropout 증강 테스트."""

    def _make_tensor(self) -> torch.Tensor:
        """3채널 랜덤 텐서 생성."""
        return torch.rand(3, 64, 64)

    def test_p1_drops_exactly_one_channel(self):
        """p=1.0 이면 정확히 1개 채널이 0이 된다."""
        dropout = ChannelDropout(p=1.0)
        tensor = torch.ones(3, 64, 64)
        result = dropout(tensor)
        zero_channels = sum(1 for c in range(3) if result[c].sum().abs() < 1e-6)
        assert zero_channels == 1

    def test_p0_preserves_tensor(self):
        """p=0.0 이면 원본 텐서가 유지된다."""
        dropout = ChannelDropout(p=0.0)
        tensor = torch.ones(3, 64, 64)
        result = dropout(tensor)
        assert torch.allclose(result, tensor)

    def test_train_transform_contains_channel_dropout(self):
        """get_train_transform 결과에 ChannelDropout이 포함된다."""
        transform = get_train_transform()
        has_dropout = any(isinstance(t, ChannelDropout) for t in transform.transforms)
        assert has_dropout

    def test_output_shape_unchanged(self):
        """ChannelDropout 적용 후 텐서 shape 변화 없음."""
        dropout = ChannelDropout(p=1.0)
        tensor = self._make_tensor()
        result = dropout(tensor)
        assert result.shape == tensor.shape

    def test_non_dropped_channels_unchanged(self):
        """p=1.0 이면 드롭되지 않은 2개 채널은 원본과 동일."""
        dropout = ChannelDropout(p=1.0)
        tensor = torch.ones(3, 64, 64) * 0.5
        result = dropout(tensor)
        non_zero = [c for c in range(3) if result[c].sum() > 0]
        assert len(non_zero) == 2
        for c in non_zero:
            assert torch.allclose(result[c], tensor[c])
