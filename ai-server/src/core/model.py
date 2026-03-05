"""HFD (Human Figure Drawing) Multi-label Binary Classifier.

ADR-018: EfficientNet-B2 backbone (frozen) + 4 Linear heads (19/14/16/11).
forward() returns raw logits as Tuple for ONNX compatibility.
Sigmoid is applied only at inference time, NOT inside forward().
"""

from typing import Tuple

import torch
import torch.nn as nn
from torchvision.models import efficientnet_b2, EfficientNet_B2_Weights

from src.config import ModelConfig


class HFDClassifier(nn.Module):
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()

        backbone_full = efficientnet_b2(weights=EfficientNet_B2_Weights.DEFAULT)
        self.backbone = backbone_full.features
        self.avgpool = backbone_full.avgpool

        # Backbone 동결 (Transfer Learning)
        for param in self.backbone.parameters():
            param.requires_grad = False

        feature_dim = config.backbone_feature_dim  # 1408
        self.head_a = nn.Linear(feature_dim, config.head_a_size)  # 19: 머리/얼굴
        self.head_b = nn.Linear(feature_dim, config.head_b_size)  # 14: 몸통/비례
        self.head_c = nn.Linear(feature_dim, config.head_c_size)  # 16: 사지/말단
        self.head_d = nn.Linear(feature_dim, config.head_d_size)  # 11: 의복/질적

    def forward(
        self, x: torch.Tensor
    ) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        features = self.backbone(x)
        features = self.avgpool(features)
        features = features.flatten(1)  # [B, 1408]

        return (
            self.head_a(features),
            self.head_b(features),
            self.head_c(features),
            self.head_d(features),
        )
