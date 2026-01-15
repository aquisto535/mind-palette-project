# Mind Palette 프로젝트 개발 진행 상황

## 📅 프로젝트 로드맵 및 현황

| 단계 | 주요 내용 | 상태 | 완료/예정일 |
|:---:|:---|:---:|:---:|
| **Phase 1** | **프로젝트 착수 및 설계** | ✅ 완료 | 2025-12-05 |
| | - 요구사항 정의, 아키텍처 설계 | ✅ 완료 | |
| | - 기술 스택 선정 (React, Node.js, C++, Python) | ✅ 완료 | |
| | - 개발 환경 설정 (Cursor, Git) | ✅ 완료 | |
| **Phase 2** | **Frontend & API Gateway (MVP)** | 🔄 진행중 | |
| | - React 프론트엔드 UI 구현 | ✅ 완료 | 2025-12-06 |
| | - Netlify 배포 및 CI/CD 구축 | ✅ 완료 | 2025-12-06 |
| | - Node.js API Gateway 기본 구조 | ✅ 완료 | 2025-12-06 |
| | - 파일 업로드 및 결과 반환 API (Mock) | ✅ 완료 | 2025-12-06 |
| | - Frontend-Backend 연동 (로컬/Mock 분기) | ✅ 완료 | 2025-12-06 |
| **Phase 3** | **C++ Preprocessing Server** | 🔄 진행중 | |
| | - C++ REST API (Crow) 구축 | ✅ 완료 | |
| | - Node.js ↔ C++ 연동 (JSON Protocol) | ✅ 완료 | |
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
