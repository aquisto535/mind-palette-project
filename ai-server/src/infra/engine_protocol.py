"""추론 엔진 공통 인터페이스 (Protocol 기반 duck typing).

OnnxInferenceEngine, TensorRtNativeEngine, TensorRtOrtEngine이
명시적 상속 없이 동일 인터페이스로 교환 가능하게 한다.
"""

from typing import Protocol, Tuple, runtime_checkable

import numpy as np


@runtime_checkable
class InferenceEngine(Protocol):
    """HFD 추론 엔진 공통 인터페이스.

    이 Protocol을 구현하는 클래스는 명시적 상속 없이도
    isinstance(engine, InferenceEngine) 검사를 통과한다.
    """

    def run(self, image: np.ndarray) -> Tuple[np.ndarray, ...]:
        """추론 실행.

        Args:
            image: float32 numpy 배열, shape=(batch, 3, H, W)

        Returns:
            (head_a, head_b, head_c, head_d) 각각 numpy 배열
        """
        ...

    @property
    def output_names(self) -> list[str]:
        """출력 노드 이름 목록."""
        ...
