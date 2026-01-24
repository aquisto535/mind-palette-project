# Mind Palette 프로젝트 개발 진행 상황

## 📅 프로젝트 로드맵 및 현황

| 단계 | 주요 내용 | 상태 | 완료/예정일 |
|:---:|:---|:---:|:---:|
| **Phase 1** | **프로젝트 착수 및 설계** | ✅ 완료 | 2025-12-05 |
| | - 요구사항 정의, 아키텍처 설계 | ✅ 완료 | |
| | - 기술 스택 선정 (React, Node.js, C++, Python) | ✅ 완료 | |
| | - 개발 환경 설정 (Cursor, Git) | ✅ 완료 | |
| **Phase 2** | **Frontend & API Gateway (MVP)** | ✅ 완료 | 2026-01-19 |
| | - React 프론트엔드 UI 구현 | ✅ 완료 | 2025-12-06 |
| | - Netlify 배포 및 CI/CD 구축 | ✅ 완료 | 2025-12-06 |
| | - Node.js API Gateway 기본 구조 | ✅ 완료 | 2025-12-06 |
| | - 파일 업로드 및 결과 반환 API (Mock) | ✅ 완료 | 2025-12-06 |
| | - Frontend-Backend 연동 (로컬/Mock 분기) | ✅ 완료 | 2025-12-06 |
| | - Health Check API 구현 | ✅ 완료 | 2026-01-19 |
| **Phase 3** | **C++ Preprocessing Server** | 🔄 진행중 | |
| | - C++ REST API (Crow) 구축 | ✅ 완료 | 2026-01-14 |
| | - Node.js ↔ C++ 연동 (JSON Protocol) | ✅ 완료 | 2026-01-15 |
| | - 이미지 전처리 (OpenCV) | | |
| | - 윤곽선 추출 및 노이즈 제거 | | |
| **Phase 4** | **Python AI Inference Server** | ⏳ 대기 | |
| | - PyTorch 모델 서빙 | | |
| | - HTP/KFD 분석 알고리즘 구현 | | |
| **Phase 5** | **통합 및 고도화** | ⏳ 대기 | |
| | - 전체 파이프라인 연동 | | |
| | - 성능 최적화 및 보안 강화 | | |

---

## 📝 상세 진행 로그

### 2026-01-24 (Day 8)
- **API Gateway TypeScript 마이그레이션**
  - **Language Conversion**: Node.js(`server.js`) 기반 프로젝트를 TypeScript(`src/server.ts`)로 100% 변환 완료.
  - **Type Safety**:
    - Express (`Request`, `Response`) 및 Multer (`File`) 등 주요 객체에 엄격한 타입 적용.
    - `CustomRequest` 인터페이스 정의로 커스텀 속성(`fileValidationError`) 타입 안정성 확보.
  - **Configuration & Build**:
    - `tsconfig.json` 설정 (ES2020, Strict Mode).
    - `tsc` 빌드 파이프라인 및 `nodemon` + `ts-node` 개발 환경 구축.
  - **Test Migration**:
    - 기존 Jest 테스트 코드(`.js`)를 모두 TypeScript(`.ts`)로 리팩터링.
    - 5개 테스트 슈트, 14개 테스트 케이스 전원 통과 확인 (`npm test`).

### 2026-01-23 (Day 7)
- **API Gateway 안정성 및 보안 강화**
  - **Health Check API 레이트 리미팅 적용**:
    - `express-rate-limit` 도입: 호출 횟수 제한을 통한 DoS 방어 기반 마련.
    - `/health` 엔드포인트 전용 리미터 설정: 1분당 최대 60회 호출로 제한.
    - **도입 배경**: `/health` 경로가 실행하는 OS 명령(`execSync` 기반 `wmic`, `df`)의 리소스 소모(CPU/Blocking) 문제를 해결하고 잠재적 악용을 차단.
    - 검증 완료: 60회 초과 호출 시 `429 Too Many Requests` 정상 반환 확인.

### 2026-01-19 (Day 6)
- **프로젝트 일정 가속화 및 고도화 전략 수립**
  - **일정 재조정**: PyTorch 학습 속도(2월 2주차 완강 예상)에 맞춰 전체 로드맵을 2개월씩 앞당김.
    - Phase 3 (C++ 전처리): 3월 → **1월 말~2월**
    - Phase 4 (AI 서버/모델): 4~5월 → **2월~3월**
    - Phase 5 (통합/최적화): 6월 → **4월**
  - **AI 최적화 3단계 로드맵 확립**: PyTorch(Base) → ONNX(Universal) → TensorRT(Extreme) 경로 명확화.
  - **OpenCV 심화 과정 정의**: GrabCut 배경 제거, 정량적 특징 추출(필압/선 떨림), 하이브리드 결합 로직 계획.

- **개발 인프라 및 품질 보증 체계 구축**
  - **로깅 시스템 설계**: 
    - 서비스별 도구 선정 (Node.js: Winston, C++: spdlog, Python: structlog)
    - Request ID 전파를 통한 분산 추적(Distributed Tracing) 기반 마련
    - 구조화된 JSON 로그 포맷 통일 (timestamp, service, level, message)
  - **System Reliability 계획**: Health Checks, Monitoring(Prometheus+Grafana), Error Tracking(Sentry), API Documentation(Swagger) 체계화.
  - **`.gitignore` 생성**: C++/CMake/Visual Studio 아티팩트 및 `shared_volume` 런타임 데이터 제외 규칙 적용.
  - **`.cursorrules` 업데이트**: "User Execution Preferred" 규칙 추가 (터미널 명령어는 제안만 하고 사용자가 직접 실행).

- **Node.js API Gateway 완성도 향상**
  - **Health Check API 구현** (TDD):
    - `/health` 엔드포인트 추가: 서버 uptime, 메모리 사용량, 디스크 여유 공간을 JSON으로 반환.
    - Windows/Linux 크로스 플랫폼 디스크 체크 로직 구현 (`wmic` / `df` 명령어 분기).
    - 5개 테스트 케이스 작성 및 전체 통과 (5/5 passed, 2.694s).
  - **Phase 2 완료**: API Gateway가 프로덕션 수준의 헬스 체크 및 안정성 기능 확보.
  - **보안 강화 (Security Hardening)** (TDD):
    - **입력 검증(Input Validation)**: `multer` 설정을 통해 파일 업로드 보안 강화.
      - MIME 타입 검사: `image/*` 외 파일 거부 (텍스트 파일 등으로 테스트 검증).
      - 파일 크기 제한: 5MB 초과 파일 거부.
      - 안정성 개선: `ECONNRESET` 방지를 위해 `cb(null, false)` 패턴 및 `req.fileValidationError` 플래그 처리 적용.
    - **테스트 커버리지**: `tests/security.test.js` 추가 및 3개 시나리오(정상, 비이미지, 대용량) 통과.
  - **업로드 보안 검증 구현 완료**:
    - `fileFilter` + `limits` 적용으로 업로드 보안 정책을 실제 코드에 반영.
    - 에러 응답 표준화 (`Only image files are allowed`, `File too large`).

- **문서화 및 계획 고도화**
  - **`plan.md` 대폭 확장**: 
    - Cross-Cutting Concerns 섹션 신설 (로깅, 시스템 안정성).
    - Phase 4 AI 서버 체크리스트를 단계별(Base/ONNX/TensorRT)로 세분화.
    - Phase 3 OpenCV 체크리스트에 심화 알고리즘 항목 추가.
  - **프로젝트 계획서 업데이트**: 마일스톤 날짜를 현실 진행 속도에 맞춰 조정.

### 2026-01-15 (Day 5)
- **Node.js ↔ C++ 연동 파이프라인 구성**
  - **통신 규격 정의**: Node.js가 이미지 경로(`imagePath`)를 보내면 C++이 결과 경로(`processedPath`)를 반환하는 JSON 프로토콜 확립.
  - **통합 테스트**: `curl` 및 `Jest` + `nock`을 활용하여 Node.js API Gateway에서 C++ 서버를 호출하고 응답을 받는 전체 흐름 검증 완료.
  - **기술적 의사결정**: Docker/Linux 환경을 고려하여 Windows API 대신 표준 C++ 사용 원칙 재확인.

### 2026-01-14 (Day 4)
- **Phase 3: C++ 전처리 서버 기초 구축**
  - **Crow 프레임워크 도입**: 경량 C++ REST API 서버 구축 (`preprocess-server`).
  - **라우팅 구현**: 헬스 체크(`GET /health`) 및 전처리 요청(`POST /preprocess`) 엔드포인트 구현.
  - **테스트 주도 개발(TDD)**:
    - **GoogleTest(GTest)** 도입: API 라우팅 및 JSON 응답 검증을 위한 단위 테스트 환경 구축.
    - **테스트 케이스**: `RootRoute`, `HealthCheck`, `PreprocessContract` (통신 규격) 테스트 작성 및 통과.

### 2025-12-29 (Day 3)
- **보안 취약점 대응 (Security Patch)**
  - **React CVE 대응**: React 19 버전에서 보고된 **'인증받지 않은 원격 코드 실행(RCE)'** 및 RSC 관련 취약점(CVE-2025-55183 등) 대응.
    - 조치: 프로젝트의 React 버전을 안정적인 `v18.3.1`로 고정/업데이트하여 해당 취약점 원천 차단.
  - **의존성 강제 업데이트**: `npm audit fix --force`를 통해 메이저 보안 취약점 해결.
    - `jspdf`: `v2.5.1` -> `v3.0.4` (XSS 취약점 해결).
    - `vite`: `v5.0.0` -> `v7.3.0` (개발 서버 취약점 해결).
  - **안정성 검증**: 메이저 버전 업데이트 후 빌드(`npm run build`) 정상 동작 확인.
- **테스트 환경 개선**
  - `IntersectionObserver` Mock 구현 추가로 Frontend 단위 테스트 오류 해결.

### 2025-12-06 (Day 2)
- **CI/CD 및 보안 강화**
  - GitHub Actions 워크플로우(`main.yml`) 구축: Backend/Frontend CI, Integration Test, Security Analysis
  - CodeQL Action 버전을 `v2` -> `v3`로 마이그레이션 (Deprecated 경고 해결)
  - `integration.test.js` 추가: 실제 API 호출 및 파일 저장 검증 자동화
- **Frontend-Backend 연동**
  - `frontend/src/api/uploadApi.ts` 수정: `VITE_USE_MOCK` 환경변수로 Mock/Real 모드 전환 가능하도록 개선
  - 로컬 연동 테스트 성공: `curl`을 이용한 파일 업로드 및 `shared_volume` 저장 확인
- **API Gateway 고도화 (TDD & Tidy First)**
  - `server.js` 리팩터링: `routes`, `services`, `utils`로 모듈 분리
  - TDD 사이클 완료: 실패하는 테스트 -> 구현 -> 리팩터링
- **보안 및 의존성 관리 (Maintenance)**
  - Node.js 패키지 업데이트: `multer` (v2.0.2), `supertest` (v7.1.4) 등 최신 보안 버전 적용
  - GitHub Actions 버전 업그레이드: `v3` -> `v4` (CodeQL, Checkout, Setup-Node) 일괄 적용하여 미래 호환성 확보
  - Frontend CI 강화: `npm run build` 단계 추가로 배포 전 빌드 오류 사전 차단

### 2025-12-05 (Day 1)
- **프로젝트 초기 설정**
  - `plan.md`, `.cursorrules`, `development_progress.md` 문서화 완료
  - Node.js API Gateway 프로젝트 초기화 (`api-gateway` 폴더)
  - React Frontend 프로젝트 초기화 및 Netlify 배포 성공
