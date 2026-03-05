"""Backbone 동결 및 Multi-head 추론 검증 테스트 (L2)."""

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


# --- Cycle 6: Feature Extractor 동결 검증 ---


def test_backbone_parameters_frozen(model):
    """Backbone의 모든 파라미터는 requires_grad == False여야 한다."""
    for name, param in model.backbone.named_parameters():
        assert param.requires_grad is False, f"Backbone param '{name}' is not frozen"


def test_head_parameters_trainable(model):
    """모든 Head 파라미터는 requires_grad == True (학습 가능)여야 한다."""
    heads = [model.head_a, model.head_b, model.head_c, model.head_d]
    for head in heads:
        for name, param in head.named_parameters():
            assert param.requires_grad is True, f"Head param '{name}' is frozen"


def test_backbone_param_count_vs_head(model):
    """Backbone 파라미터 수는 Head보다 훨씬 많아야 한다."""
    backbone_params = sum(p.numel() for p in model.backbone.parameters())
    head_params = sum(
        p.numel()
        for head in [model.head_a, model.head_b, model.head_c, model.head_d]
        for p in head.parameters()
    )
    assert backbone_params > head_params * 10


# --- Cycle 7: Multi-head sigmoid 출력 범위 ---


def test_sigmoid_outputs_in_valid_range(model, config):
    """각 Head의 sigmoid 출력이 [0, 1] 범위 내여야 한다."""
    torch.manual_seed(42)
    dummy = torch.randn(1, 3, config.input_size, config.input_size)

    model.eval()
    with torch.no_grad():
        logits_a, logits_b, logits_c, logits_d = model(dummy)

    for logits, name in [
        (logits_a, "head_a"),
        (logits_b, "head_b"),
        (logits_c, "head_c"),
        (logits_d, "head_d"),
    ]:
        probs = torch.sigmoid(logits)
        assert probs.min() >= 0.0, f"{name} has value < 0"
        assert probs.max() <= 1.0, f"{name} has value > 1"


def test_deterministic_output_with_fixed_seed(model, config):
    """동일한 seed로 동일한 출력이 나와야 한다 (재현성)."""
    model.eval()

    torch.manual_seed(42)
    dummy1 = torch.randn(1, 3, config.input_size, config.input_size)
    with torch.no_grad():
        out1 = model(dummy1)

    torch.manual_seed(42)
    dummy2 = torch.randn(1, 3, config.input_size, config.input_size)
    with torch.no_grad():
        out2 = model(dummy2)

    for t1, t2 in zip(out1, out2):
        assert torch.allclose(t1, t2), "Outputs differ with same seed"


def test_forward_returns_logits_not_probabilities(model, config):
    """forward()는 raw logits을 반환해야 한다 (sigmoid 미적용, 음수 또는 >1 존재)."""
    torch.manual_seed(0)
    dummy = torch.randn(1, 3, config.input_size, config.input_size)

    model.eval()
    with torch.no_grad():
        logits_a, logits_b, logits_c, logits_d = model(dummy)

    all_logits = torch.cat([logits_a, logits_b, logits_c, logits_d], dim=1)
    has_negative = (all_logits < 0).any()
    has_above_one = (all_logits > 1).any()
    assert has_negative or has_above_one, "Outputs look like probabilities, not logits"
