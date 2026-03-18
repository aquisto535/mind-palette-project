"""TensorRT Native Python API 기반 추론 엔진.

흐름:
1. ONNX → TensorRT Engine 빌드 (trt.Builder) 또는 .engine 파일 로드
   - FP16 활성화 (trt.BuilderFlag.FP16)
2. .engine 파일 로드 → IRuntime.deserialize_cuda_engine()
3. IExecutionContext 생성 → execute_v2() 실행
"""

from typing import List, Tuple

import numpy as np

try:
    import tensorrt as trt  # type: ignore[import-untyped]
    import torch

    _TRT_AVAILABLE = True
except ImportError:
    _TRT_AVAILABLE = False


class TensorRtNativeEngine:
    """TensorRT Native API 기반 HFD 추론 엔진.

    Args:
        engine_path: .engine 직렬화 파일 경로
        fp16: FP16 추론 여부 (엔진 빌드 시 FP16이 활성화된 경우에만 유효)
    """

    OUTPUT_NAMES = ["head_a", "head_b", "head_c", "head_d"]
    # HFDClassifier 4 head 출력 크기
    _HEAD_SIZES = {"head_a": 19, "head_b": 14, "head_c": 16, "head_d": 11}

    def __init__(self, engine_path: str, fp16: bool = True) -> None:
        if not _TRT_AVAILABLE:
            raise RuntimeError(
                "tensorrt 패키지가 설치되지 않았습니다. "
                "pip install tensorrt-cu12 를 실행하세요."
            )
        if not torch.cuda.is_available():
            raise RuntimeError(
                "CUDA GPU가 필요합니다. "
                "GPU 환경에서만 TensorRtNativeEngine을 사용할 수 있습니다."
            )

        self._fp16 = fp16
        self._logger = trt.Logger(trt.Logger.WARNING)

        self._engine = self._load_engine(engine_path)
        self._context = self._engine.create_execution_context()

    def _load_engine(self, path: str) -> "trt.ICudaEngine":
        runtime = trt.Runtime(self._logger)
        with open(path, "rb") as f:
            engine_data = f.read()
        engine = runtime.deserialize_cuda_engine(engine_data)
        if engine is None:
            raise RuntimeError(f".engine 파일 역직렬화 실패: {path}")
        return engine

    @property
    def output_names(self) -> List[str]:
        return self.OUTPUT_NAMES

    def run(self, image: np.ndarray) -> Tuple[np.ndarray, ...]:
        """TensorRT 추론 실행.

        Args:
            image: float32 numpy 배열, shape=(batch, 3, H, W)

        Returns:
            (head_a, head_b, head_c, head_d) CPU numpy float32 배열 튜플
        """
        if image.dtype != np.float32:
            image = image.astype(np.float32)

        batch_size = image.shape[0]
        # TRT FP16 엔진도 I/O 바인딩은 float32: 내부 레이어만 FP16으로 동작
        x_gpu = torch.from_numpy(image).to(dtype=torch.float32).cuda()

        # 동적 배치 엔진: execute_v2 전에 실제 입력 shape를 컨텍스트에 등록해야 함
        self._context.set_input_shape("input", image.shape)

        outputs_gpu = [
            torch.zeros(
                (batch_size, self._HEAD_SIZES[name]),
                dtype=torch.float32,
                device="cuda",
            )
            for name in self.OUTPUT_NAMES
        ]

        bindings: List[int] = [x_gpu.data_ptr()] + [
            o.data_ptr() for o in outputs_gpu
        ]
        self._context.execute_v2(bindings=bindings)
        torch.cuda.synchronize()

        return tuple(o.cpu().numpy().astype(np.float32) for o in outputs_gpu)


def build_tensorrt_engine(
    onnx_path: str,
    engine_path: str,
    fp16: bool = True,
    workspace_gb: int = 2,
    input_shape: tuple = (1, 3, 260, 260),
    max_batch_size: int = 8,
) -> None:
    """ONNX 파일을 TensorRT .engine 파일로 변환한다.

    Args:
        onnx_path: 입력 .onnx 파일 경로
        engine_path: 출력 .engine 파일 경로
        fp16: FP16 양자화 활성화 여부
        workspace_gb: 빌드 워크스페이스 크기(GB)
        input_shape: 입력 텐서 shape (batch, C, H, W)
        max_batch_size: Optimization Profile 최대 배치 크기
    """
    if not _TRT_AVAILABLE:
        raise RuntimeError(
            "tensorrt 패키지가 설치되지 않았습니다. "
            "pip install tensorrt-cu12 를 실행하세요."
        )

    logger = trt.Logger(trt.Logger.WARNING)
    builder = trt.Builder(logger)
    network_flags = 1 << int(trt.NetworkDefinitionCreationFlag.EXPLICIT_BATCH)
    network = builder.create_network(network_flags)
    parser = trt.OnnxParser(network, logger)

    # parse_from_file() 사용: external data(.onnx.data)를 같은 디렉토리에서 자동 탐색
    if not parser.parse_from_file(onnx_path):
        errors = [str(parser.get_error(i)) for i in range(parser.num_errors)]
        raise RuntimeError(f"ONNX 파싱 실패: {errors}")

    build_config = builder.create_builder_config()
    if fp16 and builder.platform_has_fast_fp16:
        build_config.set_flag(trt.BuilderFlag.FP16)

    build_config.set_memory_pool_limit(
        trt.MemoryPoolType.WORKSPACE, workspace_gb << 30
    )

    # Optimization Profile: dynamic_axes로 배치 차원이 동적인 경우 필수
    # min=1, opt=1(일반적 추론), max=max_batch_size
    _, c, h, w = input_shape
    profile = builder.create_optimization_profile()
    profile.set_shape(
        "input",
        min=(1, c, h, w),
        opt=(1, c, h, w),
        max=(max_batch_size, c, h, w),
    )
    build_config.add_optimization_profile(profile)

    serialized = builder.build_serialized_network(network, build_config)
    if serialized is None:
        raise RuntimeError("TensorRT 엔진 빌드 실패")

    with open(engine_path, "wb") as f:
        f.write(serialized)
