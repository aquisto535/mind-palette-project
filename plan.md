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

## ⚙️ Phase 3: C++ Preprocessing Server 
> 목표(계획서 기준): **Crow + OpenCV** 기반 전처리 마이크로서비스를 구축하고, **전처리 속도 < 100ms**를 목표로 멀티스레딩 최적화 및 **정적 분석/테스트(QA)** 를 적용한다.

### Week 1: REST API 기본 골격 (Crow)
- [x] `GET /` 요청 시 200 OK와 함께 서버 상태 메시지를 반환해야 한다.
- [x] `GET /health` 요청 시 200 OK와 "OK"를 반환해야 한다.
- [x] **통신 계약(초기, 파일 경로 공유)**: `Node.js ↔ C++`는 `{ "imagePath": "/shared/uploads/img.jpg" }` → `{ "processedPath": "/shared/processed/img_clean.jpg" }` JSON으로 주고받아야 한다.
- [x] **Node.js ↔ C++ 통신 테스트**: API Gateway가 C++ 전처리 엔드포인트를 호출해 `processedPath`를 받을 수 있어야 한다.

### Week 2: OpenCV 전처리(기본) + API
- [ ] **OpenCV 도입(vcpkg + CMake)**: 전처리 모듈을 빌드에 포함해야 한다.
- [ ] `POST /preprocess` 요청 시 `imagePath`가 없거나 빈 값이면 400 Bad Request를 반환해야 한다.
- [ ] `POST /preprocess` 요청 시 존재하지 않는 파일 경로면 404 Not Found를 반환해야 한다.
- [ ] `POST /preprocess` 요청 시 유효한 이미지 경로면 200 OK와 `processedPath`를 반환해야 한다.
- [ ] **크기 정규화**: 입력 이미지는 512x512로 리사이즈되어야 한다.
- [ ] **노이즈 제거**: GaussianBlur + medianBlur가 적용되어야 한다.
- [ ] **그레이스케일 변환**: 후속 에지 검출을 위한 grayscale 이미지가 생성되어야 한다.

### Week 3: 에지/배경 제거 고도화
- [ ] **Canny 에지 검출**: 에지 이미지가 생성되어야 한다.
- [ ] **윤곽선 강화**: 모폴로지 연산(MORPH_CLOSE 등)이 적용되어야 한다.
- [ ] **배경 제거**: GrabCut 기반 배경 제거가 적용되어야 한다.
- [ ] **이진화 및 모폴로지**: 이진화 + 모폴로지 결과를 저장할 수 있어야 한다.

### Week 4: 멀티스레딩/성능/품질(QA)
- [ ] **Thread Pool 구현(표준 C++만)**: `std::thread`/`std::mutex`/`std::condition_variable` 기반으로 구현해야 한다. (Windows API 금지)
- [ ] **배치 처리**: 여러 이미지를 한 번에 처리하여 오버헤드를 줄일 수 있어야 한다.
- [ ] **성능 벤치마크**: 전처리 1건 처리 시간을 측정하고 < 100ms 목표 달성 근거를 남겨야 한다.
- [ ] **Atomic Write 구현**: 결과 파일 저장 시 `.tmp`로 쓰고 완료 후 `rename`하여 Python 서버가 미완성 파일을 읽는 경쟁 상태(Race Condition) 방지.
- [ ] **MSVC Code Analysis + Core Guidelines**: 메모리 누수/오용을 점검하고(RAII 준수, Raw Pointer 최소화) CI 파이프라인에 포함되어야 한다.
- [ ] **GoogleTest (GTest)**: 이미지 처리 알고리즘 단위 테스트 작성 (회귀 테스트/엣지 케이스 포함)

## 🧠 Phase 4: Python AI Server (예정)

- [ ] (TODO) FastAPI 서버 헬스 체크
- [ ] (TODO) 추론 요청 테스트

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
