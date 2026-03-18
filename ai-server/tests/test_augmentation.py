"""TDD: augmentation 모듈 테스트."""

import numpy as np
import pytest
import torch
from PIL import Image
from torchvision import transforms

from src.core.augmentation import get_train_transform, get_val_transform


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
