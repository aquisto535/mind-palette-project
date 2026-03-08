"""모델 로드 및 상태 관리.

남녀 모델을 독립적으로 로드하고 상태를 추적한다.
가중치 파일이 없어도 서버는 정상 기동한다.
"""

import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import torch

from src.config import ModelConfig
from src.core.model import HFDClassifier

logger = logging.getLogger(__name__)


@dataclass
class ModelState:
    male_model: Optional[HFDClassifier] = None
    female_model: Optional[HFDClassifier] = None

    @property
    def male_loaded(self) -> bool:
        return self.male_model is not None

    @property
    def female_loaded(self) -> bool:
        return self.female_model is not None


def load_models(config: ModelConfig) -> ModelState:
    """남녀 모델을 독립적으로 로드. 실패 시 해당 모델만 None."""
    state = ModelState()

    state.male_model = _try_load(config, config.male_model_path, "male")
    state.female_model = _try_load(config, config.female_model_path, "female")

    return state


def _try_load(
    config: ModelConfig, model_path: str, gender: str
) -> Optional[HFDClassifier]:
    path = Path(model_path)
    if not path.exists():
        logger.warning(f"{gender} model not found at {path}")
        return None

    try:
        model = HFDClassifier(config)
        model.load_state_dict(torch.load(path, map_location=config.device))
        model.eval()
        logger.info(f"{gender} model loaded from {path}")
        return model
    except Exception as e:
        logger.error(f"Failed to load {gender} model: {e}")
        return None
