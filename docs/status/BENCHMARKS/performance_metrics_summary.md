# Mind Palette 성능 측정 수치 총람

> **목적**: 면접·포트폴리오에서 "왜 이 기술을 선택했는가?", "얼마나 빨라졌는가?"에 즉시 답변하기 위한 실측 수치 모음.
> **측정 방법**: W3C `Server-Timing` 헤더 기반 (`scripts/e2e_smoke_test.ps1 -ProfileMode`) + `time.perf_counter()` 기반 Python 벤치마크.
> **최종 업데이트**: 2026-04-09

---

## 🏆 핵심 수치 요약 (1분 답변용)

| 항목 | Before | After | 개선율 |
|------|--------|-------|-------|
| C++ 전처리 레이턴시 | 183ms | **97ms** | -47% |
| AI 추론 P95 (PyTorch → ONNX) | 39.9ms | **19.6ms** | -51% |
| AI 추론 P95 (ONNX → TensorRT) | 24.4ms | **14.1ms** | -42% |
| AI 최고 처리량 | 48 QPS | **325 QPS** | +577% |
| Python AI 메모리 (PyTorch → ONNX) | ~1,200MB | **~500MB** | -58% |
| C++ 멀티스레딩 처리량 | 1x | **2.1x** | +108% |
| TensorRT FP16 정확도 손실 | — | **0.0004** | 사실상 0 |

---

## 1. AI 추론 엔진 3-Way 벤치마크

> 환경: NVIDIA GeForce RTX 3050 Ti Laptop GPU, CUDA 12.6, TensorRT 10.15.1.29, Python 3.13.5
> 측정: N_WARMUP=5, N_RUNS=30, 입력 shape=(1, 3, 260, 260)
> 출처: `docs/status/BENCHMARKS/phase4_step4_tensorrt_benchmark.md`

| Engine | P50 | P95 | Mean | Throughput | GPU Memory |
|--------|-----|-----|------|-----------|-----------|
| PyTorch GPU | 15.3ms | 29.9ms | 16.9ms | 67 QPS | 53 MB |
| ONNX CPU | 20.8ms | 24.4ms | 20.7ms | 48 QPS | — |
| **TensorRT FP16 GPU** | **8.1ms** | **14.1ms** | **7.6ms** | **325 QPS** | **40 MB** |

**배율 요약**:

| 비교 | P95 배율 | Throughput 배율 |
|------|---------|----------------|
| TensorRT vs PyTorch GPU | **2.1x** 빠름 | **4.9x** 높음 |
| TensorRT vs ONNX CPU | **1.7x** 빠름 | **6.8x** 높음 |

**ONNX 변환 단독 효과** (opset=17, do_constant_folding=True):

| 엔진 | P95 | 배율 |
|------|-----|------|
| PyTorch CPU (기준) | 39.9ms | 1.0x |
| ONNX CPU | 19.6ms | **2.0x 향상** |

---

## 2. TensorRT FP16 정확도 검증

> 기준: FP32 ONNX 추론 결과 대비 sigmoid 확률 공간에서 오차 측정

| 항목 | 측정값 | 허용 기준 | 결과 |
|------|--------|---------|------|
| head_a max_diff | 0.0001 | < 0.20 | ✅ 통과 |
| 전체 head 5-seed max_diff | **0.0004** | < 0.20 | ✅ 통과 |
| 이진 분류 일치율 | **100%** | ≥ 70% | ✅ 통과 |

> 허용 기준을 보수적으로 0.20으로 잡았으나 실측값이 0.0004 — FP32와 사실상 동일.

---

## 3. C++ 전처리 파이프라인 레이턴시

> 출처: ADR-024 (FilterPipeline + Early Resize), ADR-028 (WorkerPool 튜닝)

### 3a. Early Resize 최적화 (ADR-024)

| 상태 | 레이턴시 | 목표 (< 100ms) |
|------|---------|--------------|
| 최적화 전 (원본 해상도) | 183ms | ❌ 초과 |
| **최적화 후 (768px 선제 리사이즈)** | **97ms** | **✅ 달성** |
| 개선율 | **47% 감소** | |

**병목 분석**:

| 필터 | 소요 시간 | 비고 |
|------|---------|------|
| AdaptiveThreshold | ~94ms | 최대 병목 |
| ROI 탐색 (findContours) | ~70ms | 2위 병목 |

### 3b. 멀티스레딩 파이프라인 속도 (Week 4)

| 파이프라인 | 처리 시간 | 목표 |
|----------|---------|------|
| 기본 전처리 (Resize/Denoise/Gray) | **25.49ms** | ✅ < 100ms |
| 스케치 파이프라인 (Canny/Morph/Invert) | **36.56ms** | ✅ < 100ms |

**멀티스레딩 성능 향상**: 8코어 환경에서 싱글 스레드 대비 **+108% (처리량 2.1배)**

### 3c. 콜드 스타트 분석 (ADR-028)

| 요청 순서 | CPP_Pre_Ms | 상태 |
|---------|-----------|------|
| 1번 (서버 재시작 직후) | ~530ms | 콜드 스타트 (수용) |
| 2번 | ~124ms | 정상 |
| 3번 | ~138ms | 정상 |

> 콜드 스타트 원인: OpenCV SIMD/TBB 초기화 + OS 페이지 매핑. HFD 검사 특성상 서버 재시작이 드물어 운영상 허용.

**기각된 최적화 시도** (`cv::setNumThreads(1)`):

| 측정 | Before | After (기각) |
|------|--------|-------------|
| 1번 (콜드) | 537ms | 585ms (+9%) |
| 2번 | 115ms | 190ms (+65%) |
| 평균 | 257ms | 340ms (+32%) |

---

## 4. E2E 파이프라인 구간 분해

> 측정: `e2e_smoke_test.ps1 -ProfileMode -Concurrency 3` + `Server-Timing` 헤더 파싱
> TrafficBot: 10건, 동시성 1, 실제 아동 인물화 이미지 사용

**구간별 소요 시간 (콜드 스타트 기준)**:

| 구간 | 소요 시간 | 전체 비율 |
|------|---------|---------|
| Total E2E | ~664ms | 100% |
| Gateway (Node.js) | ~607ms | ~91% |
| └── C++ Preprocess | ~530ms | **~80% (병목)** |
| Python AI 추론 | ~18ms | ~3% |

**TrafficBot 집계 지표**:

| 항목 | 수치 |
|------|------|
| 평균 응답시간 (avgResponseMs) | 214ms |
| P95 응답시간 | 841ms |
| 성공률 | 10/10 (100%) |

---

## 5. 모델 선택 비교 (ADR-010)

> 기준: 입력 512×512, ImageNet Pretrained, Transfer Learning 관점

| 모델 | 파라미터 | ImageNet Top-1 | 추론 속도 | VRAM | 판정 |
|------|--------|--------------|---------|------|------|
| **EfficientNet-B2** | **9.2M** | **80.1%** | **~50ms** | **~1.5GB** | **✅ 채택** |
| ConvNeXt-Tiny | 28.6M | 82.1% | ~80ms | — | ❌ 파라미터 3배 |
| ResNet-50 | 25.6M | 76.1% | ~40ms | — | ❌ 정확도 낮음 |
| ViT-B/16 | 86.6M | 77.9% | ~120ms | ~4GB+ | ❌ VRAM 초과 |

> EfficientNet-B2 = ResNet-50 파라미터의 **36%** 로 **4% 더 높은 정확도** (Compound Scaling 효과)

---

## 6. 서비스별 메모리 사용량 (c5.large 4GB 기준)

| 서비스 | 메모리 (PyTorch) | 메모리 (ONNX) |
|--------|----------------|-------------|
| Node.js API Gateway | ~150MB | ~150MB |
| C++ Preprocess Server | ~150MB | ~150MB |
| Python AI Server | ~1,200MB | **~500MB** |
| OS + 버퍼 | ~500MB | ~500MB |
| **합계** | **~2.0GB** | **~1.3GB / 4GB** |

> ONNX Runtime 전환으로 Python AI 메모리 **58% 절감** → c5.large 4GB 내 여유 운영 가능

---

## 7. EC2 인스턴스 비용 비교 (ADR-027)

| 인스턴스 | vCPU | RAM | 월 비용 | CPU 보장 | 판정 |
|---------|------|-----|--------|---------|------|
| t3.medium | 2 (버스터블) | 4GB | ~$30 | 20% 기준, 크레딧 소진 시 강제 제한 | ❌ 기각 |
| **c5.large** | **2 (고정)** | **4GB** | **~$62** | **100% 항시 보장** | **✅ 채택** |
| c5.xlarge | 4 (고정) | 8GB | ~$124 | 100% | 확장 경로 |
| g4dn.xlarge | 4 + T4 GPU | 16GB | ~$380 | GPU 포함 | TensorRT 배포 시 |

> t3.medium 기각 이유: CPU 크레딧 소진 시 EfficientNet 추론(~1~2s) + OpenCV 전처리(~115ms)가 0.4 vCPU로 직렬 처리되어 실사용 불가.

---

## 8. 테스트 통과 현황

| 단계 | 테스트 수 | 결과 |
|------|---------|------|
| Phase 4 TensorRT (test_tensorrt.py 신규) | 10 | ✅ 10/10 |
| Phase 4 전체 누적 | 118 | ✅ 118/118 |
| Phase 3 FilterPipeline 리팩터링 후 (CTest) | 91 | ✅ 91/91 |
| Phase 3 Week 4 멀티스레딩 (유닛 테스트) | 57 | ✅ 57/57 |

---

## 참고 문서

| 문서 | 내용 |
|------|------|
| `docs/status/BENCHMARKS/phase4_step4_tensorrt_benchmark.md` | TensorRT 3-Engine 상세 벤치마크 |
| `docs/architecture/ARCHITECTURE_DECISIONS.md` (ADR-024) | FilterPipeline + Early Resize 최적화 근거 |
| `docs/architecture/ARCHITECTURE_DECISIONS.md` (ADR-027) | EC2 인스턴스 선택 근거 및 메모리 분석 |
| `docs/architecture/ARCHITECTURE_DECISIONS.md` (ADR-028) | C++ WorkerPool 튜닝 + 콜드 스타트 실험 기록 |
| `docs/architecture/ARCHITECTURE_DECISIONS.md` (ADR-029) | TensorRT FP16 채택 근거 |
| `docs/status/development_progress.md` | 단계별 개발 이력 및 성능 수치 |
