# Phase 4 Step 4: TensorRT 구현 트러블슈팅

**발생일**: 2026-03-18
**단계**: Phase 4 Step 4 — Extreme Optimization (TensorRT + Deep Dive)
**환경**: Windows 11, Python 3.13.5, RTX 3050 Ti, CUDA 12.6, TensorRT 10.15.1.29

---

## 문제 1: ONNX external data 파일 탐색 실패

### 증상
```
RuntimeError: ONNX 파싱 실패: ['In node -1 with name: and operator: (parseGraph):
INVALID_GRAPH: Failed to import initializer: backbone.0.0.weight']
[TRT] [E] WeightsContext.cpp:190: Failed to open file: tmpkb8fij8r.onnx.data
```

### 원인
EfficientNet-B2 모델은 가중치 크기가 커서 `torch.onnx.export()` 호출 시 `.onnx.data` 파일을 별도로 생성한다.
기존 코드에서는 `parser.parse(raw)` 방식으로 파일 바이너리를 직접 읽어 TRT에 전달했는데,
이 경우 TRT가 external data 파일을 탐색할 기준 경로가 없어 로드에 실패한다.

추가로 `tempfile.NamedTemporaryFile()`로 생성한 `.onnx` 파일은 시스템 임시 디렉토리 루트에 위치하여
`.onnx.data`가 같은 디렉토리에 있음에도 TRT가 찾지 못하는 경우가 있다.

### 해결
**`tensorrt_engine.py`**: `parser.parse(raw)` → `parser.parse_from_file(onnx_path)` 교체

```python
# Before
with open(onnx_path, "rb") as f:
    raw = f.read()
if not parser.parse(raw):
    ...

# After
# parse_from_file()은 파일 경로 기준으로 external data(.onnx.data)를 자동 탐색
if not parser.parse_from_file(onnx_path):
    ...
```

**`conftest.py`**: `NamedTemporaryFile` → `tempfile.mkdtemp()` 전용 디렉토리 사용

```python
# Before
with tempfile.NamedTemporaryFile(suffix=".onnx", delete=False) as f:
    path = f.name
# ... yield path
# os.unlink(path)

# After
tmpdir = tempfile.mkdtemp()          # 전용 디렉토리: .onnx와 .onnx.data가 함께 위치
path = os.path.join(tmpdir, "model.onnx")
# ... yield path
# shutil.rmtree(tmpdir, ignore_errors=True)  # .onnx.data까지 함께 삭제
```

---

## 문제 2: TRT 엔진 빌드 실패 (Optimization Profile 미정의)

### 증상
```
[TRT] [E] IBuilder::buildSerializedNetwork: Error Code 4: API Usage Error
(Network has dynamic or shape inputs, but no optimization profile has been defined.)
```
`builder.build_serialized_network()` 반환값이 `None`

### 원인
`torch.onnx.export()`에서 `dynamic_axes={"input": {0: "batch_size"}}`를 사용하면
ONNX 모델의 배치 차원이 동적(dynamic)으로 설정된다.
TensorRT는 동적 입력이 있는 네트워크를 빌드할 때 **Optimization Profile**이 반드시 정의되어야 한다.
Optimization Profile 없이 빌드하면 TRT가 최적화 범위를 알 수 없어 빌드를 거부한다.

### 해결
`build_tensorrt_engine()`에 Optimization Profile 추가:

```python
_, c, h, w = input_shape
profile = builder.create_optimization_profile()
profile.set_shape(
    "input",
    min=(1, c, h, w),       # 최소 배치
    opt=(1, c, h, w),       # 최적 배치 (일반 추론)
    max=(max_batch_size, c, h, w),  # 최대 배치 (기본 8)
)
build_config.add_optimization_profile(profile)
```

`conftest.py`의 `build_tensorrt_engine` 호출에도 `input_shape` 인자 추가:

```python
build_tensorrt_engine(
    onnx_path=onnx_model_path,
    engine_path=engine_path,
    fp16=config.trt_fp16_enable,
    workspace_gb=config.trt_workspace_gb,
    input_shape=(1, config.input_channels, config.input_size, config.input_size),  # 추가
)
```

---

## 문제 3: FP16 TRT 추론 시 NaN 출력

### 증상
```python
FP16 NaN: True | Inf: False
head_a sample: [nan nan nan]
```
모든 출력이 NaN

### 원인
TRT FP16 엔진은 **I/O 바인딩을 float32로 받고 내부 레이어만 FP16으로 연산**한다.
기존 `run()` 구현에서 `image.astype(float16)`으로 입력을 float16으로 변환하고,
출력 버퍼도 `torch.float16`으로 할당하면서 데이터 타입 불일치가 발생했다.
EfficientNet-B2의 BatchNorm 통계값 등이 FP16 범위를 초과하여 NaN이 발생한다.

### 해결
`TensorRtNativeEngine.run()`에서 I/O dtype을 항상 float32로 고정:

```python
# Before
input_dtype = torch.float16 if self._fp16 else torch.float32
x_gpu = torch.from_numpy(image).to(dtype=input_dtype).cuda()
outputs_gpu = [torch.zeros(..., dtype=input_dtype, ...) for ...]

# After
# TRT FP16 엔진도 I/O 바인딩은 float32: 내부 레이어만 FP16으로 동작
x_gpu = torch.from_numpy(image).to(dtype=torch.float32).cuda()
outputs_gpu = [torch.zeros(..., dtype=torch.float32, ...) for ...]
```

---

## 문제 4: `execute_v2` 오류 (입력 shape 미등록)

### 증상
```
[TRT] [E] IExecutionContext::executeV2: Error Code 3: API Usage Error
(Parameter check failed, condition: inputDimensionSpecified && inputShapesSpecified.
Not all shapes are specified. Following input tensors' dimensions are not specified: input.)
```

### 원인
동적 배치 엔진(Optimization Profile 사용)은 `execute_v2()` 호출 전에
**실제 입력 shape를 ExecutionContext에 등록**해야 한다.
등록하지 않으면 TRT가 입력 크기를 알 수 없어 실행을 거부한다.

### 해결
`run()` 메서드에서 GPU 입력 생성 후 `set_input_shape()` 호출 추가:

```python
x_gpu = torch.from_numpy(image).to(dtype=torch.float32).cuda()

# 동적 배치 엔진: execute_v2 전에 실제 입력 shape를 컨텍스트에 등록해야 함
self._context.set_input_shape("input", image.shape)

outputs_gpu = [...]
bindings = [x_gpu.data_ptr()] + [o.data_ptr() for o in outputs_gpu]
self._context.execute_v2(bindings=bindings)
```

---

## 핵심 교훈

| 항목 | 규칙 |
|------|------|
| ONNX external data | `parse(raw)` 대신 `parse_from_file(path)` 사용 |
| 임시 파일 | `.onnx.data` 공존을 위해 `mkdtemp()` 전용 디렉토리 사용 |
| dynamic_axes | TRT 빌드 시 반드시 Optimization Profile 정의 (`set_shape()`) |
| FP16 I/O | TRT FP16 엔진의 입출력 dtype은 **항상 float32** (내부만 FP16) |
| 동적 배치 실행 | `execute_v2()` 전에 `context.set_input_shape("input", shape)` 필수 |
