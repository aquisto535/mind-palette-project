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
- [ ] (Refactor) `server.js`의 비즈니스 로직을 별도 모듈로 분리해야 한다.

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
- [ ] `handleUpload` 함수가 실제 API 엔드포인트로 `FormData`를 전송해야 한다 (Mocking).
- [ ] API 응답 성공 시 결과 페이지(`setStep('result')`)로 전환되어야 한다.
- [ ] API 응답 실패 시 에러 메시지를 처리해야 한다.

---

## ⚙️ Phase 3: C++ Preprocessing Server (예정)

- [ ] (TODO) C++ 서버 헬스 체크 엔드포인트 테스트
- [ ] (TODO) 이미지 전처리 요청 테스트
- [ ] **MSVC Code Analysis**: C++ 코드 품질 분석(메모리 누수, API 오용 등)이 CI 파이프라인에 포함되어야 한다.
- [ ] **GoogleTest (GTest)**: 이미지 처리 알고리즘 단위 테스트 작성

## 🧠 Phase 4: Python AI Server (예정)

- [ ] (TODO) FastAPI 서버 헬스 체크
- [ ] (TODO) 추론 요청 테스트

