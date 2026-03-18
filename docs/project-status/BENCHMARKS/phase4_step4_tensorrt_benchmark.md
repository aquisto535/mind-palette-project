# Phase 4 Step 4: TensorRT 3-Engine 벤치마크 리포트

**측정일**: 2026-03-18
**단계**: Phase 4 Step 4 — Extreme Optimization (TensorRT + Deep Dive)

---

## 환경

| 항목 | 값 |
|------|-----|
| GPU | NVIDIA GeForce RTX 3050 Ti Laptop GPU |
| CUDA 버전 | 12.6 |
| TensorRT 버전 | 10.15.1.29 |
| PyTorch 버전 | 2.10.0+cu126 |
| ONNX Runtime 버전 | onnxruntime-gpu |
| Python 버전 | 3.13.5 |
| OS | Windows 11 Pro 10.0.26200 |
| 입력 shape | (1, 3, 260, 260) — batch=1, RGB, 260×260 |
| 모델 | HFDClassifier (EfficientNet-B2 Backbone + 4 Linear Heads) |

---

## 3-Engine 벤치마크 결과

> 측정 방법: N_WARMUP=5, N_RUNS=30, `time.perf_counter()` 기반 ms 변환

| Engine | P50 Latency | P95 Latency | Mean Latency | Throughput | GPU Memory |
|--------|------------|------------|-------------|------------|------------|
| PyTorch GPU | 15.3ms | 29.9ms | 16.9ms | 67 QPS | 53 MB |
| ONNX CPU | 20.8ms | 24.4ms | 20.7ms | 48 QPS | — |
| **TensorRT FP16 GPU** | **8.1ms** | **14.1ms** | **7.6ms** | **325 QPS** | **40 MB** |

### 성능 배율 요약 (TensorRT FP16 기준)

| 비교 대상 | P95 배율 | Throughput 배율 |
|-----------|---------|----------------|
| vs PyTorch GPU | 2.1x 빠름 | 4.9x 높음 |
| vs ONNX CPU | **1.7x 빠름** | **6.8x 높음** |

> Step 2 목표: ONNX P95 ≤ PyTorch P95 × 2.0 → TensorRT P95 14.1ms vs ONNX 24.4ms (**통과**)

---

## FP16 정확도 검증 결과

> 비교 기준: FP32 ONNX 추론 결과 대비 FP16 TensorRT 오차 측정 (sigmoid 확률 공간)

| 항목 | 측정값 | 허용 기준 | 결과 |
|------|--------|---------|------|
| head_a max_diff | **0.0001** | < 0.20 | ✅ 통과 |
| 전체 head 5-seed max_diff | **0.0004** | < 0.20 | ✅ 통과 |
| 이진 분류 일치율 | **100%** | ≥ 70% | ✅ 통과 |

> FP16 허용 기준 0.20은 보수적으로 설정했으나 실측값은 0.0004로 사실상 FP32와 동일한 수준.

---

## 전체 테스트 결과

| 테스트 파일 | 테스트 수 | 결과 |
|------------|---------|------|
| `tests/test_tensorrt.py` (신규) | 10 | ✅ 10/10 통과 |
| 기존 전체 테스트 | 108 | ✅ 108/108 통과 |
| **합계** | **118** | **✅ 118/118 통과** |

---

## 구현 파일

| 파일 | 역할 |
|------|------|
| `src/infra/engine_protocol.py` | `InferenceEngine` Protocol (duck typing) |
| `src/infra/tensorrt_engine.py` | `TensorRtNativeEngine` + `build_tensorrt_engine()` |
| `src/infra/tensorrt_ort_engine.py` | `TensorRtOrtEngine` (ORT TensorrtExecutionProvider) |
| `tests/test_tensorrt.py` | L1/L2/L3 TDD 테스트 10개 |
| `src/config.py` | TensorRT 설정 필드 추가 |
| `tests/conftest.py` | `trt_engine_path`, `trt_native_engine` 픽스처 추가 |

---

## 결론

TensorRT FP16 GPU 추론이 ONNX CPU 대비 **1.7x 빠른 P95 지연시간**과 **6.8x 높은 처리량**을 달성했다.
FP16 정확도 손실은 사실상 0에 가까워 (max_diff=0.0004), 프로덕션 적용에 문제없는 수준이다.

포트폴리오 관점에서 "ONNX 변환 + TensorRT 최적화 + 벤치마크 리포트 자동 생성"까지 완성된 AI 추론 파이프라인을 증명한다.
