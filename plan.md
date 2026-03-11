# 📋 Mind Palette 개발 계획 (TDD Checklist)

이 문서는 Antigravity와 함께 TDD 사이클을 관리하는 체크리스트입니다.
체크되지 않은(`[ ]`) 상단 항목부터 테스트 작성을 시작하여 점진적으로 완성해 나갑니다.

---

## 🛠️ 개발 방법론 (Methodology)
- **TDD Cycle**: Always [Red] → [Green] → [Refactor]
- **Tidy First**: 구조적 변경(Structural)과 기능적 변경(Behavioral)을 분리한다.
- **MCP Workflow**: [MCP_WORKFLOWS.md](file:///c:/Users/user/Documents/GitHub/mind-palette-project/docs/methodology/MCP_WORKFLOWS.md)에 따라 `shrimp`, `sequential-thinking`, `context7`을 유기적으로 활용한다.
- **Multi-Agent Strategy (ADR-019, ADR-022)**: 설계(Architect), 품질 및 보안(Guardian), 최적화(Optimizer) 3대 전문 서브 에이전트 체제를 통해 태스크를 격리하고 수행 품질을 극대화한다.

---

## 📊 현재 프로젝트 상태 (2026.03.10 기준)

> ⚡ **최우선 과제**: Phase 4(Python AI Server)가 미완성이면 프로젝트 전체가 "전처리 서비스"에 불과하며, AI 프로젝트라 부르기 어려움.

### 프로젝트 경쟁력 점수표
| 시점 | 점수 | 상태 | 비고 |
|------|------|------|------|
| **현재 (C++ 전처리만)** | 60점 | ⚠️ 위험 | "전처리 서비스"로만 평가됨 |
| **AI 서버 완성 시** | 85점 | ✅ 안전 | 채용 합격 가능 수준 |
| **ONNX + 벤치마크 추가** | 95점 | ⭐ 우수 | 경쟁력 있는 포트폴리오 |
| **파라미터 근거 문서화** | +5점 | 🎁 보너스 | 면접 신뢰도 상승 |

---

## 🧠 Phase 4: Python AI Server (진행 중: 2026.02 ~ 03)

### Step 1: Base Model (FastAPI + PyTorch)

> 테스트 깊이 분류: **L1**(데이터 구조/형태) → **L2**(변환 로직) → **L3**(제약/경계)

#### L1: 데이터 구조 (What) — "형태가 올바른가?"
- [x] **FastAPI 서버 구축**:
  - [x] [TDD][L1] `/health` 응답 구조 테스트: 200 OK, `{ "status": str, "model_loaded": bool }` 필드 존재 및 타입 (Red)
  - [x] FastAPI 기본 골격 및 헬스 체크 엔드포인트 구현 (Green)
- [x] **PyTorch 모델 구성 (EfficientNet-B2)**:
  - [x] **[MCP]** `context7`으로 EfficientNet-B2의 Feature Extractor 레이어 구조 조사
  - [x] [TDD][L1] 모델 아키텍처 구조 테스트: 가중치(`.pt`) 로드, Backbone 레이어 존재, Head 개수 == 4 검증 (Red)
  - [x] EfficientNet-B2 기반 Transfer Learning 모델 클래스 작성 (Green)
- [x] **Multi-head 분류 구조 구현**:
  - [x] [TDD][L1] 출력 텐서 Shape 테스트: 더미 입력 `(1,3,260,260)` → Head A `(1,19)`, Head B `(1,14)`, Head C `(1,16)`, Head D `(1,11)`, dtype=float32 (Red)
  - [x] Feature Extractor 동결 및 Multi-head Classifier(Linear Layers) 구현 (Green)
- [x] **3-Channel 입력 이미지 구조**:
  - [x] [TDD][L1] C++ 전처리 결과 이미지 구조 테스트: shape==(H,W,3), dtype==uint8, 채널 의미(R=gray, G=binary, B=distance) (Red)

#### L2: 변환 로직 (How) — "변환이 정확한가?"
- [x] **이미지 전처리 파이프라인**:
  - [x] [TDD][L2] Resize→Normalize→ToTensor 변환 테스트: 출력 shape==(1,3,260,260), 값 범위 [0,1] 검증 (Red)
- [x] **Feature Extractor 동결 검증**:
  - [x] [TDD][L2] backbone.parameters()의 requires_grad==False 검증 (Red)
- [x] **Multi-head 추론 정확성**:
  - [x] [TDD][L2] 고정 seed 입력 → 각 head sigmoid 출력 값이 모두 [0, 1] 범위 내 검증 (Red)
- [ ] **Multi-Channel 입력 최적화 (Ablation Study)**:
  - [ ] **[Deep Dive]** `R=binary, G=gray, B=distance_transform` 3채널 입력 방식 vs 단일 이진화 입력 방식의 분류 정확도 비교
  - [ ] 스케치 데이터셋 mean/std 산출 후 ImageNet 정규화 파라미터 대체 (`mean=[0.485,0.456,0.406]` → 스케치 기반 값으로 재계산)
  - [ ] 최적 채널 조합을 C++ 전처리 파이프라인에 반영 (Phase 3 Multi-Channel Merge 완성)
- [x] **Toy Model (MVP)**: ImageNet Pretrained EfficientNet-B2를 로드하여 더미 데이터 추론 성공.
- [x] **E2E 연동**:
  - [x] Node.js ↔ C++(전처리) ↔ Python(추론) 전체 파이프라인 통합 테스트 및 실시간 결과 반환 성공. (지원: AI Pipeline Architect)
  - [x] `multipart/form-data` 기반 실제 이미지 데이터 전송 및 최종 JSON 결과 연동 완료.

#### L3: 제약과 검증 (Why) — "경계에서도 안전한가?"
- [x] **비정상 입력 처리**:
  - [x] [TDD][L3] 손상된 파일, 0바이트 파일, 비이미지 파일 입력 시 400/422 반환 및 서버 무중단 테스트 (Red)
  - [x] `/analyze` 엔드포인트 구현 — 매직 바이트 + PIL.verify() 이미지 검증 (Green)
- [x] **모델 미로드 상태**:
  - [x] [TDD][L3] 모델 파일 경로 오류 시 /health에서 model_loaded==false, 서버 기동 유지 테스트 (Red)
- [ ] **GPU 메모리 고갈**:
  - [ ] [TDD][L3] OOM 시뮬레이션 시 503 Service Unavailable 반환 및 로그 기록 테스트 (Red)
- [x] **정규화 파라미터 불변식**:
  - [x] [TDD][L3] mean/std 값이 config에서 로드되는지 검증, 하드코딩 방지 테스트 (Red)

### Step 2: Universal Optimization (ONNX + Deep Dive)

#### L1: 데이터 구조 (What)
- [x] **[MCP]** `context7`으로 PyTorch 모델의 ONNX 변환 시 지원되는 최신 Ops 및 호환성 리서치
  - 결론: opset=17(A+), dynamo=False(최안정), do_constant_folding=True(BN folding ~15% 속도 향상), 현재 설정 최적
- [x] **ONNX 모델 파일 구조**:
  - [x] [TDD][L1] 변환된 .onnx 파일의 입력 노드 shape==(1,3,260,260), 출력 노드 개수==4 검증 (Red)
  - [x] OnnxConverter (`src/core/onnx_converter.py`) 구현 (Green)
- [x] **ONNX Runtime 세션**:
  - [x] [TDD][L1] InferenceSession 객체 생성 성공 및 provider 확인 테스트 (Red)
  - [x] OnnxInferenceEngine (`src/infra/onnx_inference.py`) 구현 (Green)

#### L2: 변환 로직 (How)
- [x] **ONNX 변환 동등성**:
  - [x] [TDD][L2] 동일 입력에 대해 PyTorch vs ONNX Runtime 추론 결과 차이(max abs err) < 1e-4 검증 (Red)
  - [x] 모델 변환 및 ONNX Runtime 추론 엔진 구현 (Green)
- [x] **[Deep Dive] Latency Analysis**: PyTorch P95=39.9ms vs ONNX P95=19.6ms — ONNX가 ~2x 빠름 (CPU 기준).
- [x] **ONNX Runtime 교체**: OnnxInferenceEngine 구현 완료, 속도 향상 검증.

#### L3: 제약과 검증 (Why)
- [x] **추론 지연시간 회귀**:
  - [x] [TDD][L3] P95 latency가 PyTorch 대비 2배 이하인지 벤치마크 — ONNX P95 19.6ms (통과)

### Step 3: Extreme Optimization (TensorRT + Deep Dive)

#### L1: 데이터 구조 (What)
- [ ] **[MCP]** `sequential-thinking`을 사용하여 TensorRT 엔진 빌드 시 FP16 양자화에 따른 정확도 손실 분석 (제1원칙)
- [ ] **TensorRT 엔진 파일**:
  - [ ] [TDD][L1] .engine 파일 로드 성공 및 GPU 메모리 할당 확인 테스트 (Red)

#### L2: 변환 로직 (How)
- [ ] **FP16 양자화 정확도**:
  - [ ] [TDD][L2] FP32 원본 vs FP16 양자화 정확도 차이 < 1%p 이내 검증 (Red)
  - [ ] ONNX ↔ TensorRT 변환 및 Quantization(FP16) 적용 (Green)

#### L3: 제약과 검증 (Why)
- [ ] **3-Engine 최종 벤치마크**:
  - [ ] [TDD][L3] PyTorch vs ONNX vs TensorRT: Latency, Throughput, Memory 3축 비교 리포트 자동 생성 (Red)

---

## ⚙️ Phase 3 잔여 실행 항목 (C++ 전처리 고도화)

> 배경 반전·Multi-Object Crop은 즉시 적용 가능. 나머지는 Phase 4 완성 이후 ROI 최적.

### 🔥 [확정] Production-Level Preprocessing (우선순위 최상)
- [x] **Advanced Pipeline 리팩터링**: `ImageProcessor::Preprocess`를 제안된 5단계 로직으로 전면 교체
  - [x] **Step 1 (Denoise)**: `GaussianBlur(5x5)`로 종이 질감 제거 및 필압 보존 Grayscale 생성
  - [x] **Step 2 (Adaptive Binary)**: `AdaptiveThreshold(11, 2)` + `Morphology(Close)`로 선 연결성 강화
  - [x] **Step 3 (Smart ROI)**: `0.1% Area` 이상의 모든 객체를 포함하는 `Union Rect` 계산 (신발/부속물 누락 방지)
  - [x] **Step 4 (Letterbox)**: 비율 왜곡 없이 512x512 중앙 배치 (Padding 추가)

- [x] **3-Channel Hybrid Strategy 구현 (AI 입력 최적화)**
  - [x] **Channel Construction**: 단순 RGB 변환이 아닌, 채널별 의미 부여
    - **R (Gray)**: 원본 명암 유지 (필압/실체감 분석용)
    - **G (Inverted Binary)**: 흰 배경 검은 선 (형태/윤곽선 분석용)
    - **B (Distance/Clone)**: `DistanceTransform` 또는 G채널 복제 (선의 골격 강조)
  - [x] **Domain Adaptation**: 최종 결과물을 **White Background**로 통일하여 ImageNet Pretrained 모델 친화적 데이터 생성

### Phase 4 이후 실행 (ROI 순)
- [ ] **파라미터 근거 문서화**: 각 필터 파라미터를 최소 3가지 값으로 비교 실험하고, 선택 근거를 주석/ADR에 기록 (1주, ROI 높음)
- [ ] **보간법 최적화**: `cv::resize`에서 축소 시 `INTER_AREA`, 확대 시 `INTER_CUBIC` 적용 (반나절, ROI 높음)
- [ ] **Otsu 기반 자동 Threshold**: Canny threshold를 이미지 통계 기반으로 자동 결정하는 로직 도입 (1~2일, ROI 높음)
- [ ] **CLAHE 히스토그램 평활화 추가**: 조명 불균일 대응 필터 추가 (1일, ROI 중간)
- [ ] **PSNR/SSIM 품질 메트릭 도입**: 전처리 전후 품질을 정량 비교하는 유틸리티 (2일, ROI 중간)
- [ ] **`cv::fastNlMeansDenoising` 적용**: 엣지 보존 노이즈 제거로 DenoiseFilter 고도화 (1일, ROI 중간)

### Phase 4 연계 항목 (AI 서버와 함께)
- [ ] **필압 분석(히스토그램)**: R채널(Gray)의 픽셀 분포를 분석하여 AI Feature와 별도로 필압 점수 산출
- [ ] **Hybrid Input Normalization**:
  - [ ] 기존 ImageNet Mean/Std (`[0.485, ...]`) 사용 불가
  - [ ] **Dataset Statistics**: 구축된 3채널(Gray/Binary/Dist) 데이터셋 전체의 Mean/Std를 새로 계산하여 정규화 파라미터 갱신 필수
- [ ] **Channel Dropout Augmentation**: 학습 시 R, G, B 채널 중 하나를 랜덤하게 0으로 만들어, 특정 정보(예: 필압)가 없어도 형태만으로 맞추거나 그 반대가 가능하도록 강건성 확보
- [ ] **선 떨림 보정(Contour Moment)**: AI 특징 추출과 통합하여 Phase 4에서 구현
- [ ] **하이브리드 결과 결합**: C++ 기하학적 특징 + AI 추론 결과 → Phase 4 연동 시 설계

---

## 🌐 Phase 5: 통합 및 고도화 (배포 전략)

### 성능 최적화 (Performance Optimization)
- [ ] **Hash-based Caching (지연 시간 해결 - ADR-020 대응)**:
  - [ ] [TDD] 동일 이미지 업로드 시 보안 검증 및 분석 단계를 건너뛰고 결과 즉시 반환 테스트 (Red)
  - [ ] SHA-256 해시 기반 "Security-Verified Cache" 레이어 구현 (Green)
  - [ ] **성능 목표**: 캐시 적중 시 지연 시간 < 10ms 달성
- [ ] **Inference Optimization**: Python AI 서버의 추론 엔진 최종 ONNX/TensorRT 통합 및 회귀 테스트.

### 배포 아키텍처 및 보안 (Architecture & Security)
- [ ] **Nginx Reverse Proxy 도입**: AWS EC2 앞단에 Nginx를 배치하여 SSL 인증서(Let's Encrypt) 관리 및 HTTPS 트래픽 처리.
- [ ] **Mixed Content 방지**: Frontend(HTTPS) ↔ API Gateway(HTTPS) 간 보안 통신 구현.
- [ ] **Internal Private Network**: API Gateway ↔ C++ ↔ Python 구간은 내부망 HTTP 통신(Plain Text) 유지하여 성능 최적화 (SSL 오버헤드 제거).
- [ ] **Docker Compose 프로덕션 설정**: `restart: always`, 로깅 드라이버, 볼륨 백업 정책 적용.

---

## 📊 Cross-Cutting Concerns

### 🪵 Logging System
> 목표: 모든 서비스에 구조화된 로깅(Structured Logging)을 도입하여, 장애 추적 및 성능 분석을 가능하게 한다.

#### Node.js (API Gateway) - Winston ✅ 완료
- [x] **Winston 도입**: JSON 포맷, 파일/콘솔 동시 출력, 로그 레벨(DEBUG/INFO/WARN/ERROR) 설정.
- [x] **요청/응답 로깅**: 모든 API 요청의 메타데이터(타임스탬프, 파일명, 크기)를 INFO 레벨로 기록.
- [x] **에러 스택 추적**: 예외 발생 시 전체 스택 트레이스를 ERROR 레벨로 기록.

#### C++ (Preprocess Server) - spdlog ✅ 완료
- [x] **spdlog 도입**: vcpkg로 설치하고, 멀티스레드 안전(thread-safe) 로깅.
- [x] **성능 로깅**: 전처리 소요 시간을 밀리초(ms) 단위로 측정하여 기록.
- [x] **파일 회전(Rotation)**: 로그 파일이 10MB를 초과하면 자동으로 새 파일로 교체.

#### Python (AI Server) - structlog ✅ 완료
- [x] **structlog 도입**: JSON 포맷, `request_id` 바인딩 및 파일 로깅(`logs/`) 연동 완료. (지원: AI Pipeline Architect)
- [x] **추론 추적**: 모델 입력 정보 및 가공된 분석 결과 기록 로직 구현.
- [x] **컨텍스트 바인딩**: `X-Request-ID`를 로그에 자동 추가하여 전 구간 추적 가능.

#### 통합 (Cross-Service Integration) ✅ 완료
- [x] **Request ID 전파**: Node.js에서 생성한 UUID가 C++, Python 서버 로그에 일관되게 전파 및 기록됨을 검증 완료. (지원: AI Pipeline Architect)
- [ ] **에러 알림 시스템(선택)**: CRITICAL 레벨 로그 발생 시 Slack/Email 알림 메커니즘 구축.

### 🏥 System Reliability
> 목표: 시스템이 24/7 안정적으로 동작하고, 장애 발생 시 즉시 감지할 수 있는 기반을 구축한다.

#### Health Checks - Tier 1: 필수
- [x] **C++**: `/health` 엔드포인트 구현 완료.
- [x] **Node.js**: `/health` 엔드포인트 구현 완료.
- [x] **Python**:
  - [x] 모델 로드 상태, Uptime, 시스템 리소스(CPU/Mem) 확인 헬스 체크 구현 완료. (지원: AI Pipeline Architect)
  - [ ] [TDD] GPU 메모리 고갈 시 503 Service Unavailable 반환 테스트 (Red) — Phase 5 이후
- [ ] **Docker Healthcheck**:
  - [ ] [TDD] 컨테이너 비정상 종료 시 Docker Daemon의 재시작 정책 동작 테스트 (Red)
  - [ ] `docker-compose.yml` 내 healthcheck (interval, timeout) 설정 (Green)

#### API Documentation - Tier 2: 권장
- [x] **[MCP]** `context7`으로 OpenAPI 3.0 스펙의 가독성 좋은 문서화 패턴 리서치
- [x] **Node.js API 명세**:
  - [x] [TDD] API 명세 파일이 실제 엔드포인트 구조와 일치하는지 자동 검증 테스트 (Red)
  - [x] OpenAPI 3.0/Swagger Spec 작성 및 저장 (Green)

### 🔐 Security
> 목표: 입력/저장/전송/의존성 전 구간에서 최소한의 보안 기준을 충족한다.

#### 입력 검증 (Input Validation) - Tier 1: 필수
- [x] **[MCP]** `context7`으로 이미지 파일 매직 바이트를 활용한 완벽한 확장자 위조 탐지 기법 리서치 — 결론: 6-Layer 검증 모델(확장자→매직바이트→MIME→라이브러리파싱→크기/해상도→재인코딩) 권장. Node.js(빠른 1차 필터)+Python(PIL.verify 심층검증) 책임 분리 아키텍처는 L4·L6 기준 충족. PNG 시그니처는 4바이트(현재)→8바이트 강화 권장
- [x] **파일 업로드 검증**:
  - [x] [TDD] .txt 파일을 .jpg로 속여 업로드 시 차단되는지 테스트 (Red)
  - [x] MIME 타입/매직 바이트 기반 물리적 검증 로직 구현 (Green) — `fileStorage.ts::hasValidMagicBytes()`, `analyze.ts`에서 적용
- [ ] **검증 파이프라인 최적화 (Latency Strategy)**:
  - [ ] **Parallel Validation**: L2(Magic Byte)와 L5(Resource Limit)를 병렬로 체크하여 블로킹 시간 단축.
  - [ ] **Deferred Sanitization**: L6(재인코딩) 과정을 전처리 서버로 이관하여 API 응답 경로에서 분리(Async) 고려.
- [x] **경로 정규화**:
  - [x] [TDD] `../../etc/passwd`와 같은 Path Traversal 공격 시 차단 테스트 (Red)
  - [x] 입력 경로 정규화 및 화이트리스트 디렉토리 체크 구현 (Green) — `fileStorage.ts::isSafeFilename()` (null byte, 절대경로, `..` 차단)

#### 저장/무결성 (Storage & Integrity) - Tier 1: 필수
- [x] **Atomic Write & Atomic Delete**:
  - [x] [TDD] 저장/삭제 중 예상치 못한 중단 시 데이터 불일치 여부 테스트 (Red)
  - [x] `.tmp` → `rename` 패턴 및 원자적 삭제 로직 보완 (Green) — `AtomicFileWriter::atomicDelete()` (rename→.del→remove 패턴)
- [x] **해시 무결성**:
  - [x] [TDD] 결과 파일 변조 시 캐시 매칭 실패 및 재분석 트리거 테스트 (Red)
  - [x] SHA-256 해시 저장 및 무결성 검증 자동화 (Green) — `hashIntegrity.ts::saveWithHash()/verifyHash()`, `analysisService.ts` 연동

#### 전송 보안 (Transport Security) - Tier 2: 권장
- [ ] **외부 HTTPS**: Frontend ↔ API Gateway는 HTTPS를 사용해야 한다. _(Phase 5 배포 시 Nginx TLS 설정으로 처리 — 애플리케이션 코드 변경 없음)_
- [x] **내부망 격리**: API Gateway ↔ C++ ↔ Python 통신은 내부망 HTTP로 제한해야 한다.

#### 로깅 보안 (Logging Hygiene) - Tier 2: 권장
- [x] **PII 마스킹**: 이름/생년월일 등 개인정보는 로그에 남기지 않거나 마스킹해야 한다.
- [x] **에러 메시지 최소화**: 내부 경로/스택 노출을 최소화해야 한다.

#### 의존성/공급망 보안 (Dependency Security) - Managed by TDD Quality Guardian
- [x] **정기 점검**: `npm audit`와 CodeQL을 주기에 맞춰 실행하고 결과를 분석한다. (TDD Quality Guardian 전담)
- [x] **버전 고정**: vcpkg baseline pinning을 유지하여 빌드 재현성을 확보해야 한다.

### 🧪 Traffic & Load Testing
> 목표: 서비스의 안정성을 검증하고, 대량의 로그를 생성하여 시스템의 한계를 테스트한다.

#### Traffic Generation - Phase 3~4 (검증용)
- [ ] **[MCP]** `sequential-thinking`을 사용한 대량 로그 발생 시 파일 I/O 병목 및 시스템 영향도 분석
- [ ] **Node.js Traffic Bot**:
  - [ ] [TDD] 봇 가동 시 로그 파일 크기 증가 및 로테이션 발동 여부 테스트 (Red)
  - [ ] `axios` 기반 주기적 요청 자동화 스크립트 작성 (Green)

#### Load Testing - Phase 5 (최종 성능)
- [ ] **k6 부하 테스트**:
  - [ ] [TDT] 동시 접속자 100명 달성 시 응답 지연(P95) 기준 미달 시 실패 처리 (Red)
  - [ ] k6 시나리오 작성 및 결과 벤치마크 리포트 생성 (Green)

---

## 🎓 Phase 6: System Reliability & Chaos Engineering (Apr ~ Aug)
> **Goal**: 개별 모듈의 Deep Dive(Phase 3,4)가 끝난 후, 전체 시스템 차원의 안정성과 복구 능력을 검증합니다.

### 🛡️ Reliability & Resilience (복구 탄력성)
- [ ] **[MCP]** `sequential-thinking`을 사용하여 특정 서비스 지연이 전체 시스템의 '연쇄적 중단(Cascading Failure)'을 일으키는지 분석
- [ ] **장애 복구(Failover) 시나리오**:
  - [ ] [TDD] C++ 서버 가용 불능 시 Node.js Gateway의 Circuit Breaker 오픈 및 대체 메시지 반환 테스트 (Red)
  - [ ] 재시도(Retry) 및 서킷 브레이커 로직 구현 (Green)
- [ ] **Chaos Testing**:
  - [ ] [TDD] 런타임에 임의의 서비스 강제 종료 시 데이터 유실 없이 자동 복구 테스트 (Red)
  - [ ] Docker Compose 자동 복구(restart) 및 상태 전파 확인 (Green)

---

## ✅ 완료된 Phase (참고용)

### 🎨 Phase 1: Frontend (React) 통합

#### 업로드 컴포넌트 연동
- [x] `handleUpload` 함수가 실제 API 엔드포인트로 `FormData`를 전송해야 한다 (Mocking).
- [x] API 응답 성공 시 결과 페이지(`setStep('result')`)로 전환되어야 한다.
- [x] API 응답 실패 시 에러 메시지를 처리해야 한다.

### 🚀 Phase 2: API Gateway (Node.js) 개발

#### 기본 서버 설정 및 상태 확인
- [x] `GET /` 요청 시 200 OK와 함께 서버 상태 메시지를 반환해야 한다.

#### 이미지 분석 API (`POST /analyze`)
- [x] 이미지 파일 없이 요청 시 400 Bad Request 에러를 반환해야 한다.
- [x] 유효한 이미지 파일 업로드 시 200 OK와 분석 결과 JSON을 반환해야 한다.
- [x] 업로드된 이미지가 `shared_volume/uploads` 폴더에 실제로 저장되어야 한다.
- [x] 분석 결과가 `shared_volume/results` 폴더에 JSON 파일로 저장되어야 한다.

#### 리팩터링 (Refactoring)
- [x] (Refactor) `server.js`의 비즈니스 로직을 별도 모듈로 분리해야 한다.

### 🛡️ 품질 및 보안 보증 (QA)

#### CI/CD 파이프라인 및 보안 분석
- [x] GitHub Actions 워크플로우(`.github/workflows/main.yml`)가 정상적으로 동작해야 한다.
- [x] Backend 및 Frontend 단위 테스트가 CI에서 자동 실행되어야 한다.
- [x] 통합 테스트(Integration Test)가 CI에서 자동 실행되어야 한다.
- [x] **CodeQL (Security Analysis)**: JavaScript/TypeScript 및 C++ 코드의 보안 취약점 분석이 CI에 포함되어야 한다.

### ⚙️ Phase 3: C++ Preprocessing Server (완료)
> 목표(계획서 기준): **Crow + OpenCV** 기반 전처리 마이크로서비스를 구축하고, **전처리 속도 < 100ms**를 목표로 멀티스레딩 최적화 및 **정적 분석/테스트(QA)** 를 적용한다.

#### Week 1: REST API 기본 골격 (Crow)
- [x] `GET /` 요청 시 200 OK와 함께 서버 상태 메시지를 반환해야 한다.
- [x] `GET /health` 요청 시 200 OK와 "OK"를 반환해야 한다.
- [x] **통신 계약(초기, 파일 경로 공유)**: `Node.js ↔ C++`는 `{ "imagePath": "/shared/uploads/img.jpg" }` → `{ "processedPath": "/shared/processed/img_clean.jpg" }` JSON으로 주고받아야 한다.
- [x] **Node.js ↔ C++ 통신 테스트**: API Gateway가 C++ 전처리 엔드포인트를 호출해 `processedPath`를 받을 수 있어야 한다.

#### Week 2: OpenCV 전처리(기본) + API
- [x] **OpenCV 도입(vcpkg + CMake)**: 전처리 모듈을 빌드에 포함해야 한다.
- [x] `POST /preprocess` 요청 시 `imagePath`가 없거나 빈 값이면 400 Bad Request를 반환해야 한다.
- [x] `POST /preprocess` 요청 시 존재하지 않는 파일 경로면 404 Not Found를 반환해야 한다.
- [x] `POST /preprocess` 요청 시 유효한 이미지 경로면 200 OK와 `processedPath`를 반환해야 한다.
- [x] **크기 정규화**: 입력 이미지는 512x512로 리사이즈되어야 한다.
- [x] **노이즈 제거**: GaussianBlur + medianBlur가 적용되어야 한다.
- [x] **그레이스케일 변환**: 후속 에지 검출을 위한 grayscale 이미지가 생성되어야 한다.

#### Week 3: 에지/배경 제거 고도화 (Advanced OpenCV + Deep Dive) ✅
- [x] **[MCP]** `sequential-thinking`을 사용하여 GrabCut vs DL 기반 배경 제거의 효율성 분석 (제1원칙)
- [x] **GrabCut 배경 제거 & 실험**:
  - [x] [TDD] `image_processor_test.cpp`: GrabCut 초기 마스크 생성 및 유효성 검증 테스트 (Red)
  - [x] `image_processor.cpp`: GrabCut 알고리즘 기본 구현 (Green)
  - [x] **[Deep Dive] Optimization**: `iterCount`(1회 vs 5회)에 따른 수행 시간(ms)과 품질 차이를 주석으로 기록.
  - [x] ⚠️ **의사결정**: 성능 4.2초 → 도메인 부적합 → 파이프라인 제외, 테스트 유지 (ADR-011)
- [x] **Canny 에지 검출**:
  - [x] **[MCP]** `context7`으로 Canny 알고리즘의 최신 최적화 파라미터 조사
  - [x] [TDD] Canny Threshold(low/high) 변화에 따른 엣지 검출 정량적 정확도 테스트 (Red)
  - [x] Canny 에지 검출 로직 구현 (Green)
- [x] **윤곽선 강화**: 모폴로지 연산(MORPH_CLOSE) 적용 및 테스트 완료.
- [x] **이진화**: Adaptive Threshold 적용 및 테스트 완료.
- [x] **RGB 변환**: Binarized 이미지 → RGB 3채널 변환 (EfficientNet-B2 호환)
- [x] **ADR-011 작성**: C++ 전처리 파이프라인 결과물 명세 문서화

#### Week 3.5: 디자인 패턴 적용 및 아키텍처 리팩터링 (Architecture & Scalability)
- [x] **[MCP]** `context7`으로 Modern C++(C++17)에서의 Strategy Pattern 및 Factory Pattern 최적 구현 사례 리서치
- [x] **Strategy Pattern (Filter System)**:
  - [x] [Refactor] 기존 `if-else` 기반 필터 로직을 `IFilter` 인터페이스 및 구체 클래스(`BlurFilter`, `CannyFilter`)로 분리.
  - [x] [TDD] 새로운 필터 추가 시 기존 코드 수정 없이 확장 가능한지 검증하는 테스트 (OCP 준수 확인).
- [x] **Pipeline Composite Pattern**:
  - [x] [TDD] 여러 필터를 순차적으로 적용하는 `FilterPipeline` 클래스 구현 (Red).
  - [x] 동적으로 필터 순서를 조합(예: `Resize` -> `Blur` -> `Canny`)하여 실행하는 로직 구현 (Green).
- [x] **Producer-Consumer Pattern (Preparation)**:
  - [x] Week 4 멀티스레딩을 위한 `TaskQueue` 인터페이스 설계 및 단일 스레드 기반 모의 구현.

#### Week 4: 멀티스레딩/성능/품질 (Concurrency Deep Dive)
- [x] **Thread Pool 구현 (std::thread)**:
  - [x] [TDD] 스레드 풀 작업 큐의 동기화 및 데드락 방지 단위 테스트 (Red)
  - [x] `std::thread`/`mutex`/`condition_variable` 기반 표준 스레드 풀 구현 (Green)
  - [x] **[Deep Dive] Scalability Test**: 스레드 개수(1 vs 4 vs 8)에 따른 처리량(Throughput) 비교 벤치마크 수행.
- [x] **배치 처리**: 병렬 작업 분할 로직에 대한 데이터 레이스 검증 테스트.
- [x] **성능 벤치마크**: [TDT] 전처리 1건 처리 시간 < 100ms 자동 회귀 테스트 구축.
- [x] **Atomic Write & Safety**:
  - [x] [TDD] 저장 중 프로세스 종료 시 Corrupted 파일 잔존 여부 테스트 (Red)
  - [x] `.tmp` → `rename` 패턴 적용으로 원자성(Atomicity) 보장 (Green)
- [x] **[MCP]** MSVC Code Analysis 및 Core Guidelines 위반 사항 `sequential-thinking`분석
- [x] **Quality Gates**: CI 파이프라인에 정적 분석 통합 및 통과 확인.
- [x] **GoogleTest (GTest)**: 전체 알고리즘에 대한 경계값(Edge Case) 및 회귀 테스트 완료.

#### Phase 3 회고 (최종): C++ 전처리 모듈 기술 수준 자가 진단 (Updated)
> **평가 기준**: 영상처리/CV 엔지니어가 포트폴리오를 리뷰한다고 가정
> **종합 등급**: ⭐⭐⭐⭐☆ (4.0/5.0) — "Data-Centric AI를 위한 Smart Pipeline & Feature Engineering"

**✅ 강화된 강점 (Solved Problems)**
| 측면 | 수준 | 요약 |
|------|------|------|
| **Data-Centric Engineering** | ⭐⭐⭐⭐⭐ | 단순 이미지 처리가 아닌, **AI 학습 효율을 위한 3-Channel Hybrid Strategy** (Pressure/Shape/Structure) 설계 |
| **Smart ROI Algorithm** | ⭐⭐⭐⭐☆ | Morphology + Contour Analysis + Union Logic을 결합하여 노이즈에 강건한 객체 추출 구현 |
| **서비스 구조화** | ⭐⭐⭐⭐⭐ | HTTP 서버 + 파이프라인 + 벤치마크 + Atomic Write + **Test-Driven Visualization** |
| **Modern C++17** | ⭐⭐⭐⭐☆ | `unique_ptr`, `move semantics`, `std::optional` 활용 |

**⚠️ 잔여 약점 및 과제 (Backlog)**
| 약점 | 설명 | 면접 리스크 및 대응 |
|------|------|---------------------|
| **Manual Tuning** | `AdaptiveThreshold(11, 2)`, `Blur(5x5)` 등이 실험적(Heuristic) 값임 | "데이터셋 분포에 따른 Auto-Tuning 로직이 왜 없는가?" (Phase 4에서 통계 기반 보완 필요) |
| **정량적 품질 지표** | 변환 결과가 "AI에 얼마나 좋은지" 수치화 부족 | "mAP/Accuracy 향상 폭을 제시하라" (AI 학습 후 성능 비교로 증명 예정) |

---

### 🎯 커리어 포지셔닝 전략
> 이 프로젝트를 통해 어필할 포지션과 어필하지 말아야 할 포지션을 구분한다.

#### ✅ 어필 포지션: "AI 파이프라인 / 시스템 엔지니어"
- **핵심 어필**: C++ 시스템 프로그래밍 배경으로, 3개 언어(C++/Node.js/Python) 마이크로서비스를 Docker 기반으로 통합한 엔드투엔드 AI 시스템 설계·구현·배포 역량
- **차별점**: 알고리즘 하나에 국한되지 않고 전체 시스템을 혼자 만들 수 있는 희소 역량
- **증거**: 15개 ADR, GTest/Jest/PyTest 테스트, CI/CD, Atomic Write 등 엔지니어링 프랙티스

**적합한 포지션 (경쟁력 높음)**
| 포지션 | 경쟁력 | 이유 |
|--------|--------|------|
| C++ 영상 파이프라인 개발 | ⭐⭐⭐⭐☆ | Crow + OpenCV + ThreadPool + Pipeline 패턴 |
| MLOps / AI 인프라 | ⭐⭐⭐⭐☆ | Docker + 마이크로서비스 + CI/CD + ONNX 변환 |
| 풀스택 AI 서비스 개발 | ⭐⭐⭐⭐⭐ | 한 사람이 전체 시스템 설계-구현-배포 = 희소 역량 |

#### ❌ 피해야 할 포지셔닝: "영상처리 전문가"
- SIFT/ORB, 주파수 분석, 카메라 캘리브레이션 등 전문 지식 부족
- 석박사 CV 연구자와 직접 경쟁하게 되어 불리
- "OpenCV 필터 8종 구현"은 차별화 불가 (누구나 가능)

**면접 예상 질문 대비**
| 질문 | 강한 답변 방향 | 약한 답변 (회피) |
|------|---------------|-----------------|
| "왜 C++로 전처리를 했나?" | CPU 캐시 + SIMD 최적화 + GIL 없는 멀티스레딩 → Python 대비 성능 우위 | "C++을 잘해서요" |
| "OpenCV 어느 수준까지?" | 기본 파이프라인 구축 + 디자인 패턴 적용 + 서비스화 경험 | 고급 알고리즘 전문가인 척 |
| "왜 이 아키텍처?" | ADR 15개의 의사결정 근거 (규모에 맞는 판단력) | "트렌드라서요" |
| "AI 모델 경험은?" | EfficientNet-B2 Transfer Learning + ONNX 최적화 벤치마크 | 모델 자체 설계 경험 과장 |

### 📅 4개월 실행 로드맵 (2026.02 ~ 05)
> **진단 기준일**: 2026년 2월 18일 | **목표**: 85-95점 포트폴리오 완성

| 기간 | 주력 목표 | 복습 목표 | 마일스톤 |
|------|-----------|-----------|----------|
| **2월 말** | FastAPI 서버 완성 + PyTorch 환경 구축 | GaussianBlur 파라미터 실험 1회 | Phase 4 Step 1 착수 |
| **3월** | EfficientNet-B2 모델 구현 + 학습 파이프라인 | Canny Threshold 3가지 값 비교 실험 | AI 서버 기본 추론 성공 |
| **4월** | ONNX 변환 + 벤치마크 + E2E 파이프라인 연동 | 파라미터 실험 결과 ADR 문서화 | 전체 파이프라인 작동 증명 |
| **5월** | 데모 영상 제작 + README 정리 + 면접 준비 | 아키텍처 다이어그램 1장 완성 | 포트폴리오 완성 (85-95점) |

### 📌 핵심 원칙 (Kent Beck)
```
"Make it work → Make it right → Make it fast"

현재 위치: "Make it work" ✅ (Phase 1-3 완료)
다음 단계: Phase 4 완성 → "Make it right" (파라미터 실험/문서화)
최종 목표: "Make it fast" (ONNX/TensorRT 최적화)

핵심: 작동하는 불완전한 시스템 > 완벽한 부분 시스템
```

### ❌ DON'T (하지 말아야 할 것)
- **진행 중단하고 복습** → 모멘텀 상실 위험
- **완벽주의 추구** → 무한 학습의 함정
- **파라미터 하나하나 논문 찾기** → 시간 대비 효과 낮음
