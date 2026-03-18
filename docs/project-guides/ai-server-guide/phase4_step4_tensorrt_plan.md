# Phase 4 Step 4: TensorRT Extreme Optimization 구현 계획

## Context
Step 3 완료(108/108 테스트 통과)로 학습 파이프라인이 완성됐다.
Step 4는 ONNX(CPU, P95=19.6ms)를 넘어 TensorRT FP16 GPU 추론으로 최대 4-6x 가속을 달성하고,
PyTorch / ONNX / TensorRT 3-Engine 벤치마크 리포트를 자동 생성하는 것이 목표다.

현재 환경: RTX 3050 (4GB, CUDA 12.6) + PyTorch 2.10.0+cpu (CUDA 버전 교체 필요).

---

## 0. 사전 작업: 환경 설정

### PyTorch CUDA 교체 + TensorRT 설치
```bash
# 1. CPU PyTorch 제거 후 CUDA 12.6 버전 설치
pip uninstall torch torchvision torchaudio -y
pip install torch==2.6.0+cu126 torchvision==0.21.0+cu126 --index-url https://download.pytorch.org/whl/cu126

# 2. onnxruntime-gpu (TensorrtExecutionProvider 포함) — cpu 버전 교체
pip uninstall onnxruntime -y
pip install onnxruntime-gpu

# 3. TensorRT Native API
pip install tensorrt-cu12

# 4. 검증
python -c "import torch; print(torch.cuda.is_available(), torch.cuda.get_device_name(0))"
python -c "import tensorrt; print('TRT:', tensorrt.__version__)"
```

**주의**: `onnxruntime-gpu`는 `onnxruntime`과 동일하게 import되므로 기존 코드 변경 없음.

---

## 1. 구현 파일 목록

### 새로 생성
| 파일 | 역할 |
|------|------|
| `src/infra/engine_protocol.py` | `InferenceEngine` Protocol (duck typing) |
| `src/infra/tensorrt_engine.py` | `TensorRtNativeEngine` (trt.Builder, .engine 직접 관리) |
| `src/infra/tensorrt_ort_engine.py` | `TensorRtOrtEngine` (ORT TensorrtExecutionProvider, FP16) |
| `tests/test_tensorrt.py` | L1/L2/L3 TDD 테스트 전체 |

### 수정
| 파일 | 변경 |
|------|------|
| `src/config.py` | TensorRT 설정 필드 4개 추가 |
| `tests/conftest.py` | `trt_engine_path`, `trt_native_engine` 픽스처 추가 |
| `pyproject.toml` | `onnxruntime` → `onnxruntime-gpu`, TensorRT optional dep 추가 |

---

## 2. Structural 변경 (Tidy First)

### 2-1. `src/infra/engine_protocol.py` (새 파일)
```python
from typing import Protocol, Tuple, runtime_checkable
import numpy as np

@runtime_checkable
class InferenceEngine(Protocol):
    def run(self, image: np.ndarray) -> Tuple[np.ndarray, ...]: ...
    @property
    def output_names(self) -> list[str]: ...
```

### 2-2. `src/config.py` 추가 필드
```python
# TensorRT 설정 (ModelConfig에 추가)
male_trt_engine_path: str = "models/mind_palette_male.engine"
female_trt_engine_path: str = "models/mind_palette_female.engine"
trt_fp16_enable: bool = True
trt_engine_cache_dir: str = "models/trt_cache"
trt_workspace_gb: int = 2  # RTX 3050 4GB에서 2GB 빌드 워크스페이스

# inference_backend 기존 필드 주석 확장:
# "pytorch" | "onnx" | "tensorrt_native" | "tensorrt_ort"
```

**커밋**: `refactor: InferenceEngine Protocol 및 TensorRT 설정 추가`

---

## 3. TDD Red: `tests/test_tensorrt.py` 구조

### 모듈 레벨 skipif
```python
pytestmark = pytest.mark.skipif(
    not torch.cuda.is_available(),
    reason="CUDA GPU 없으면 TensorRT 테스트 스킵"
)
```

### conftest.py 추가 픽스처
```python
@pytest.fixture(scope="module")
def trt_engine_path(config, onnx_model_path):
    """ONNX → TRT .engine 빌드 후 임시 경로 yield, 종료 시 삭제."""
    # trt.Builder → FP16 플래그 → build_serialized_network → .engine 저장

@pytest.fixture(scope="module")
def trt_native_engine(trt_engine_path):
    from src.infra.tensorrt_engine import TensorRtNativeEngine
    return TensorRtNativeEngine(trt_engine_path, fp16=True)
```

### L1: `TestTensorRtEngineStructure` (4개 테스트)
- `test_engine_file_exists_and_nonzero` — .engine 파일 크기 > 0
- `test_engine_loads_without_error` — trt.Runtime으로 로드 성공
- `test_gpu_memory_context_creation` — ExecutionContext 생성 성공 (GPU 할당 확인)
- `test_engine_protocol_compliance` — `InferenceEngine` Protocol 준수 (hasattr run/output_names)

### L2: `TestTensorRtFp16Accuracy` (3개 테스트)
- `test_fp16_fp32_head_a_accuracy` — head_a FP32 vs FP16 확률 차이 < 0.01
- `test_fp16_fp32_all_heads_accuracy` — 5 seed × 4 head 전체 최대 차이 < 0.01
- `test_fp16_binary_match_rate` — 이진 예측 일치율 ≥ 95%

### L3: `TestThreeEngineBenchmark` (2개 테스트)
- `test_three_engine_benchmark_report_generated` — JSON 리포트 생성 및 구조 검증
- `test_tensorrt_faster_than_onnx_cpu` — TRT P95 ≤ ONNX CPU P95 × 2.0

**커밋**: `test: TensorRT L1/L2/L3 Red 테스트 작성`

---

## 4. TDD Green: 구현

### `src/infra/tensorrt_engine.py` — TensorRtNativeEngine

핵심 흐름:
```
__init__(engine_path):
    trt.Runtime.deserialize_cuda_engine(bytes) → ICudaEngine
    engine.create_execution_context() → IExecutionContext

run(image: np.ndarray):
    image → float16 변환 → torch.cuda Tensor
    output 버퍼 GPU 할당 (head_a:19, head_b:14, head_c:16, head_d:11)
    bindings = [input.data_ptr()] + [o.data_ptr() for o in outputs]
    context.execute_v2(bindings)
    → CPU numpy float32 튜플 반환
```

try/except로 `tensorrt` ImportError → RuntimeError로 변환 (GPU 없는 환경에서 안전).

### `src/infra/tensorrt_ort_engine.py` — TensorRtOrtEngine

```python
providers = [
    ("TensorrtExecutionProvider", {
        "trt_fp16_enable": fp16,
        "trt_engine_cache_enable": True,
        "trt_engine_cache_path": engine_cache_dir,
    }),
    "CUDAExecutionProvider",
    "CPUExecutionProvider",
]
session = ort.InferenceSession(onnx_path, providers=providers)
```

OnnxInferenceEngine과 동일한 인터페이스. 기존 코드 변경 없음.

**커밋**: `feat: TensorRtNativeEngine 및 TensorRtOrtEngine 구현`

---

## 5. 벤치마크 리포트 출력 형식

`test_three_engine_benchmark_report_generated` 실행 시 stdout에 JSON 출력:

```json
{
  "environment": {
    "gpu": "NVIDIA GeForce RTX 3050 Laptop GPU",
    "cuda_version": "12.6",
    "input_shape": [1, 3, 260, 260]
  },
  "engines": {
    "pytorch_gpu":          { "p50_ms": 8.2,  "p95_ms": 9.1,  "throughput_qps": 118, "gpu_memory_mb": 124 },
    "onnx_cpu":             { "p50_ms": 19.6, "p95_ms": 22.1, "throughput_qps":  48, "gpu_memory_mb": 0   },
    "tensorrt_fp16_gpu":    { "p50_ms": 3.1,  "p95_ms": 3.8,  "throughput_qps": 312, "gpu_memory_mb":  88 }
  }
}
```

측정 축: Latency(p50/p95/mean), Throughput(QPS), GPU Memory(MB).

---

## 6. 실행 순서 요약

1. **환경 설정**: PyTorch CUDA 교체 + TensorRT 설치 (pip 명령어 실행)
2. **Structural**: Protocol + config 필드 추가 → 커밋
3. **Red**: `test_tensorrt.py` 작성 → 테스트 실패 확인 → 커밋
4. **Green**: `tensorrt_engine.py` + `tensorrt_ort_engine.py` 구현 → 테스트 통과 → 커밋
5. **Refactor**: 벤치마크 리포트 JSON 파일 저장 옵션 추가 → 전체 테스트 실행
6. **plan.md 업데이트**: Step 4 항목 [x] 체크

---

## 7. 검증 방법

```bash
cd ai-server

# GPU 환경 확인
python -c "import torch; print('CUDA:', torch.cuda.is_available())"

# TensorRT 테스트만 실행
pytest tests/test_tensorrt.py -v -s

# 전체 테스트 회귀 확인 (기존 108개 포함)
pytest tests/ -v --ignore=tests/test_tensorrt.py  # 기존 먼저
pytest tests/test_tensorrt.py -v -s                # TRT 테스트

# 리포트 확인 (-s로 stdout 출력)
pytest tests/test_tensorrt.py::TestThreeEngineBenchmark -v -s
```

---

## 8. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| Python 3.13 + tensorrt-cu12 호환성 | pip install 전 `pip index versions tensorrt-cu12` 확인 |
| FP16 정확도 1%p 초과 | `trt_fp16_enable=False` fallback + 테스트 스킵 마킹 |
| .engine 빌드 시간 (~1-3분) | `scope="module"` 픽스처로 1회만 빌드 |
| RTX 3050 4GB VRAM 부족 | `trt_workspace_gb=2` 워크스페이스 제한 |
