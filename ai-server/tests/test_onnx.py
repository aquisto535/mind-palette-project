"""ONNX 변환 및 Runtime 추론 테스트 (Phase 4 Step 2).

L1: ONNX 파일 구조 및 InferenceSession 생성
L2: PyTorch vs ONNX 추론 동등성
L3: 추론 지연시간 회귀
"""

import time
from typing import List

import numpy as np
import onnx
import onnxruntime as ort
import pytest
import torch

from src.config import ModelConfig
from src.core.model import HFDClassifier


@pytest.fixture(scope="module")
def model_and_onnx_path(config: ModelConfig):
    """동등성 테스트를 위해 동일한 모델 인스턴스로 ONNX를 변환한 (model, path) 쌍 반환."""
    import io
    import os
    import sys
    import tempfile

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    model = HFDClassifier(config)
    model.eval()

    with tempfile.NamedTemporaryFile(suffix=".onnx", delete=False) as f:
        path = f.name

    dummy_input = torch.zeros(
        1, config.input_channels, config.input_size, config.input_size
    )
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

    yield model, path

    os.unlink(path)


# ──────────────────────────────────────────────
# L1: 데이터 구조 (ONNX 파일 구조 & Session)
# ──────────────────────────────────────────────


class TestOnnxFileStructure:
    """변환된 .onnx 파일의 입출력 노드 구조 검증."""

    def test_onnx_model_loads_without_error(self, onnx_model_path: str) -> None:
        """변환된 .onnx 파일이 오류 없이 로드되어야 한다."""
        model = onnx.load(onnx_model_path)
        onnx.checker.check_model(model)

    def test_onnx_input_node_shape(self, onnx_model_path: str, config: ModelConfig) -> None:
        """입력 노드의 shape이 (batch, 3, 260, 260)이어야 한다."""
        model = onnx.load(onnx_model_path)
        input_node = model.graph.input[0]

        shape = input_node.type.tensor_type.shape
        dims: List[int | None] = []
        for d in shape.dim:
            if d.HasField("dim_param"):
                dims.append(None)  # dynamic axis
            else:
                dims.append(d.dim_value)

        assert dims[1] == config.input_channels, f"channel dim: {dims[1]}"
        assert dims[2] == config.input_size, f"height dim: {dims[2]}"
        assert dims[3] == config.input_size, f"width dim: {dims[3]}"

    def test_onnx_output_node_count(self, onnx_model_path: str) -> None:
        """출력 노드가 정확히 4개(head_a, head_b, head_c, head_d)여야 한다."""
        model = onnx.load(onnx_model_path)
        assert len(model.graph.output) == 4

    def test_onnx_output_node_names(self, onnx_model_path: str) -> None:
        """출력 노드 이름이 head_a, head_b, head_c, head_d이어야 한다."""
        model = onnx.load(onnx_model_path)
        output_names = [o.name for o in model.graph.output]
        assert output_names == ["head_a", "head_b", "head_c", "head_d"]

    def test_onnx_output_shapes(self, onnx_model_path: str, config: ModelConfig) -> None:
        """각 출력 노드의 feature 차원이 19/14/16/11이어야 한다."""
        model = onnx.load(onnx_model_path)
        expected_sizes = [
            config.head_a_size,
            config.head_b_size,
            config.head_c_size,
            config.head_d_size,
        ]
        for output, expected in zip(model.graph.output, expected_sizes):
            shape = output.type.tensor_type.shape
            feat_dim = shape.dim[1].dim_value
            assert feat_dim == expected, f"{output.name}: expected {expected}, got {feat_dim}"


class TestOnnxRuntimeSession:
    """ONNX Runtime InferenceSession 생성 및 provider 검증."""

    def test_inference_session_creates_successfully(self, onnx_model_path: str) -> None:
        """InferenceSession 객체가 오류 없이 생성되어야 한다."""
        session = ort.InferenceSession(onnx_model_path)
        assert session is not None

    def test_inference_session_has_cpu_provider(self, onnx_model_path: str) -> None:
        """CPUExecutionProvider가 활성화되어 있어야 한다."""
        session = ort.InferenceSession(onnx_model_path)
        providers = session.get_providers()
        assert "CPUExecutionProvider" in providers

    def test_inference_session_input_name(self, onnx_model_path: str) -> None:
        """세션의 입력 이름이 'input'이어야 한다."""
        session = ort.InferenceSession(onnx_model_path)
        input_names = [i.name for i in session.get_inputs()]
        assert "input" in input_names

    def test_inference_session_output_names(self, onnx_model_path: str) -> None:
        """세션의 출력 이름이 head_a/b/c/d이어야 한다."""
        session = ort.InferenceSession(onnx_model_path)
        output_names = [o.name for o in session.get_outputs()]
        assert output_names == ["head_a", "head_b", "head_c", "head_d"]


# ──────────────────────────────────────────────
# L2: 변환 로직 (PyTorch vs ONNX 동등성)
# ──────────────────────────────────────────────


class TestOnnxConversionEquivalence:
    """동일 입력에 대해 PyTorch와 ONNX Runtime 출력 차이가 허용 오차 이하여야 한다.

    핵심: model_and_onnx_path 픽스처를 통해 ONNX 변환에 사용된 모델과
    동일한 인스턴스로 PyTorch 추론을 수행하여 공정한 비교를 보장한다.
    """

    TOLERANCE = 1e-4  # CPU 부동소수점 오차 허용 한계

    def _run_pytorch(
        self, model: HFDClassifier, input_tensor: torch.Tensor
    ) -> List[np.ndarray]:
        model.eval()
        with torch.no_grad():
            outputs = model(input_tensor)
        return [o.numpy() for o in outputs]

    def _run_onnx(
        self, path: str, input_array: np.ndarray
    ) -> List[np.ndarray]:
        session = ort.InferenceSession(path)
        return session.run(None, {"input": input_array})

    def test_head_a_output_matches(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """head_a 출력의 L2 norm 차이가 TOLERANCE 이하여야 한다."""
        model, path = model_and_onnx_path
        torch.manual_seed(0)
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        diff = np.linalg.norm(pt_out[0] - ort_out[0])
        assert diff < self.TOLERANCE, f"head_a L2 diff={diff:.2e} > {self.TOLERANCE}"

    def test_head_b_output_matches(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """head_b 출력의 L2 norm 차이가 TOLERANCE 이하여야 한다."""
        model, path = model_and_onnx_path
        torch.manual_seed(1)
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        diff = np.linalg.norm(pt_out[1] - ort_out[1])
        assert diff < self.TOLERANCE, f"head_b L2 diff={diff:.2e} > {self.TOLERANCE}"

    def test_head_c_output_matches(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """head_c 출력의 L2 norm 차이가 TOLERANCE 이하여야 한다."""
        model, path = model_and_onnx_path
        torch.manual_seed(2)
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        diff = np.linalg.norm(pt_out[2] - ort_out[2])
        assert diff < self.TOLERANCE, f"head_c L2 diff={diff:.2e} > {self.TOLERANCE}"

    def test_head_d_output_matches(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """head_d 출력의 L2 norm 차이가 TOLERANCE 이하여야 한다."""
        model, path = model_and_onnx_path
        torch.manual_seed(3)
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        diff = np.linalg.norm(pt_out[3] - ort_out[3])
        assert diff < self.TOLERANCE, f"head_d L2 diff={diff:.2e} > {self.TOLERANCE}"

    def test_all_heads_max_abs_error(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """모든 Head의 최대 절대 오차(max abs error)가 1e-4 이하여야 한다."""
        model, path = model_and_onnx_path
        torch.manual_seed(42)
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        for i, (pt, ort_o) in enumerate(zip(pt_out, ort_out)):
            max_err = np.abs(pt - ort_o).max()
            assert max_err < self.TOLERANCE, (
                f"head_{['a','b','c','d'][i]} max abs err={max_err:.2e} > {self.TOLERANCE}"
            )

    def test_batch_inference_equivalence(
        self, model_and_onnx_path: tuple, config: ModelConfig
    ) -> None:
        """배치 추론(batch=4)에서도 PyTorch vs ONNX 결과가 일치해야 한다."""
        model, path = model_and_onnx_path
        batch_size = 4
        torch.manual_seed(7)
        x = torch.randn(
            batch_size, config.input_channels, config.input_size, config.input_size
        )

        pt_out = self._run_pytorch(model, x)
        ort_out = self._run_onnx(path, x.numpy())

        for i, (pt, ort_o) in enumerate(zip(pt_out, ort_out)):
            max_err = np.abs(pt - ort_o).max()
            assert max_err < self.TOLERANCE, (
                f"batch head_{['a','b','c','d'][i]} max err={max_err:.2e}"
            )


# ──────────────────────────────────────────────
# L3: 제약과 검증 (추론 지연시간 회귀)
# ──────────────────────────────────────────────


class TestOnnxLatencyRegression:
    """ONNX Runtime P95 latency가 PyTorch 대비 동등 이하인지 벤치마크."""

    N_WARMUP = 3
    N_RUNS = 20

    def _measure_latency(
        self, fn, n_warmup: int = N_WARMUP, n_runs: int = N_RUNS
    ) -> dict:
        for _ in range(n_warmup):
            fn()

        times = []
        for _ in range(n_runs):
            t0 = time.perf_counter()
            fn()
            times.append(time.perf_counter() - t0)

        times_ms = sorted(t * 1000 for t in times)
        return {
            "p50": times_ms[len(times_ms) // 2],
            "p95": times_ms[int(len(times_ms) * 0.95)],
            "mean": sum(times_ms) / len(times_ms),
        }

    def test_onnx_p95_latency_within_2x_pytorch(
        self, onnx_model_path: str, config: ModelConfig
    ) -> None:
        """ONNX P95 latency가 PyTorch P95 latency의 2배 이하여야 한다.

        Deep Dive: ONNX Runtime은 연산자 최적화로 CPU 추론에서
        PyTorch 대비 동등하거나 빠른 성능을 내야 한다.
        2배 여유를 두어 환경 차이(캐시 콜드 스타트 등)를 흡수.
        """
        x = torch.randn(1, config.input_channels, config.input_size, config.input_size)
        x_np = x.numpy()
        model = HFDClassifier(config)
        model.eval()
        session = ort.InferenceSession(onnx_model_path)

        def run_pytorch() -> None:
            with torch.no_grad():
                model(x)

        def run_onnx() -> None:
            session.run(None, {"input": x_np})

        pt_stats = self._measure_latency(run_pytorch)
        ort_stats = self._measure_latency(run_onnx)

        # 결과 출력 (벤치마크 주석 — Deep Dive)
        print(
            f"\n[Latency Benchmark] PyTorch P95={pt_stats['p95']:.1f}ms, "
            f"mean={pt_stats['mean']:.1f}ms | "
            f"ONNX P95={ort_stats['p95']:.1f}ms, mean={ort_stats['mean']:.1f}ms"
        )

        assert ort_stats["p95"] <= pt_stats["p95"] * 2.0, (
            f"ONNX P95 ({ort_stats['p95']:.1f}ms) > PyTorch P95 ({pt_stats['p95']:.1f}ms) * 2"
        )

    def test_onnx_single_inference_under_500ms(
        self, onnx_model_path: str, config: ModelConfig
    ) -> None:
        """ONNX Runtime 단일 추론이 500ms 이하여야 한다 (CPU, 절대 한계).

        이 값은 회귀 방지용 안전망이며, 실제 목표는 100ms 이하다.
        """
        rng = np.random.default_rng(seed=0)
        x_np = rng.standard_normal(
            (1, config.input_channels, config.input_size, config.input_size)
        ).astype(np.float32)
        session = ort.InferenceSession(onnx_model_path)

        # warmup
        session.run(None, {"input": x_np})

        t0 = time.perf_counter()
        session.run(None, {"input": x_np})
        elapsed_ms = (time.perf_counter() - t0) * 1000

        assert elapsed_ms < 500.0, f"ONNX single inference {elapsed_ms:.1f}ms >= 500ms"
