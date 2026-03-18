"""ONNX Runtime TensorrtExecutionProvider 기반 추론 엔진.

onnxruntime-gpu 패키지의 TensorrtExecutionProvider를 사용한다.
TensorRT가 지원하지 않는 연산은 CUDAExecutionProvider로 자동 fallback.
OnnxInferenceEngine과 동일한 인터페이스를 제공한다.
"""

import os
from typing import List, Tuple

import numpy as np
import onnxruntime as ort


class TensorRtOrtEngine:
    """ORT TensorrtExecutionProvider 기반 HFD 추론 엔진.

    onnxruntime-gpu의 TensorrtExecutionProvider를 통해 TensorRT 가속을 제공하며,
    .onnx 파일을 직접 입력으로 받아 TRT 엔진을 내부적으로 캐시한다.

    Args:
        onnx_path: .onnx 모델 파일 경로
        fp16: FP16 양자화 활성화 여부
        engine_cache_dir: TRT 엔진 캐시 저장 디렉토리
    """

    def __init__(
        self,
        onnx_path: str,
        fp16: bool = True,
        engine_cache_dir: str = "models/trt_cache",
    ) -> None:
        os.makedirs(engine_cache_dir, exist_ok=True)

        trt_ep_options = {
            "trt_fp16_enable": fp16,
            "trt_engine_cache_enable": True,
            "trt_engine_cache_path": engine_cache_dir,
        }

        providers = [
            ("TensorrtExecutionProvider", trt_ep_options),
            "CUDAExecutionProvider",
            "CPUExecutionProvider",
        ]

        self._session = ort.InferenceSession(onnx_path, providers=providers)
        self._input_name: str = self._session.get_inputs()[0].name
        self._output_names: List[str] = [
            o.name for o in self._session.get_outputs()
        ]

    @property
    def providers(self) -> List[str]:
        return self._session.get_providers()

    @property
    def output_names(self) -> List[str]:
        return self._output_names

    def run(self, image: np.ndarray) -> Tuple[np.ndarray, ...]:
        """ORT TensorRT Provider로 추론 실행.

        Args:
            image: float32 numpy 배열, shape=(batch, 3, H, W)

        Returns:
            (head_a, head_b, head_c, head_d) numpy 배열 튜플
        """
        if image.dtype != np.float32:
            image = image.astype(np.float32)

        outputs: List[np.ndarray] = self._session.run(
            self._output_names, {self._input_name: image}
        )
        return tuple(outputs)  # type: ignore[return-value]
