# CLAUDE.md

## 역할 전략

당신(Claude Code)은 Mind Palette 프로젝트에서 **TDD 사이클·터미널 기반 구현·Git 관리** 전문가입니다.
Antigravity와 역할을 분담하고 있으며, 아래 규칙을 따르세요.

### 당신의 핵심 역할
- **TDD Red-Green 사이클**: 테스트 작성 → 실행 → 실패 확인 → 구현 → 통과의 빠른 반복
- **C++ preprocess-server 작업**: CMake 빌드 → CTest 실행 사이클
- **Git 커밋 관리**: 구조적/기능적 변경 분리 커밋 실행
  - 📖 **반드시 참고**: `docs/project-guides/git-workflow-guide.md`
  - Feature Branch + Pull Request 방식 준수
  - Conventional Commits 규칙 준수 (feat:, fix:, refactor: 등)
  - 커밋 전 Self-Review 체크리스트 확인
- **빠른 버그 수정**: 에러 발생 → 수정 → 테스트의 빠른 루프
- **리팩터링 (Tidy First)**: 구조 변경 후 테스트 실행 반복
- **단일 모듈 집중 코딩**: 한 모듈에 깊이 있는 작업

### Antigravity 담당 (당신이 하지 않아도 되는 것)
- 아키텍처 설계/분석 (sequential-thinking, shrimp 활용)
- 기술 조사/문서 탐색 (context7 활용)
- Frontend 브라우저 테스트/시각적 검증
- 복잡한 리서치 및 지식 축적

### 충돌 방지 규칙
- Antigravity가 수정 중인 파일에 동시에 접근하지 마세요
- 작업 단위(모듈/태스크)를 명확히 나누고, 한 번에 한 도구만 해당 영역을 담당

---

## 프로젝트 개요
Mind Palette — 아동 인물화(HFD) 지능측정을 위한 AI 이미지 전처리·분석 시스템.
마이크로서비스 아키텍처: C++ Preprocess Server → Python AI Server → API Gateway (Node.js/Express) → Frontend (React)

## 공통 코딩 표준
**TDD, C++17, 커밋 규율, 코드 품질 등의 상세 규칙은 `docs/CODING_STANDARDS.md`를 참조하세요.**

## 워크플로우
- `plan.md`의 미완료 항목을 순서대로 진행. "시작(go)" 명령 시 다음 항목을 실행.
- 응답 언어: 항상 **한국어**
- **세부 실행 계획**: `docs/WORKFLOW_EXECUTION_PLAN.md`를 참조하여 작업을 수행하세요.
  - Explore → Plan → Implement → Commit 사이클
  - 역할별(TDD/C++/Git/디버깅/리팩터링) 실행 방법
  - 스킬/서브에이전트 활용 맵
  - Antigravity와의 협업 프로토콜

## 프로젝트 구조

| 디렉토리 | 역할 | 기술 스택 |
|----------|------|-----------|
| `preprocess-server/` | 이미지 전처리 서버 | C++17, Crow, OpenCV, spdlog |
| `ai-server/` (예정) | AI 추론 서버 | Python, EfficientNet-B2, ONNX/TensorRT |
| `api-gateway/` | API 라우팅·보안 | TypeScript, Express, Jest |
| `frontend/` | 웹 클라이언트 | React, TypeScript |
| `docs/` | 설계 문서·ADR | Markdown |

## 💎 Git 커밋 규칙

**모든 커밋 전에 `docs/project-guides/git-workflow-guide.md`를 참조하세요.**

### Conventional Commits 형식
```
<타입>(<범위>): <제목>

<본문> (선택사항)
```

### 커밋 타입
| 타입 | 사용 시기 | 예시 |
|------|----------|------|
| `feat` | 새 기능 추가 | `feat: FastAPI 서버 골격 구축` |
| `fix` | 버그 수정 | `fix: ThreadPool 데드락 해결` |
| `test` | 테스트 추가/수정 | `test: Canny edge detection L2 검증` |
| `refactor` | 리팩터링 | `refactor: Strategy 패턴으로 필터 분리` |
| `docs` | 문서 변경 | `docs: WORKFLOW_EXECUTION_PLAN 추가` |
| `chore` | 빌드/설정 변경 | `chore: vcpkg 의존성 업데이트` |

### Self-Review 체크리스트 (커밋 전)
- [ ] 모든 테스트 통과하는가?
- [ ] 구조적 변경과 기능적 변경이 분리되어 있는가?
- [ ] 커밋 메시지가 Conventional Commits 규칙을 따르는가?
- [ ] 불필요한 console.log나 디버그 코드가 없는가?
- [ ] .gitignore에 추가해야 할 파일은 없는가?

### 금지 사항
- ❌ main 브랜치에 직접 커밋
- ❌ 구조적 변경과 기능적 변경을 같은 커밋에 섞기
- ❌ 의도하지 않은 파일 변경 (공백, 디버그 코드 등)

---

## Gotchas (트러블슈팅 기록 참조)
**🚨 오류 발생 시 반드시 `docs/troubleshooting/` 폴더의 문서들을 우선 확인하세요.**
- **Windows 빌드 (CRT Mismatch 방지)**: vcpkg triplet은 반드시 **동적 링크인 `x64-windows`**를 사용합니다. 정적 링크(`x64-windows-static`) 사용 시 `std::string`, `cv::Mat` 등 객체 전달 과정에서 Heap Assertion 오류(`__acrt_first_block == header`)가 발생합니다. (참고: `docs/troubleshooting/Week4_Final_Runtime_Assertion_Fix.md`)
- **MSVC 인코딩 (C2523 / C4819 방지)**: 한글 주석 사용 시 파싱 오류(개행 먹힘)를 피하기 위해 `CMakeLists.txt`에 반드시 `/utf-8` 플래그가 포함되어야 합니다.
- OpenCV 의존성은 vcpkg.json으로 관리 — 수동 설치 금지
- **명령어 실행 방침**: git 명령어(add, commit, push)는 실행할 명령어를 텍스트로 제공하고, 사용자가 터미널에서 직접 실행. GitHub PR 생성·Review·Merge도 사용자가 직접 처리.
