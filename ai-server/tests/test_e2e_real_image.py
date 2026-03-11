"""실제 이미지를 사용한 E2E 통합 테스트."""

import pytest
from pathlib import Path
from PIL import Image
import torch

from src.config import ModelConfig
from src.core.model import HFDClassifier
from src.core.preprocessing import create_transform_pipeline


@pytest.fixture(scope="session")
def config():
    return ModelConfig()


@pytest.fixture(scope="session")
def model(config):
    return HFDClassifier(config)


@pytest.fixture(scope="session")
def test_image_path():
    """프로젝트 루트의 테스트 이미지."""
    path = Path(__file__).parent.parent.parent / "남자사람_8_남_06463.jpg"
    assert path.exists(), f"Test image not found: {path}"
    return path


def test_real_image_exists(test_image_path):
    """테스트 이미지가 존재해야 한다."""
    assert test_image_path.exists()
    assert test_image_path.suffix.lower() in [".jpg", ".jpeg", ".png"]


def test_real_image_load(test_image_path):
    """테스트 이미지를 PIL로 로드할 수 있어야 한다."""
    img = Image.open(test_image_path)
    assert img.size[0] > 0
    assert img.size[1] > 0


def test_real_image_preprocessing(test_image_path, config):
    """전처리 파이프라인이 실제 이미지를 처리할 수 있어야 한다."""
    img = Image.open(test_image_path)
    pipeline = create_transform_pipeline(config)

    tensor = pipeline(img)
    assert tensor.shape == (3, config.input_size, config.input_size)
    assert tensor.dtype == torch.float32
    assert tensor.min() >= -3.0
    assert tensor.max() <= 3.0


def test_real_image_inference(test_image_path, config, model):
    """실제 이미지에 대한 모델 추론."""
    img = Image.open(test_image_path)
    pipeline = create_transform_pipeline(config)

    tensor = pipeline(img).unsqueeze(0)  # (1, 3, 260, 260)
    model.eval()
    with torch.no_grad():
        logits_a, logits_b, logits_c, logits_d = model(tensor)

    # Shape 검증
    assert logits_a.shape == (1, 19)
    assert logits_b.shape == (1, 14)
    assert logits_c.shape == (1, 16)
    assert logits_d.shape == (1, 11)

    # Sigmoid 적용 후 범위 검증
    probs_a = torch.sigmoid(logits_a)
    probs_b = torch.sigmoid(logits_b)
    probs_c = torch.sigmoid(logits_c)
    probs_d = torch.sigmoid(logits_d)

    assert (probs_a >= 0).all() and (probs_a <= 1).all()
    assert (probs_b >= 0).all() and (probs_b <= 1).all()
    assert (probs_c >= 0).all() and (probs_c <= 1).all()
    assert (probs_d >= 0).all() and (probs_d <= 1).all()


def test_real_image_multiple_inference(test_image_path, config, model):
    """동일 이미지 여러 번 추론 시 동일 결과 (결정론적)."""
    img = Image.open(test_image_path)
    pipeline = create_transform_pipeline(config)
    tensor = pipeline(img).unsqueeze(0)

    model.eval()
    with torch.no_grad():
        result1 = model(tensor)
        result2 = model(tensor)

    for t1, t2 in zip(result1, result2):
        assert torch.allclose(t1, t2), "Inference 결과가 결정론적이지 않음"


def test_real_image_batch_inference(test_image_path, config, model):
    """배치 크기 > 1로 동일 이미지 여러 장 추론."""
    img = Image.open(test_image_path)
    pipeline = create_transform_pipeline(config)
    tensor_single = pipeline(img).unsqueeze(0)

    # 배치 크기 4로 동일 이미지 4장
    tensor_batch = tensor_single.repeat(4, 1, 1, 1)

    model.eval()
    with torch.no_grad():
        logits_a, logits_b, logits_c, logits_d = model(tensor_batch)

    assert logits_a.shape == (4, 19)
    assert logits_b.shape == (4, 14)
    assert logits_c.shape == (4, 16)
    assert logits_d.shape == (4, 11)

    # Sigmoid 적용 후 범위 검증
    probs_a = torch.sigmoid(logits_a)
    probs_b = torch.sigmoid(logits_b)
    probs_c = torch.sigmoid(logits_c)
    probs_d = torch.sigmoid(logits_d)

    assert (probs_a >= 0).all() and (probs_a <= 1).all()
    assert (probs_b >= 0).all() and (probs_b <= 1).all()
    assert (probs_c >= 0).all() and (probs_c <= 1).all()
    assert (probs_d >= 0).all() and (probs_d <= 1).all()
