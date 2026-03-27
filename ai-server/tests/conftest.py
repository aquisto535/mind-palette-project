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
    """HFDClassifier를 ONNX로 변환하여 임시 파일 경로 반환 (세션당 1회).

    전용 tmpdir 사용: .onnx와 .onnx.data(external weights)가 같은 디렉토리에 위치하도록
    보장하여 TensorRT 파서가 external data를 올바르게 탐색할 수 있게 한다.
    """
    import shutil

    import torch
    from src.core.model import HFDClassifier

    _ensure_utf8_stdout()

    model = HFDClassifier(config)
    model.eval()

    tmpdir = tempfile.mkdtemp()
    path = os.path.join(tmpdir, "model.onnx")

    dummy_input = torch.zeros(1, config.input_channels, config.input_size, config.input_size)
    # dynamo=True + dynamic_shapes: PyTorch 2.9+ 새 ONNX exporter 사용
    batch_dim = torch.export.Dim("batch_size", min=1, max=16)
    dynamic_shapes = {"x": {0: batch_dim}}
    torch.onnx.export(
        model,
        (dummy_input,),
        path,
        opset_version=config.onnx_opset_version,
        input_names=["input"],
        output_names=["head_a", "head_b", "head_c", "head_d"],
        dynamic_shapes=dynamic_shapes,
        do_constant_folding=True,
        dynamo=True,
    )

    yield path

    shutil.rmtree(tmpdir, ignore_errors=True)


@pytest.fixture(scope="module")
def trt_engine_path(config, onnx_model_path):
    """ONNX → TRT .engine 파일 빌드 후 임시 경로 yield, 종료 시 삭제."""
    import torch

    if not torch.cuda.is_available():
        pytest.skip("CUDA GPU가 없어 TensorRT 픽스처를 건너뜁니다.")

    from src.infra.tensorrt_engine import build_tensorrt_engine

    with tempfile.NamedTemporaryFile(suffix=".engine", delete=False) as f:
        engine_path = f.name

    build_tensorrt_engine(
        onnx_path=onnx_model_path,
        engine_path=engine_path,
        fp16=config.trt_fp16_enable,
        workspace_gb=config.trt_workspace_gb,
        input_shape=(1, config.input_channels, config.input_size, config.input_size),
    )

    yield engine_path

    os.unlink(engine_path)


@pytest.fixture(scope="module")
def trt_native_engine(trt_engine_path):
    """TensorRtNativeEngine 인스턴스 반환 (module-scoped)."""
    from src.infra.tensorrt_engine import TensorRtNativeEngine

    return TensorRtNativeEngine(trt_engine_path, fp16=True)
