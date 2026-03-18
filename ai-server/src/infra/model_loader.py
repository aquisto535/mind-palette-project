import os
import torch
from dataclasses import dataclass
from typing import Any, Optional
from pathlib import Path
from src.config import ModelConfig
from src.infra.logger import get_logger

logger = get_logger(__name__)

@dataclass
class ModelState:
    male_engine: Optional[Any] = None
    female_engine: Optional[Any] = None
    engine_type: str = "none"

    @property
    def male_loaded(self) -> bool:
        return self.male_engine is not None

    @property
    def female_loaded(self) -> bool:
        return self.female_engine is not None


def load_models(config: ModelConfig) -> ModelState:
    state = ModelState()
    
    # Use exact same model base names as config.male_onnx_path
    trt_male = config.male_onnx_path.replace(".onnx", ".engine")
    trt_female = config.female_onnx_path.replace(".onnx", ".engine")
    
    try:
        if torch.cuda.is_available():
            from src.infra.tensorrt_engine import TensorRtNativeEngine
            if os.path.exists(trt_male) and os.path.exists(trt_female):
                state.male_engine = TensorRtNativeEngine(trt_male)
                state.female_engine = TensorRtNativeEngine(trt_female)
                state.engine_type = "tensorrt"
                logger.info("Loaded TensorRT engines successfully.")
                return state
    except Exception as e:
        logger.warning(f"TensorRT 엔진 로드 실패 (Fallback to ONNX): {e}")

    # Fallback to ONNX
    try:
        from src.infra.onnx_inference import OnnxInferenceEngine
        if os.path.exists(config.male_onnx_path) and os.path.exists(config.female_onnx_path):
            state.male_engine = OnnxInferenceEngine(config.male_onnx_path)
            state.female_engine = OnnxInferenceEngine(config.female_onnx_path)
            state.engine_type = "onnx"
            logger.info("Loaded ONNX engines successfully.")
            return state
    except Exception as e:
        logger.error(f"Failed to load ONNX models: {e}")

    return state
