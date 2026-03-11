import io
import os
import sys
import tempfile

import pytest

from src.config import ModelConfig
from src.main import create_app


def _ensure_utf8_stdout() -> None:
    """Windows cp949 환경에서 torch.onnx.export 내부 출력 인코딩 오류 방지."""
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    elif sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")


@pytest.fixture
def app():
    """테스트용 FastAPI 앱 인스턴스 (테스트마다 독립 생성)."""
    return create_app()


@pytest.fixture(scope="session")
def config():
    """테스트용 ModelConfig (기본값 사용)."""
    return ModelConfig()


@pytest.fixture(scope="session")
def onnx_model_path(config):
    """HFDClassifier를 ONNX로 변환하여 임시 파일 경로 반환 (세션당 1회)."""
    import torch
    from src.core.model import HFDClassifier

    _ensure_utf8_stdout()

    model = HFDClassifier(config)
    model.eval()

    with tempfile.NamedTemporaryFile(suffix=".onnx", delete=False) as f:
        path = f.name

    dummy_input = torch.zeros(1, config.input_channels, config.input_size, config.input_size)
    # dynamo=False (기본값) 사용: TorchScript 기반 안정적 변환
    # dynamo=True는 dynamic_axes 대신 dynamic_shapes가 필요하며 실험적 기능
    torch.onnx.export(
        model,
        (dummy_input,),
        path,
        opset_version=config.onnx_opset_version,
        input_names=["input"],
        output_names=["head_a", "head_b", "head_c", "head_d"],
        dynamic_axes={"input": {0: "batch_size"}},
        do_constant_folding=True,
    )

    yield path

    os.unlink(path)
