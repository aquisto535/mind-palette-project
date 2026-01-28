# 📋 Mind Palette 개발 계획 (TDD Checklist)

이 문서는 `.cursorrules`에 따라 TDD 사이클을 관리하는 체크리스트입니다.
"시작(go)" 명령 시, 체크되지 않은(`[ ]`) 가장 상단의 항목부터 테스트 작성을 시작합니다.

---

## �️ 개발 방법론 (Methodology)
- **TDD Cycle**: Always [Red] → [Green] → [Refactor]
- **Tidy First**: 구조적 변경(Structural)과 기능적 변경(Behavioral)을 분리한다.
- **MCP Workflow**: [MCP_WORKFLOWS.md](file:///c:/Users/user/Documents/GitHub/mind-palette-project/docs/methodology/MCP_WORKFLOWS.md)에 따라 `shrimp`, `sequential-thinking`, `context7`을 유기적으로 활용한다.

---

## �🚀 Phase 2: API Gateway (Node.js) 개발

### 기본 서버 설정 및 상태 확인
- [x] `GET /` 요청 시 200 OK와 함께 서버 상태 메시지를 반환해야 한다.

### 이미지 분석 API (`POST /analyze`)
- [x] 이미지 파일 없이 요청 시 400 Bad Request 에러를 반환해야 한다.
- [x] 유효한 이미지 파일 업로드 시 200 OK와 분석 결과 JSON을 반환해야 한다.
- [x] 업로드된 이미지가 `shared_volume/uploads` 폴더에 실제로 저장되어야 한다.
- [x] 분석 결과가 `shared_volume/results` 폴더에 JSON 파일로 저장되어야 한다.

### 리팩터링 (Refactoring)
- [x] (Refactor) `server.js`의 비즈니스 로직을 별도 모듈로 분리해야 한다.

---

## 🛡️ 품질 및 보안 보증 (Quality Assurance)

### CI/CD 파이프라인 및 보안 분석
- [x] GitHub Actions 워크플로우(`.github/workflows/main.yml`)가 정상적으로 동작해야 한다.
- [x] Backend 및 Frontend 단위 테스트가 CI에서 자동 실행되어야 한다.
- [x] 통합 테스트(Integration Test)가 CI에서 자동 실행되어야 한다.
- [x] **CodeQL (Security Analysis)**: JavaScript/TypeScript 코드의 보안 취약점 분석이 CI에 포함되어야 한다.

---

## 🎨 Phase 1: Frontend (React) 통합

### 업로드 컴포넌트 연동
- [x] `handleUpload` 함수가 실제 API 엔드포인트로 `FormData`를 전송해야 한다 (Mocking).
- [x] API 응답 성공 시 결과 페이지(`setStep('result')`)로 전환되어야 한다.
- [x] API 응답 실패 시 에러 메시지를 처리해야 한다.

---

## ⚙️ Phase 3: C++ Preprocessing Server (Target: 2026.01 ~ 02)
> 목표(계획서 기준): **Crow + OpenCV** 기반 전처리 마이크로서비스를 구축하고, **전처리 속도 < 100ms**를 목표로 멀티스레딩 최적화 및 **정적 분석/테스트(QA)** 를 적용한다.

### Week 1: REST API 기본 골격 (Crow)
- [x] `GET /` 요청 시 200 OK와 함께 서버 상태 메시지를 반환해야 한다.
- [x] `GET /health` 요청 시 200 OK와 "OK"를 반환해야 한다.
- [x] **통신 계약(초기, 파일 경로 공유)**: `Node.js ↔ C++`는 `{ "imagePath": "/shared/uploads/img.jpg" }` → `{ "processedPath": "/shared/processed/img_clean.jpg" }` JSON으로 주고받아야 한다.
- [x] **Node.js ↔ C++ 통신 테스트**: API Gateway가 C++ 전처리 엔드포인트를 호출해 `processedPath`를 받을 수 있어야 한다.

### Week 2: OpenCV 전처리(기본) + API
- [x] **OpenCV 도입(vcpkg + CMake)**: 전처리 모듈을 빌드에 포함해야 한다.
- [x] `POST /preprocess` 요청 시 `imagePath`가 없거나 빈 값이면 400 Bad Request를 반환해야 한다.
- [x] `POST /preprocess` 요청 시 존재하지 않는 파일 경로면 404 Not Found를 반환해야 한다.
- [x] `POST /preprocess` 요청 시 유효한 이미지 경로면 200 OK와 `processedPath`를 반환해야 한다.
- [x] **크기 정규화**: 입력 이미지는 512x512로 리사이즈되어야 한다.
- [x] **노이즈 제거**: GaussianBlur + medianBlur가 적용되어야 한다.
- [x] **그레이스케일 변환**: 후속 에지 검출을 위한 grayscale 이미지가 생성되어야 한다.

### Week 3: 에지/배경 제거 고도화 (Advanced OpenCV + Deep Dive)
- [ ] **[MCP]** `sequential-thinking`을 사용하여 GrabCut vs DL 기반 배경 제거의 효율성 분석 (제1원칙)
- [ ] **GrabCut 배경 제거 & 실험**: 
  - [ ] [TDD] `image_processor_test.cpp`: GrabCut 초기 마스크 생성 및 유효성 검증 테스트 (Red)
  - [ ] `image_processor.cpp`: GrabCut 알고리즘 기본 구현 (Green)
  - [ ] **[Deep Dive] Optimization**: `iterCount`(1회 vs 5회)에 따른 수행 시간(ms)과 품질 차이를 주석으로 기록.
- [ ] **정량적 특징 추출 (Features)**:
  - [ ] **[MCP]** `context7`으로 Canny 알고리즘의 최신 최적화 파라미터 조사
  - [ ] [TDD] Canny Threshold(low/high) 변화에 따른 엣지 검출 정량적 정확도 테스트 (Red)
  - [ ] 필압 분석(히스토그램), 선 떨림 보정(Contour Moment) 등 수치적 특징 계산 로직 구현 (Green)
- [ ] **하이브리드 결과 결합**: C++ 기하학적 특징 + AI 추론 결과 결합 로직 설계 및 테스트.
- [ ] **윤곽선 강화**: 모폴로지 연산(MORPH_CLOSE 등) 적용 후 결과 무결성 테스트.
- [ ] **이진화 및 모폴로지**: 이진화 처리 결과 저장 원자성(Atomicity) 확인.

### Week 4: 멀티스레딩/성능/품질 (Concurrency Deep Dive)
- [ ] **Thread Pool 구현 (std::thread)**: 
  - [ ] [TDD] 스레드 풀 작업 큐의 동기화 및 데드락 방지 단위 테스트 (Red)
  - [ ] `std::thread`/`mutex`/`condition_variable` 기반 표준 스레드 풀 구현 (Green)
  - [ ] **[Deep Dive] Scalability Test**: 스레드 개수(1 vs 4 vs 8)에 따른 처리량(Throughput) 비교 벤치마크 수행.
- [ ] **배치 처리**: 병렬 작업 분할 로직에 대한 데이터 레이스 검증 테스트.
- [ ] **성능 벤치마크**: [TDT] 전처리 1건 처리 시간 < 100ms 자동 회귀 테스트 구축.
- [ ] **Atomic Write & Safety**: 
  - [ ] [TDD] 저장 중 프로세스 종료 시 Corrupted 파일 잔존 여부 테스트 (Red)
  - [ ] `.tmp` → `rename` 패턴 적용으로 원자성(Atomicity) 보장 (Green)
- [ ] **[MCP]** MSVC Code Analysis 및 Core Guidelines 위반 사항 `sequential-thinking`분석
- [ ] **Quality Gates**: CI 파이프라인에 정적 분석 통합 및 통과 확인.
- [ ] **GoogleTest (GTest)**: 전체 알고리즘에 대한 경계값(Edge Case) 및 회귀 테스트 완료.

## 🧠 Phase 4: Python AI Server (Target: 2026.02 ~ 03)

### Step 1: Base Model (FastAPI + PyTorch)
- [ ] **FastAPI 서버 구축**: 
  - [ ] [TDD] `/health` 요청 시 200 OK와 모델 로딩 상태 반환 테스트 (Red)
  - [ ] FastAPI 기본 골격 및 헬스 체크 엔드포인트 구현 (Green)
- [ ] **PyTorch 모델 구성**: 
  - [ ] [TDD] 모델 가중치(`.pt`) 파일 무결성 확인 및 아키텍처 일치 테스트 (Red)
  - [ ] ResNet/CNN 기반 모델 로드 클래스 작성 (Green)
- [ ] **클래스 매핑 및 레이블링**: 
  - [ ] [TDD] 출력 텐서 ↔ 레이블 명칭 변환 결과 정확도 테스트 (Red)
  - [ ] 매핑 로직 및 후처리 클래스 구현 (Green)
- [ ] **Toy Model (MVP)**: 단순한 CNN 모델(ResNet18 등)을 로드하여 더미 데이터를 추론할 수 있어야 한다.
- [ ] **E2E 연동**: Node.js ↔ C++(전처리) ↔ Python(추론) 전체 파이프라인 통합 테스트.

### Step 2: Universal Optimization (ONNX + Deep Dive)
- [ ] **[MCP]** `context7`으로 PyTorch 모델의 ONNX 변환 시 지원되는 최신 Ops 및 호환성 리서치
- [ ] **ONNX 변환**: 
  - [ ] [TDD] ONNX 모델과 원본 PyTorch 모델의 추론 결과 오차(Epsilon) 검증 테스트 (Red)
  - [ ] 모델 변환 및 ONNX Runtime 추론 엔진 구현 (Green)
- [ ] **[Deep Dive] Latency Analysis**: PyTorch 순정 vs ONNX Runtime 추론 속도 비교 측정 및 주석 기록.
- [ ] **ONNX Runtime 교체**: 추론 엔진 교체 및 속도 향상 검증.

### Step 3: Extreme Optimization (TensorRT + Deep Dive)
- [ ] **[MCP]** `sequential-thinking`을 사용하여 TensorRT 엔진 빌드 시 FP16 양자화에 따른 정확도 손실 분석 (제1원칙)
- [ ] **TensorRT Engine 빌드**: 
  - [ ] [TDD] TensorRT 엔진 추론 가용성 및 GPU 메모리 할당 테스트 (Red)
  - [ ] ONNX ↔ TensorRT 변환 및 Quantization(FP16) 적용 (Green)
- [ ] **최종 벤치마크**: PyTorch vs ONNX vs TensorRT 성능 비교 리포트를 작성해야 한다.

---

## 🌐 Phase 5: 통합 및 고도화 (배포 전략)

### 성능 최적화 (Performance Optimization)
- [ ] **Hash-based Caching (중복 방지)**: 
  - [ ] [TDD] 동일 이미지 업로드 시 캐시 적중(Hit) 및 결과 즉시 반환 테스트 (Red)
  - [ ] SHA-256 해시 기반 분석 생략 로직 구현 (Green)
- [ ] **Inference Optimization**: Python AI 서버의 추론 엔진 최종 ONNX/TensorRT 통합 및 회귀 테스트.

### 배포 아키텍처 및 보안 (Architecture & Security)
- [ ] **Nginx Reverse Proxy 도입**: AWS EC2 앞단에 Nginx를 배치하여 SSL 인증서(Let's Encrypt) 관리 및 HTTPS 트래픽 처리.
- [ ] **Mixed Content 방지**: Frontend(HTTPS) ↔ API Gateway(HTTPS) 간 보안 통신 구현.
- [ ] **Internal Private Network**: API Gateway ↔ C++ ↔ Python 구간은 내부망 HTTP 통신(Plain Text) 유지하여 성능 최적화 (SSL 오버헤드 제거).
- [ ] **Docker Compose 프로덕션 설정**: `restart: always`, 로깅 드라이버, 볼륨 백업 정책 적용.

---

## 📊 Cross-Cutting Concerns: Logging System
> 목표: 모든 서비스에 구조화된 로깅(Structured Logging)을 도입하여, 장애 추적 및 성능 분석을 가능하게 한다.

### Node.js (API Gateway) - Winston
- [x] **Winston 도입**: JSON 포맷, 파일/콘솔 동시 출력, 로그 레벨(DEBUG/INFO/WARN/ERROR) 설정이 되어야 한다.
- [x] **요청/응답 로깅**: 모든 API 요청의 메타데이터(타임스탬프, 파일명, 크기)를 INFO 레벨로 기록해야 한다.
- [x] **에러 스택 추적**: 예외 발생 시 전체 스택 트레이스를 ERROR 레벨로 기록해야 한다.

### C++ (Preprocess Server) - spdlog
- [x] **spdlog 도입**: vcpkg로 설치하고, 멀티스레드 안전(thread-safe) 로깅이 되어야 한다.
- [x] **성능 로깅**: 전처리 소요 시간을 밀리초(ms) 단위로 측정하여 기록해야 한다.
- [x] **파일 회전(Rotation)**: 로그 파일이 10MB를 초과하면 자동으로 새 파일로 교체되어야 한다.

### Python (AI Server) - structlog
- [ ] **structlog 도입**: 
  - [ ] [TDD] 로그 출력 시 JSON 포맷 유효성 및 필수 필드 포함 여부 테스트 (Red)
  - [ ] `structlog` 바인딩 및 표준 출력 설정 구현 (Green)
- [ ] **추론 추적**: 모델 입력 경로, 추론 결과(점수), 추론 시간을 기록해야 한다.
- [ ] **컨텍스트 바인딩**: Request ID를 로그에 자동 추가하여 전체 파이프라인 추적이 가능해야 한다.

### 통합 (Cross-Service Integration)
- [ ] **[MCP]** `sequential-thinking`을 사용하여 수만 건의 로그가 생성될 때의 성능 오버헤드 최소화 전략 분석
- [ ] **Request ID 전파**: 
  - [ ] [TDD] Node.js에서 생성한 UUID가 C++, Python 서버 로그에 일관되게 나타나는지 통합 테스트 (Red)
  - [ ] 헤더 전파 로직 및 각 서비스별 로그 필드 매핑 구현 (Green)
- [ ] **에러 알림 시스템(선택)**: CRITICAL 레벨 로그 발생 시 Slack/Email 알림 메커니즘 구축.

---

## 🏥 Cross-Cutting Concerns: System Reliability
> 목표: 시스템이 24/7 안정적으로 동작하고, 장애 발생 시 즉시 감지할 수 있는 기반을 구축한다.

### Health Checks (헬스 체크) - Tier 1: 필수
- [x] **C++**: `/health` 엔드포인트 구현 완료.
- [x] **Node.js**: `/health` 엔드포인트 구현 완료.
- [ ] **Python**: 
  - [ ] [TDD] GPU 메모리 고갈 시 503 Service Unavailable 반환 테스트 (Red)
  - [ ] 모델 로드 상태 및 리소스 확인 헬스 체크 구현 (Green)
- [ ] **Docker Healthcheck**: 
  - [ ] [TDD] 컨테이너 비정상 종료 시 Docker Daemon의 재시작 정책 동작 테스트 (Red)
  - [ ] `docker-compose.yml` 내 healthcheck (interval, timeout) 설정 (Green)

### API Documentation (API 문서화) - Tier 2: 권장
- [ ] **[MCP]** `context7`으로 OpenAPI 3.0 스펙의 가독성 좋은 문서화 패턴 리서치
- [ ] **Node.js API 명세**: 
  - [ ] [TDD] API 명세 파일이 실제 엔드포인트 구조와 일치하는지 자동 검증 테스트 (Red)
  - [ ] OpenAPI 3.0/Swagger Spec 작성 및 저장 (Green)

---

## 🔐 Cross-Cutting Concerns: Security
> 목표: 입력/저장/전송/의존성 전 구간에서 최소한의 보안 기준을 충족한다.

### 입력 검증 (Input Validation) - Tier 1: 필수
- [ ] **[MCP]** `context7`으로 이미지 파일 매직 바이트를 활용한 완벽한 확장자 위조 탐지 기법 리서치
- [ ] **파일 업로드 검증**: 
  - [ ] [TDD] .txt 파일을 .jpg로 속여 업로드 시 차단되는지 테스트 (Red)
  - [ ] MIME 타입/매직 바이트 기반 물리적 검증 로직 구현 (Green)
- [ ] **경로 정규화**: 
  - [ ] [TDD] `../../etc/passwd`와 같은 Path Traversal 공격 시 차단 테스트 (Red)
  - [ ] 입력 경로 정규화 및 화이트리스트 디렉토리 체크 구현 (Green)

### 저장/무결성 (Storage & Integrity) - Tier 1: 필수
- [ ] **Atomic Write & Atomic Delete**: 
  - [ ] [TDD] 저장/삭제 중 예상치 못한 중단 시 데이터 불일치 여부 테스트 (Red)
  - [ ] `.tmp` → `rename` 패턴 및 원자적 삭제 로직 보완 (Green)
- [ ] **해시 무결성**: 
  - [ ] [TDD] 결과 파일 변조 시 캐시 매칭 실패 및 재분석 트리거 테스트 (Red)
  - [ ] SHA-256 해시 저장 및 무결성 검증 자동화 (Green)

### 전송 보안 (Transport Security) - Tier 2: 권장
- [ ] **외부 HTTPS**: Frontend ↔ API Gateway는 HTTPS를 사용해야 한다.
- [ ] **내부망 격리**: API Gateway ↔ C++ ↔ Python 통신은 내부망 HTTP로 제한해야 한다.

### 로깅 보안 (Logging Hygiene) - Tier 2: 권장
- [ ] **PII 마스킹**: 이름/생년월일 등 개인정보는 로그에 남기지 않거나 마스킹해야 한다.
- [ ] **에러 메시지 최소화**: 내부 경로/스택 노출을 최소화해야 한다.

### 의존성/공급망 보안 (Dependency Security) - Tier 2: 권장
- [ ] **정기 점검**: `npm audit`와 CodeQL을 주기적으로 확인해야 한다.
- [ ] **버전 고정**: vcpkg baseline pinning을 유지하여 빌드 재현성을 확보해야 한다.

---

## 🧪 Cross-Cutting Concerns: Traffic & Load Testing
> 목표: 서비스의 안정성을 검증하고, 대량의 로그를 생성하여 시스템의 한계를 테스트한다.

### Traffic Generation (트래픽 생성) - Phase 3~4 (검증용)
- [ ] **[MCP]** `sequential-thinking`을 사용한 대량 로그 발생 시 파일 I/O 병목 및 시스템 영향도 분석
- [ ] **Node.js Traffic Bot**: 
  - [ ] [TDD] 봇 가동 시 로그 파일 크기 증가 및 로테이션 발동 여부 테스트 (Red)
  - [ ] `axios` 기반 주기적 요청 자동화 스크립트 작성 (Green)

### Load Testing (부하 테스트) - Phase 5 (최종 성능)
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

