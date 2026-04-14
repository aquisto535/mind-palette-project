# Phase 4 Step 2: Universal Optimization (ONNX + Deep Dive) TDD 실행 계획

## Context

Phase 4 Step 1에서 FastAPI + PyTorch 기반 AI 서버가 완성되었습니다 (25개 테스트 통과).
Step 2는 PyTorch 모델을 ONNX 형식으로 변환하고, ONNX Runtime 추론 엔진으로 교체하여 추론 성능을 최적화하는 작업입니다.

**현재 ONNX 준비도 (Step 1에서 확보)**:
- `forward()` → `Tuple[Tensor x4]` 반환 (ONNX multi-output 자동 매핑)
- raw logits 반환 (Sigmoid 미적용, ONNX 그래프 최적화에 유리)
- 입력: `(B, 3, 260, 260)` float32
- 출력: `(B, 19)`, `(B, 14)`, `(B, 16)`, `(B, 11)` float32
- 모든 ops가 ONNX opset 17에서 안정적 지원 (Conv, BatchNorm, AdaptiveAvgPool, Linear)

---

## 디렉토리 구조

### 신규 파일

```
ai-server/
├── src/
│   ├── core/
│   │   └── onnx_converter.py       # [NEW] PyTorch → ONNX 변환 로직
│   └── infra/
│       └── onnx_inference.py        # [NEW] ONNX Runtime 추론 엔진 + InferenceEngine Protocol
└── tests/
    ├── test_onnx_conversion.py      # [NEW] L1+L2: ONNX 파일 구조, 동등성 검증
    ├── test_onnx_inference.py       # [NEW] L1+L2: InferenceSession, predict(), 엔진 통합
    └── test_onnx_benchmark.py       # [NEW] L3: Latency 벤치마크, P95 회귀 테스트
```

### 수정 파일

| 파일 | 수정 내용 |
|------|-----------|
| `ai-server/pyproject.toml` | `onnx>=1.15.0`, `onnxruntime>=1.17.0` 의존성 추가 |
| `ai-server/src/config.py` | ONNX 경로, `inference_backend`, `onnx_opset_version` 추가 |
| `ai-server/src/infra/model_loader.py` | ONNX 엔진 로드 경로, `ModelState` 확장 |
| `ai-server/src/main.py` | `inference_backend` 설정에 따른 분기, config를 app.state에 저장 |
| `ai-server/src/routes/health.py` | 응답에 `inference_backend` 필드 추가 |
| `ai-server/tests/conftest.py` | `onnx_model_path`, `onnx_engine` fixture 추가 |

---

## TDD 실행 순서 (10 Cycles)

### Phase A: 구조 설정 (Tidy First)

**Cycle 0** — `chore: ONNX 의존성 및 config 확장`
- `pyproject.toml`에 `onnx>=1.15.0`, `onnxruntime>=1.17.0` 추가
- `src/config.py`의 `ModelConfig`에 ONNX 설정 추가:
  - `male_onnx_path: str = "models/mind_palette_male.onnx"`
  - `female_onnx_path: str = "models/mind_palette_female.onnx"`
  - `inference_backend: str = "pytorch"` (`"pytorch"` | `"onnx"`)
  - `onnx_opset_version: int = 17`
- 빈 모듈 생성: `src/core/onnx_converter.py`, `src/infra/onnx_inference.py`
- `tests/conftest.py`에 ONNX 관련 fixture 추가
- 기존 25개 테스트 회귀 확인

### Phase B: L1 테스트 (데이터 구조)

**Cycle 1** — ONNX 변환 및 파일 구조 검증
- **Red**: `test_onnx_conversion.py`
  - `test_export_creates_onnx_file` — `convert_to_onnx()` 호출 시 `.onnx` 파일 생성
  - `test_onnx_file_is_valid` — `onnx.checker.check_model()` 통과
  - `test_onnx_model_has_correct_input_shape` — 입력 노드 shape `[1, 3, 260, 260]` (batch dim은 dynamic)
  - `test_onnx_model_has_four_outputs` — 출력 노드 개수 == 4
- **Green**: `src/core/onnx_converter.py` — `convert_to_onnx()` 구현
  - `torch.onnx.export()` 호출
  - `output_names=["head_a", "head_b", "head_c", "head_d"]`
  - `dynamic_axes`로 batch 차원 가변 처리
  - `do_constant_folding=True`로 BatchNorm 등 상수 연산 사전 계산

**Cycle 2** — ONNX 출력 Shape 상세 검증
- **Red**: `test_onnx_conversion.py` 추가
  - `test_onnx_output_shapes` — 각 출력 노드 shape: head_a=(batch,19), head_b=(batch,14), head_c=(batch,16), head_d=(batch,11)
  - `test_onnx_dynamic_batch_axis` — 입력의 batch 차원이 dynamic(가변)으로 설정
  - `test_onnx_input_dtype_float32` — 입력 노드 데이터 타입 == TensorProto.FLOAT
- **Green**: Cycle 1에서 이미 구현됨 (검증 테스트)

**Cycle 3** — ONNX Runtime InferenceSession 생성
- **Red**: `test_onnx_inference.py`
  - `test_inference_session_creation` — `.onnx` 파일로 `InferenceSession` 객체 생성 성공
  - `test_inference_session_provider` — Session의 provider에 `CPUExecutionProvider` 포함
  - `test_inference_session_input_name` — 입력 이름 == `"input"`
  - `test_inference_session_output_names` — 출력 이름 == `["head_a", "head_b", "head_c", "head_d"]`
- **Green**: `src/infra/onnx_inference.py` — `ONNXInferenceEngine.__init__()` 구현

### Phase C: L2 테스트 (변환 로직)

**Cycle 4** — PyTorch vs ONNX Runtime 추론 동등성 (핵심)
- **Red**: `test_onnx_conversion.py` 추가
  - `test_pytorch_vs_onnx_equivalence` — 동일 고정 seed 입력, 4개 head별 L2 norm 차이 < 1e-5
  - `test_pytorch_vs_onnx_equivalence_batch` — batch_size=4에서도 동등성 유지
  - `test_pytorch_vs_onnx_sigmoid_equivalence` — sigmoid 적용 후에도 동등성 유지
- **Green**: 올바른 변환이면 자연스럽게 통과. 실패 시 `do_constant_folding` 옵션 조정

**Cycle 5** — ONNX 추론 엔진 `predict()` 구현
- **Red**: `test_onnx_inference.py` 추가
  - `test_onnx_engine_predict_output_count` — `predict()` 호출 시 4개 numpy array 반환
  - `test_onnx_engine_predict_output_shapes` — 각 출력 shape: (1,19)/(1,14)/(1,16)/(1,11)
  - `test_onnx_engine_predict_dtype` — 모든 출력 dtype == np.float32
  - `test_onnx_engine_predict_batch` — batch_size > 1 입력 처리 가능
  - `test_onnx_engine_predict_deterministic` — 동일 입력 → 동일 결과
- **Green**: `ONNXInferenceEngine.predict()` 구현

**Cycle 6** — 추론 엔진 추상화 및 model_loader 통합
- **Red**: `test_onnx_inference.py` 추가
  - `test_inference_engine_protocol` — `ONNXInferenceEngine`이 `InferenceEngine` Protocol 준수
  - `test_model_state_with_onnx_backend` — `ModelState`가 ONNX 엔진 보유 가능
  - `test_load_models_onnx_backend` — `inference_backend="onnx"` 설정 시 ONNX 엔진 로드
  - `test_load_models_onnx_fallback` — `.onnx` 파일 없을 때 `None` (graceful degradation)
- **Green**: `InferenceEngine` Protocol 정의 + `model_loader.py` 확장

### Phase D: L3 테스트 (제약과 검증)

**Cycle 7** — Deep Dive: Latency Analysis (PyTorch vs ONNX Runtime)
- **Red/Measure**: `test_onnx_benchmark.py`
  - `test_latency_measurement_pytorch` — PyTorch 추론의 P50/P95 latency 측정
  - `test_latency_measurement_onnx` — ONNX Runtime 추론의 P50/P95 latency 측정
- **Green**: `measure_latency()` 벤치마크 유틸리티 구현 (warmup 10회 + iterations 100회)

**Cycle 8** — P95 Latency 벤치마크 회귀 테스트
- **Red**: `test_onnx_benchmark.py` 추가
  - `test_onnx_p95_latency_not_worse_than_pytorch` — ONNX P95 ≤ PyTorch P95 × 1.1 (10% 마진)
- **Green**: ONNX Runtime 그래프 최적화(`ORT_ENABLE_ALL`)로 자연스럽게 통과
- 마커: `@pytest.mark.slow` (CI 기본 실행에서 제외, `pytest -m slow`로 명시 실행)

**Cycle 9** — /health 엔드포인트 backend 정보 통합
- **Red**: `test_health.py` 추가
  - `test_health_includes_backend_info` — `/health` 응답에 `"inference_backend": "pytorch" | "onnx"` 필드 포함
  - `test_health_onnx_backend_models_loaded` — ONNX 백엔드에서도 models.male/female 상태 정확
- **Green**: `src/routes/health.py` + `src/main.py` 수정

---

## 핵심 설계 결정

| 결정 | 선택 | 근거 |
|------|------|------|
| opset_version | 17 | PyTorch 2.x 안정 지원, EfficientNet-B2 전 ops 호환 |
| Tuple 반환 처리 | `output_names` 4개 지정 | `torch.onnx.export()`가 Tuple → multi-output 자동 처리 |
| Dynamic batch | `dynamic_axes` 설정 | 배치 크기 가변 지원 (single/batch inference) |
| Provider | `CPUExecutionProvider` | Step 2는 CPU 최적화, GPU는 Step 3(TensorRT)에서 |
| 추론 엔진 추상화 | `Protocol` (typing) | PyTorch/ONNX 엔진이 동일 인터페이스 준수 (Strategy Pattern) |
| Backend 선택 | `config.inference_backend` | 환경변수로 런타임 전환 가능 (`INFERENCE_BACKEND=onnx`) |
| 변환 시점 | 빌드 타임 (별도 스크립트) | 서버 기동 시 변환하지 않음, 미리 변환된 `.onnx` 파일 사용 |
| 벤치마크 분리 | `@pytest.mark.slow` | 일반 CI에서 건너뛰기, 명시적 실행만 |
| L2 norm 허용치 | `1e-5` | float32 정밀도 한계, 실무 표준 허용 범위 |
| constant folding | `True` | ONNX 그래프 최적화로 BatchNorm 등 상수 연산 사전 계산 |

---

## 주요 파일 목록

| 파일 | 역할 |
|------|------|
| `ai-server/pyproject.toml` | 의존성에 `onnx`, `onnxruntime` 추가 |
| `ai-server/src/config.py` | ONNX 경로, inference_backend, opset_version 중앙 관리 |
| `ai-server/src/core/onnx_converter.py` | `convert_to_onnx()`: PyTorch → ONNX 변환 (multi-output, dynamic batch) |
| `ai-server/src/infra/onnx_inference.py` | `ONNXInferenceEngine`: ONNX Runtime 추론 + `InferenceEngine` Protocol |
| `ai-server/src/infra/model_loader.py` | 남녀 모델 독립 로드 (PyTorch/ONNX 듀얼 백엔드) |
| `ai-server/src/main.py` | inference_backend 설정에 따라 PyTorch/ONNX 선택 로드 |
| `ai-server/src/routes/health.py` | GET /health (status + models + inference_backend) |

---

## 커밋 계획 (Feature Branch: `feature/ai-server-onnx-optimization`)

| # | 타입 | 메시지 |
|---|------|--------|
| 1 | `chore` | `chore(ai-server): ONNX 의존성 추가 및 config 확장` |
| 2 | `test` | `test(ai-server): ONNX 파일 구조 검증 테스트 (L1 Red)` |
| 3 | `feat` | `feat(ai-server): PyTorch → ONNX 변환기 구현` |
| 4 | `test` | `test(ai-server): ONNX 출력 Shape 및 dynamic batch 검증 (L1 Red)` |
| 5 | `test` | `test(ai-server): InferenceSession 생성 및 provider 검증 (L1 Red)` |
| 6 | `feat` | `feat(ai-server): ONNX Runtime 추론 엔진 기본 구현` |
| 7 | `test` | `test(ai-server): PyTorch vs ONNX 동등성 검증 (L2 Red)` |
| 8 | `feat` | `feat(ai-server): ONNX 추론 엔진 predict() 구현` |
| 9 | `refactor` | `refactor(ai-server): InferenceEngine Protocol 및 model_loader 통합` |
| 10 | `test` | `test(ai-server): Latency 벤치마크 및 P95 회귀 테스트 (L3 Red)` |
| 11 | `feat` | `feat(ai-server): /health에 inference_backend 정보 추가` |

---

## 검증 방법

```bash
# 1. 의존성 설치 (ONNX 추가)
cd ai-server && pip install -e ".[dev]"

# 2. 기존 테스트 회귀 확인 (25개 모두 통과)
pytest tests/ -v --ignore=tests/test_e2e_real_image.py

# 3. ONNX 변환 테스트
pytest tests/test_onnx_conversion.py -v

# 4. ONNX 추론 테스트
pytest tests/test_onnx_inference.py -v

# 5. 벤치마크 테스트 (느린 테스트 포함, 결과 콘솔 출력)
pytest tests/test_onnx_benchmark.py -v -s

# 6. 전체 테스트 (벤치마크 제외)
pytest tests/ -v --ignore=tests/test_e2e_real_image.py -m "not slow"

# 7. 전체 테스트 (벤치마크 포함)
pytest tests/ -v --ignore=tests/test_e2e_real_image.py

# 8. 서버 로컬 실행 확인 (ONNX 백엔드)
INFERENCE_BACKEND=onnx uvicorn src.main:app --reload --port 8082
curl http://localhost:8082/health
```

## 주의 사항

- ONNX 변환 fixture는 `session` 스코프로 1회만 실행 (EfficientNet-B2 변환에 수 초 소요)
- Windows 경로는 `pathlib.Path` + `tempfile.TemporaryDirectory` 사용
- 벤치마크 테스트는 `@pytest.mark.slow`로 CI 기본 실행에서 분리
- `do_constant_folding=True`로 BatchNorm 등 상수 연산 사전 계산
- Step 3(TensorRT)에서 GPU 지원 추가 시 `onnxruntime-gpu`로 교체 예정
