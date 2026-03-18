# 📚 Mind Palette 프로젝트 문서

> Mind Palette — 아동 인물화(HFD) 지능측정 AI 시스템의 모든 문서를 한 곳에서 관리합니다.

---

## 📋 문서 구조

### 🎯 [standards/](standards/) — 프로젝트 규칙·표준
프로젝트 전체에 적용되는 핵심 규칙과 아키텍처 의사결정 문서.

| 문서 | 용도 |
|------|------|
| [CODING_STANDARDS.md](standards/CODING_STANDARDS.md) | TDD, C++17, 커밋 규율, 코드 품질 표준 |
| [ARCHITECTURE_DECISIONS.md](standards/ARCHITECTURE_DECISIONS.md) | 주요 아키텍처 선택과 근거 (ADR) |

**💡 먼저 읽어야 할 문서**: 새 팀원은 이 디렉토리부터 시작하세요.

---

### 📖 [guides/](guides/) — 개발 가이드·워크플로우
실제 개발 시 따라야 할 절차와 방법론.

| 문서 | 용도 |
|------|------|
| [WORKFLOW_EXECUTION_PLAN.md](guides/WORKFLOW_EXECUTION_PLAN.md) | **Claude Code 핵심 업무 실행 계획** (Explore→Plan→Implement→Commit) |
| [WORKFLOW_AGENTS_GUIDE.md](guides/WORKFLOW_AGENTS_GUIDE.md) | 서브에이전트 활용 가이드 |
| [SKILLS_USAGE_GUIDE.md](guides/SKILLS_USAGE_GUIDE.md) | Claude Code 스킬 활용 매뉴얼 |
| [refactoring_strategy.md](guides/refactoring_strategy.md) | Tidy First 리팩터링 전략 |
| [PROJECT_OVERVIEW.md](guides/PROJECT_OVERVIEW.md) | 프로젝트 전체 개요 및 마이크로서비스 구조 |
| [MICROSERVICES_EXPLAINED.md](guides/MICROSERVICES_EXPLAINED.md) | 마이크로서비스 아키텍처 상세 설명 |

**💡 시작하기**: 새 작업을 시작하면 [WORKFLOW_EXECUTION_PLAN.md](guides/WORKFLOW_EXECUTION_PLAN.md)를 참조하세요.

---

### 🧠 [methodology/](methodology/) — 개발 방법론·철학
TDD, MCP, 제1원칙 등 개발 철학 및 방법론 문서.

| 문서 | 용도 |
|------|------|
| MCP_WORKFLOWS.md | Sequential Thinking, Context7, Shrimp MCP 활용법 |
| AI 개발 시 TDD를 적용하는 코드를 작성하게 하는 방법.md | AI 개발 TDD 실전 가이드 |
| PYTHON_FOR_CPP_DEVELOPERS.md | C++ 개발자를 위한 Python 가이드 |
| CODE_REVIEW_GUIDELINES.md | 코드 리뷰 기준·체크리스트 |

---

### 📁 [project-guides/](project-guides/) — 기술별 개발 가이드
특정 기술/스택별 심화 가이드.

| 문서 | 용도 |
|------|------|
| C++ 실전 개발 가이드.md | C++17, Crow, OpenCV 실전 가이드 |
| Mind_Palette_프론트엔드_개발_가이드.md | React/TypeScript 개발 가이드 |
| 개발_단계별_테스트_전략_가이드.md | Phase별 테스트 전략 |
| traffic-testing-guide.md | 트래픽 부하 테스트 가이드 |
| git-workflow-guide.md | Git 워크플로우 상세 가이드 |

---

### 🔗 [reference/](reference/) — 기술 참고 자료
언어·라이브러리별 기술 레퍼런스.

```
reference/
├── C++/
│   ├── Modern C++ 11,14,17 문법 정리.md
│   ├── 현대_C++_성능_최적화_원칙_종합.md
│   └── C++ 동기-비동기 시스템 아키텍처 유형.md
├── OpenCV/
│   ├── opencv_blur_comparison.md
│   ├── opencv_noise_reduction.md
│   └── Week3_Test_Results_Analysis.md
└── AI/
    └── ai_model_recommendation.md
```

---

### 📊 [project-status/](project-status/) — 진행 현황·리뷰
프로젝트 진행 상황, 코드 리뷰 히스토리.

| 문서 | 용도 |
|------|------|
| development_progress.md | Phase별 개발 진행률 |
| CODE_REVIEW_HISTORY/ | 코드 리뷰 세션 기록 |

---

### 📈 [reports/](reports/) — 상태 보고서
각 모듈/서비스의 상태 리포트.

| 문서 | 용도 |
|------|------|
| report_api_gateway.md | API Gateway 상태 리포트 |
| report_frontend.md | Frontend 상태 리포트 |

---

### 📚 [learning/](learning/) — 학습 자료
개념 정리 및 학습 자료.

| 문서 | 용도 |
|------|------|
| React_Props와_State_핵심_개념_가이드.md | React 핵심 개념 |
| TypeScript_핵심_문법_가이드.md | TypeScript 문법 |

---

### 🚨 [troubleshooting/](troubleshooting/) — 문제 해결
개발 중 마주친 문제와 해결 방법.

| 문서 | 용도 |
|------|------|
| CI_CD_INTEGRATION_GUIDE.md | GitHub Actions CI/CD 가이드 |
| NETLIFY_DEPLOY_TROUBLESHOOTING.md | Netlify 배포 문제 해결 |
| Week3_Issues.md | Week 3 이슈 기록 |
| Week4_OpenCV_Static_Linking.md | OpenCV 정적 링킹 문제 |

---

## 🚀 문서 활용 방법

### 💼 역할별 가이드

**👨‍💻 개발자 (Claude Code)**
1. [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) — 코딩 표준 확인
2. [guides/WORKFLOW_EXECUTION_PLAN.md](guides/WORKFLOW_EXECUTION_PLAN.md) — 실행 계획 확인
3. 해당 기술 가이드 참조 (project-guides/)

**🏗️ 아키텍트 (Antigravity)**
1. [standards/ARCHITECTURE_DECISIONS.md](standards/ARCHITECTURE_DECISIONS.md) — ADR 검토
2. [guides/MICROSERVICES_EXPLAINED.md](guides/MICROSERVICES_EXPLAINED.md) — 구조 분석
3. [guides/WORKFLOW_AGENTS_GUIDE.md](guides/WORKFLOW_AGENTS_GUIDE.md) — 에이전트 활용

**🎓 신입 팀원**
1. [guides/PROJECT_OVERVIEW.md](guides/PROJECT_OVERVIEW.md) — 프로젝트 개요
2. [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) — 표준 학습
3. [learning/](learning/) — 기술 학습
4. [project-guides/](project-guides/) — 심화 가이드

---

## 📝 문서 유지 규칙

- 모든 주요 아키텍처 변경은 [standards/ARCHITECTURE_DECISIONS.md](standards/ARCHITECTURE_DECISIONS.md)에 ADR 기록
- 새로운 문제 해결 경험은 [troubleshooting/](troubleshooting/)에 기록
- 세션별 코드 리뷰는 [project-status/CODE_REVIEW_HISTORY/](project-status/CODE_REVIEW_HISTORY/)에 기록
- 월별 진행도는 [project-status/development_progress.md](project-status/development_progress.md)에 업데이트

---

## 🔍 빠른 검색

| 찾는 것 | 참조 문서 |
|--------|-----------|
| TDD 방법론 | [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) |
| C++ 개발 | [project-guides/C++ 실전 개발 가이드.md](project-guides/C++) |
| React/TypeScript | [project-guides/Mind_Palette_프론트엔드_개발_가이드.md](project-guides/) |
| API 설계 | [guides/MICROSERVICES_EXPLAINED.md](guides/MICROSERVICES_EXPLAINED.md) |
| 배포 문제 | [troubleshooting/](troubleshooting/) |
| 성능 최적화 | [reference/C++/현대_C++_성능_최적화_원칙_종합.md](reference/C++) |
| 프로젝트 진행률 | [project-status/development_progress.md](project-status/) |

---

**마지막 업데이트**: 2026-03-01
