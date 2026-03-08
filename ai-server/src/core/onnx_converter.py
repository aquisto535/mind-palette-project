"""HFDClassifier → ONNX 변환 모듈.

ADR-018: TorchScript 기반 안정적 변환 (dynamo=False).
동적 배치 차원만 허용하고 입력 spatial 크기는 고정.
"""

from pathlib import Path

import torch

from src.config import ModelConfig
from src.core.model import HFDClassifier


def export_to_onnx(model: HFDClassifier, config: ModelConfig, output_path: str) -> Path:
    """HFDClassifier를 ONNX 파일로 변환한다.

    Args:
        model: eval 상태의 HFDClassifier
        config: ModelConfig (opset, 입력 크기 등 참조)
        output_path: 저장할 .onnx 파일 경로

    Returns:
        저장된 파일의 Path 객체

    Raises:
        RuntimeError: ONNX 검증 실패 시
    """
    import onnx

    model.eval()
    dummy_input = torch.zeros(
        1, config.input_channels, config.input_size, config.input_size
    )

    torch.onnx.export(
        model,
        (dummy_input,),
        output_path,
        opset_version=config.onnx_opset_version,
        input_names=["input"],
        output_names=["head_a", "head_b", "head_c", "head_d"],
        dynamic_axes={"input": {0: "batch_size"}},
        do_constant_folding=True,
    )

    # 변환 결과 무결성 검증
    onnx_model = onnx.load(output_path)
    onnx.checker.check_model(onnx_model)

    return Path(output_path)
