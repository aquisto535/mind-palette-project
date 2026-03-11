"""HFD (Human Figure Drawing) Multi-label Binary Classifier.

ADR-018: EfficientNet-B2 backbone (frozen) + 4 Linear heads (19/14/16/11).
forward() returns raw logits as Tuple for ONNX compatibility.
Sigmoid is applied only at inference time, NOT inside forward().
"""

from typing import Dict, Tuple

import torch
import torch.nn as nn
from torchvision.models import efficientnet_b2, EfficientNet_B2_Weights

from src.config import ModelConfig


class HFDClassifier(nn.Module):
    """HFD (Human Figure Drawing) Multi-label Classifier.
    
    Backbone: EfficientNet-B2 (Feature extractor)
    Heads: Dynamic Linear heads for sub-categories (Head A, B, C, D)
    """
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()

        # Backbone: EfficientNet-B2
        backbone_full = efficientnet_b2(weights=EfficientNet_B2_Weights.DEFAULT)
        self.backbone = backbone_full.features
        self.avgpool = backbone_full.avgpool

        # Freeze backbone (Transfer Learning)
        for param in self.backbone.parameters():
            param.requires_grad = False

        feature_dim = config.backbone_feature_dim  # 1408
        
        # heads를 ModuleDict로 추적하여 확장성 확보
        self.heads = nn.ModuleDict({
            'head_a': nn.Linear(feature_dim, config.head_a_size),  # 머리/얼굴
            'head_b': nn.Linear(feature_dim, config.head_b_size),  # 몸통/비례
            'head_c': nn.Linear(feature_dim, config.head_c_size),  # 사지/말단
            'head_d': nn.Linear(feature_dim, config.head_d_size),  # 의복/질적
        })

    def forward(
        self, x: torch.Tensor
    ) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        """Forward pass.
        
        Args:
            x: Input image tensor [Batch, 3, 512, 512]
            
        Returns:
            Tuple of raw logits (head_a, head_b, head_c, head_d)
        """
        features = self.backbone(x)
        features = self.avgpool(features)
        features = features.flatten(1)  # [Batch, 1408]

        # ONNX 변환 및 기존 호환성을 위해 튜플로 반환
        return (
            self.heads['head_a'](features),
            self.heads['head_b'](features),
            self.heads['head_c'](features),
            self.heads['head_d'](features),
        )
