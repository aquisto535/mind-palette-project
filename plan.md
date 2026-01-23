# 📋 Mind Palette 개발 계획 (TDD Checklist)

이 문서는 `.cursorrules`에 따라 TDD 사이클을 관리하는 체크리스트입니다.
"시작(go)" 명령 시, 체크되지 않은(`[ ]`) 가장 상단의 항목부터 테스트 작성을 시작합니다.

---

## 🚀 Phase 2: API Gateway (Node.js) 개발

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

### Week 3: 에지/배경 제거 고도화 (Advanced OpenCV)
- [ ] **GrabCut 배경 제거**: 단순 자르기가 아닌 GrabCut 알고리즘을 적용하여 배경을 정밀하게 제거해야 한다. (AI 전처리 최적화)
- [ ] **정량적 특징 추출(Feature Extraction)**: 필압 분석(히스토그램), 선 떨림 보정(Contour Moment) 등 수치적 특징을 계산하여 AI에 전달해야 한다.
- [ ] **하이브리드 결과 결합**: C++에서 계산한 기하학적 특징과 AI 추론 결과를 결합하는 로직을 설계해야 한다.
- [ ] **Canny 에지 검출**: 에지 이미지가 생성되어야 한다.
- [ ] **윤곽선 강화**: 모폴로지 연산(MORPH_CLOSE 등)이 적용되어야 한다.
- [ ] **이진화 및 모폴로지**: 이진화 + 모폴로지 결과를 저장할 수 있어야 한다.

### Week 4: 멀티스레딩/성능/품질(QA)
- [ ] **Thread Pool 구현(표준 C++만)**: `std::thread`/`std::mutex`/`std::condition_variable` 기반으로 구현해야 한다. (Windows API 금지)
- [ ] **배치 처리**: 여러 이미지를 한 번에 처리하여 오버헤드를 줄일 수 있어야 한다.
- [ ] **성능 벤치마크**: 전처리 1건 처리 시간을 측정하고 < 100ms 목표 달성 근거를 남겨야 한다.
- [ ] **Atomic Write 구현**: 결과 파일 저장 시 `.tmp`로 쓰고 완료 후 `rename`하여 Python 서버가 미완성 파일을 읽는 경쟁 상태(Race Condition) 방지.
- [ ] **MSVC Code Analysis + Core Guidelines**: 메모리 누수/오용을 점검하고(RAII 준수, Raw Pointer 최소화) CI 파이프라인에 포함되어야 한다.
- [ ] **GoogleTest (GTest)**: 이미지 처리 알고리즘 단위 테스트 작성 (회귀 테스트/엣지 케이스 포함)

## 🧠 Phase 4: Python AI Server (Target: 2026.02 ~ 03)

### Step 1: Base Model (FastAPI + PyTorch)
- [ ] **FastAPI 서버 구축**: 헬스 체크 및 추론 엔드포인트(`POST /predict`)를 구현해야 한다.
- [ ] **PyTorch 모델 구성**: 학습된 모델의 아키텍처(ResNet/CNN 등)를 정의하고 가중치(`.pt` 또는 `.pth`)를 로드하는 클래스를 작성해야 한다.
- [ ] **클래스 매핑 및 레이블링**: 모델 출력 텐서를 실제 신체 부위나 분석 항목 명칭으로 변환하는 매핑 로직을 구현해야 한다.
- [ ] **Toy Model (MVP)**: 단순한 CNN 모델(ResNet18 등)을 로드하여 더미 데이터를 추론할 수 있어야 한다.
- [ ] **E2E 연동**: Node.js ↔ C++(전처리) ↔ Python(추론) 전체 파이프라인이 동작해야 한다.

### Step 2: Universal Optimization (ONNX)
- [ ] **ONNX 변환**: 학습된 PyTorch 모델(`.pt`)을 ONNX 포맷(`.onnx`)으로 변환(Export)해야 한다.
- [ ] **ONNX Runtime 교체**: 추론 엔진을 ONNX Runtime으로 교체하고, 기존 PyTorch 추론 대비 속도 향상을 검증해야 한다.

### Step 3: Extreme Optimization (TensorRT)
- [ ] **TensorRT Engine 빌드**: ONNX 모델을 NVIDIA TensorRT Engine(`.plan`)으로 변환해야 한다.
- [ ] **Quantization (FP16)**: FP16(반정밀도) 양자화를 적용하여 추론 속도를 극대화해야 한다.
- [ ] **최종 벤치마크**: PyTorch vs ONNX vs TensorRT 성능 비교 리포트를 작성해야 한다.

---

## 🌐 Phase 5: 통합 및 고도화 (배포 전략)

### 성능 최적화 (Performance Optimization)
- [ ] **Hash-based Caching (중복 방지)**: 이미지 업로드 시 SHA-256 해시를 계산하여, 기존에 처리된 이미지(`results/`)가 있다면 분석을 생략하고 즉시 반환.
- [ ] **Inference Optimization**: Python AI 서버의 추론 엔진을 ONNX Runtime으로 교체.

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
- [ ] **structlog 도입**: 구조화된 JSON 로그를 표준 출력(stdout)으로 출력해야 한다.
- [ ] **추론 추적**: 모델 입력 경로, 추론 결과(점수), 추론 시간을 기록해야 한다.
- [ ] **컨텍스트 바인딩**: Request ID를 로그에 자동 추가하여 전체 파이프라인 추적이 가능해야 한다.

### 통합 (Cross-Service Integration)
- [ ] **로그 형식 통일**: 모든 서비스가 `timestamp`, `service`, `level`, `message` 필드를 포함해야 한다.
- [ ] **Request ID 전파**: Node.js에서 생성한 UUID를 C++, Python까지 헤더로 전달하여 전체 요청 흐름을 추적할 수 있어야 한다.
- [ ] **에러 알림 시스템(선택)**: CRITICAL 레벨 로그 발생 시 Slack/Email로 알림을 보내는 메커니즘을 구현해야 한다.

---

## 🏥 Cross-Cutting Concerns: System Reliability
> 목표: 시스템이 24/7 안정적으로 동작하고, 장애 발생 시 즉시 감지할 수 있는 기반을 구축한다.

### Health Checks (헬스 체크) - Tier 1: 필수
- [x] **C++**: `/health` 엔드포인트가 구현되어 있어야 한다. (완료)
- [x] **Node.js**: `/health` 엔드포인트가 서버 상태(uptime, 메모리 사용량, 디스크 여유 공간)를 JSON으로 반환해야 한다.
- [ ] **Python**: `/health` 엔드포인트가 모델 로드 상태 및 GPU 사용 가능 여부를 확인하여 반환해야 한다.
- [ ] **Docker Healthcheck**: `docker-compose.yml`에 각 서비스의 healthcheck 설정(interval, timeout, retries)이 추가되어야 한다.

### Monitoring (모니터링) - Tier 2: 권장 (Phase 5)
- [ ] **메트릭 수집**: 각 서버가 요청 수, 처리 시간, 에러 수를 로그에 기록해야 한다.
- [ ] **Prometheus + Grafana (선택)**: Phase 5에서 메트릭을 Prometheus로 수집하고 Grafana 대시보드로 시각화해야 한다.
- [ ] **성능 임계값 알림**: 응답 시간이 3초를 초과하면 경고 로그를 남겨야 한다.

### Error Tracking (에러 추적) - Tier 2: 권장 (Phase 5)
- [ ] **Sentry (Frontend)**: React 앱에서 발생한 에러를 자동으로 Sentry로 전송하여 브라우저별 에러 통계를 수집해야 한다.
- [ ] **Slack Webhook (선택)**: CRITICAL 레벨 에러 발생 시 Slack 채널로 실시간 알림을 보내야 한다.

### API Documentation (API 문서화) - Tier 2: 권장 (Phase 5)
- [ ] **FastAPI Swagger**: Python AI 서버의 `/docs` 엔드포인트에서 자동 생성된 Swagger UI를 확인할 수 있어야 한다.
- [ ] **Node.js API 명세**: Postman Collection 또는 OpenAPI 3.0 스펙을 작성하여 `docs/` 폴더에 저장해야 한다.
- [ ] **README 업데이트**: API 엔드포인트 목록 및 사용 예시를 README.md에 추가해야 한다.

---

## 🔐 Cross-Cutting Concerns: Security
> 목표: 입력/저장/전송/의존성 전 구간에서 최소한의 보안 기준을 충족한다.

### 입력 검증 (Input Validation) - Tier 1: 필수
- [ ] **파일 업로드 검증**: 이미지 파일의 MIME 타입/확장자/매직 바이트를 검증해야 한다.
- [ ] **파일 크기 제한**: 업로드 크기 상한(예: 10MB)을 설정해야 한다.
- [ ] **경로 정규화**: `imagePath` 입력은 허용된 디렉토리 하위 경로만 허용해야 한다. (Path Traversal 방지)

### 저장/무결성 (Storage & Integrity) - Tier 1: 필수
- [ ] **Atomic Write**: 결과 파일 저장 시 `.tmp` → `rename` 방식을 사용해야 한다. (Race Condition 방지)
- [ ] **해시 무결성**: 전처리 결과물에 대해 SHA-256 해시를 저장/검증해야 한다.
- [ ] **보관 정책**: 업로드/결과 파일의 보관 기간 및 삭제 정책을 정의해야 한다.

### 전송 보안 (Transport Security) - Tier 2: 권장
- [ ] **외부 HTTPS**: Frontend ↔ API Gateway는 HTTPS를 사용해야 한다.
- [ ] **내부망 격리**: API Gateway ↔ C++ ↔ Python 통신은 내부망 HTTP로 제한해야 한다.

### 로깅 보안 (Logging Hygiene) - Tier 2: 권장
- [ ] **PII 마스킹**: 이름/생년월일 등 개인정보는 로그에 남기지 않거나 마스킹해야 한다.
- [ ] **에러 메시지 최소화**: 내부 경로/스택 노출을 최소화해야 한다.

### 의존성/공급망 보안 (Dependency Security) - Tier 2: 권장
- [ ] **정기 점검**: `npm audit`와 CodeQL을 주기적으로 확인해야 한다.
- [ ] **버전 고정**: vcpkg baseline pinning을 유지하여 빌드 재현성을 확보해야 한다.
