"""모델 아키텍처 구조 및 출력 Shape 테스트 (L1)."""

import pytest
import torch

from src.core.model import HFDClassifier
from src.config import ModelConfig


@pytest.fixture(scope="session")
def config():
    return ModelConfig()


@pytest.fixture(scope="session")
def model(config):
    return HFDClassifier(config)


# --- Cycle 2: 아키텍처 구조 ---


def test_model_has_backbone(model):
    """Backbone(EfficientNet-B2 features) 속성이 존재해야 한다."""
    assert hasattr(model, "backbone")
    assert len(list(model.backbone.children())) > 0


def test_model_has_four_heads(model):
    """4개의 nn.Linear 분류 헤드가 존재해야 한다."""
    heads = [model.heads['head_a'], model.heads['head_b'], model.heads['head_c'], model.heads['head_d']]
    assert len(heads) == 4
    for head in heads:
        assert isinstance(head, torch.nn.Linear)


def test_head_output_dimensions(model, config):
    """각 Head의 out_features가 19/14/16/11이어야 한다."""
    assert model.heads['head_a'].out_features == config.head_a_size  # 19
    assert model.heads['head_b'].out_features == config.head_b_size  # 14
    assert model.heads['head_c'].out_features == config.head_c_size  # 16
    assert model.heads['head_d'].out_features == config.head_d_size  # 11


def test_total_output_items(model):
    """전체 출력 항목 합계는 60이어야 한다."""
    total = (
        model.heads['head_a'].out_features
        + model.heads['head_b'].out_features
        + model.heads['head_c'].out_features
        + model.heads['head_d'].out_features
    )
    assert total == 60


def test_backbone_input_features(model, config):
    """모든 Head의 in_features가 backbone feature dim(1408)과 일치해야 한다."""
    assert model.heads['head_a'].in_features == config.backbone_feature_dim
    assert model.heads['head_b'].in_features == config.backbone_feature_dim
    assert model.heads['head_c'].in_features == config.backbone_feature_dim
    assert model.heads['head_d'].in_features == config.backbone_feature_dim


# --- Cycle 3: 출력 텐서 Shape ---


def test_output_shapes_with_dummy_input(model, config):
    """더미 입력 (1,3,260,260) → Head A(1,19), B(1,14), C(1,16), D(1,11), float32."""
    dummy = torch.randn(1, 3, config.input_size, config.input_size)
    model.eval()
    with torch.no_grad():
        out_a, out_b, out_c, out_d = model(dummy)

    assert out_a.shape == (1, 19)
    assert out_b.shape == (1, 14)
    assert out_c.shape == (1, 16)
    assert out_d.shape == (1, 11)

    assert out_a.dtype == torch.float32
    assert out_b.dtype == torch.float32
    assert out_c.dtype == torch.float32
    assert out_d.dtype == torch.float32


def test_output_is_tuple(model, config):
    """forward()는 ONNX 호환을 위해 Tuple을 반환해야 한다."""
    dummy = torch.randn(1, 3, config.input_size, config.input_size)
    model.eval()
    with torch.no_grad():
        result = model(dummy)
    assert isinstance(result, tuple)
    assert len(result) == 4


def test_batch_output_shapes(model, config):
    """batch size > 1도 지원해야 한다."""
    batch_size = 4
    dummy = torch.randn(batch_size, 3, config.input_size, config.input_size)
    model.eval()
    with torch.no_grad():
        out_a, out_b, out_c, out_d = model(dummy)

    assert out_a.shape == (batch_size, 19)
    assert out_b.shape == (batch_size, 14)
    assert out_c.shape == (batch_size, 16)
    assert out_d.shape == (batch_size, 11)
