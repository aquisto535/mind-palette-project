"""ONNX Runtime 추론 엔진.

HFDClassifier의 ONNX 버전을 InferenceSession으로 실행한다.
PyTorch 의존성 없이 순수 numpy/onnxruntime으로 동작.
"""

from typing import List, Tuple

import numpy as np
import onnxruntime as ort


class OnnxInferenceEngine:
    """ONNX Runtime 기반 HFD 추론 엔진.

    Args:
        onnx_path: .onnx 모델 파일 경로
        providers: 실행 공급자 목록 (기본값: CPU)
    """

    def __init__(
        self,
        onnx_path: str,
        providers: List[str] | None = None,
    ) -> None:
        if providers is None:
            providers = ["CPUExecutionProvider"]

        self._session = ort.InferenceSession(onnx_path, providers=providers)
        self._input_name: str = self._session.get_inputs()[0].name
        self._output_names: List[str] = [o.name for o in self._session.get_outputs()]

    @property
    def providers(self) -> List[str]:
        return self._session.get_providers()

    @property
    def output_names(self) -> List[str]:
        return self._output_names

    def run(self, image: np.ndarray) -> Tuple[np.ndarray, ...]:
        """ONNX 추론 실행.

        Args:
            image: float32 numpy 배열, shape=(batch, 3, H, W)

        Returns:
            (head_a, head_b, head_c, head_d) 각각 numpy 배열
        """
        if image.dtype != np.float32:
            image = image.astype(np.float32)

        outputs: List[np.ndarray] = self._session.run(
            self._output_names, {self._input_name: image}
        )
        return tuple(outputs)  # type: ignore[return-value]
