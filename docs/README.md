# Mind Palette 프로젝트 문서

> Mind Palette — 아동 인물화(HFD) 지능측정 AI 시스템의 모든 문서를 한 곳에서 관리합니다.

---

## 문서 구조 (의도 기반 분류)

```text
docs/
├── architecture/    — 설계 결정(ADR) 전용
├── standards/       — 코딩 규약·리뷰 기준
├── planning/        — 개발 계획·마일스톤
├── guides/          — 실무 How-to·셋업·배포
├── learning/        — 배경 지식·튜토리얼
├── reference/       — 기술 레퍼런스 (C++, OpenCV, AI)
├── status/          — 진행 현황·벤치마크·코드 리뷰 이력
├── reports/         — 서비스별 정적 리포트
├── pipeline-stages/ — ML 파이프라인 산출물
├── troubleshooting/ — 이슈 해결 기록
├── api/             — API 명세
└── career/          — 개인 경력 자료
```

---

## architecture/ — 설계 결정 (ADR)

시스템의 핵심 아키텍처 선택과 근거를 영구 보존합니다.

| 문서 | 용도 |
|------|------|
| [ARCHITECTURE_DECISIONS.md](architecture/ARCHITECTURE_DECISIONS.md) | ADR-001 ~ 최신 — 전체 아키텍처 결정 목록 |
| [ADR-parameter-rationale.md](architecture/ADR-parameter-rationale.md) | 전처리 파라미터 선정 근거 |
| [ADR-phase5-retraining.md](architecture/ADR-phase5-retraining.md) | Phase 5 재학습 전략 결정 |
| [ADR033_ColorFilter_Bypass_Fix.md](architecture/ADR033_ColorFilter_Bypass_Fix.md) | Color Filter Fail-Open 버그 수정 근거 |

**새로운 아키텍처 변경** → `ARCHITECTURE_DECISIONS.md`에 ADR 추가

---

## standards/ — 코딩 규약

프로젝트 전체에 적용되는 품질 기준.

| 문서 | 용도 |
|------|------|
| [CODING_STANDARDS.md](standards/CODING_STANDARDS.md) | TDD, C++17, 커밋 규율, 코드 품질 표준 |
| [CODE_REVIEW_GUIDELINES.md](standards/CODE_REVIEW_GUIDELINES.md) | 코드 리뷰 체크리스트·기준 |

**새 팀원 필독**: 이 디렉토리부터 시작하세요.

---

## planning/ — 개발 계획

Phase별 개발 계획과 TDD 실행 플랜. 기한이 지나도 의사결정 맥락을 보존합니다.

| 문서 | 용도 |
|------|------|
| [WORKFLOW_EXECUTION_PLAN.md](planning/WORKFLOW_EXECUTION_PLAN.md) | **Claude Code 핵심 업무 실행 계획** (Explore→Plan→Implement→Commit) |
| [phase4_remaining_plan.md](planning/phase4_remaining_plan.md) | Phase 4 잔여 작업 계획 |
| [phase5_deployment_plan.md](planning/phase5_deployment_plan.md) | Phase 5 배포 계획 |
| [phase6-reliability-chaos-engineering.md](planning/phase6-reliability-chaos-engineering.md) | Phase 6 신뢰성·카오스 엔지니어링 |
| [ai-server-guide/](planning/ai-server-guide/) | AI 서버 Phase 4 단계별 TDD 플랜 |

---

## guides/ — 실무 가이드

개발·배포·테스트 시 따라야 할 실행 방법.

| 문서 | 용도 |
|------|------|
| [git-workflow-guide.md](guides/git-workflow-guide.md) | Git 워크플로우 상세 가이드 |
| [aws-deployment-guide.md](guides/aws-deployment-guide.md) | AWS EC2 배포 가이드 |
| [CI_CD_INTEGRATION_GUIDE.md](guides/CI_CD_INTEGRATION_GUIDE.md) | GitHub Actions CI/CD 가이드 |
| [DEPLOYMENT_CHECKLIST.md](guides/DEPLOYMENT_CHECKLIST.md) | 배포 전 체크리스트 |
| [CONFIG_INVENTORY.md](guides/CONFIG_INVENTORY.md) | 전체 환경 변수 인벤토리 |
| [개발_단계별_테스트_전략_가이드.md](guides/개발_단계별_테스트_전략_가이드.md) | Phase별 테스트 전략 |
| [C++ 실전 개발 가이드.md](<guides/C++ 실전 개발 가이드.md>) | C++17, Crow, OpenCV 실전 가이드 |
| [Mind_Palette_프론트엔드_개발_가이드.md](guides/Mind_Palette_프론트엔드_개발_가이드.md) | React/TypeScript 개발 가이드 |
| [traffic-testing-guide.md](guides/traffic-testing-guide.md) | 트래픽 부하 테스트 가이드 |
| [WORKFLOW_AGENTS_GUIDE.md](guides/WORKFLOW_AGENTS_GUIDE.md) | 서브에이전트 활용 가이드 |
| [SKILLS_USAGE_GUIDE.md](guides/SKILLS_USAGE_GUIDE.md) | Claude Code 스킬 활용 매뉴얼 |
| [refactoring_strategy.md](guides/refactoring_strategy.md) | Tidy First 리팩터링 전략 |
| [PROJECT_OVERVIEW.md](guides/PROJECT_OVERVIEW.md) | 프로젝트 전체 개요 |
| [MICROSERVICES_EXPLAINED.md](guides/MICROSERVICES_EXPLAINED.md) | 마이크로서비스 아키텍처 상세 설명 |
| [MCP_WORKFLOWS.md](guides/MCP_WORKFLOWS.md) | Sequential Thinking, Context7, Shrimp MCP 활용법 |
| [HARNESS_ENGINEERING_GUIDE.md](guides/HARNESS_ENGINEERING_GUIDE.md) | Claude Code 하네스 엔지니어링 가이드 |

---

## learning/ — 학습 자료

배경 지식 습득과 기술 개념 정리.

| 문서 | 용도 |
|------|------|
| [preprocess_server_learning_guide.md](learning/preprocess_server_learning_guide.md) | C++ 전처리 서버 학습 가이드 |
| [ai_server_learning_guide.md](learning/ai_server_learning_guide.md) | AI 서버 학습 가이드 |
| [ai_server_tech_stack_guide.md](learning/ai_server_tech_stack_guide.md) | AI 서버 기술 스택 가이드 |
| [gtest_learning_guide.md](learning/gtest_learning_guide.md) | Google Test 학습 가이드 |
| [PYTHON_FOR_CPP_DEVELOPERS.md](learning/PYTHON_FOR_CPP_DEVELOPERS.md) | C++ 개발자를 위한 Python 가이드 |
| [AI 개발 시 TDD를 적용하는 코드를 작성하게 하는 방법.md](<learning/AI 개발 시 TDD를 적용하는 코드를 작성하게 하는 방법.md>) | AI TDD 실전 가이드 |
| [React_Props와_State_핵심_개념_가이드.md](learning/React_Props와_State_핵심_개념_가이드.md) | React 핵심 개념 |
| [TypeScript_핵심_문법_가이드.md](learning/TypeScript_핵심_문법_가이드.md) | TypeScript 문법 |

---

## reference/ — 기술 레퍼런스

언어·라이브러리별 기술 자료 (변경이 적고 반복 참조됨).

```text
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
    ├── ai_model_recommendation.md
    ├── ai_model_principle.md
    └── hybrid_3channel_rationale.md
```

---

## status/ — 진행 현황

프로젝트 진행 상황, 벤치마크, 코드 리뷰 이력.

```text
status/
├── development_progress.md  — Phase별 개발 진행률
├── BENCHMARKS/              — 성능 측정 결과
├── CODE_REVIEW_HISTORY/     — 코드 리뷰 세션 기록
├── PROGRESS/                — 일일 진행도 리포트
└── REFACTOR_VALIDATION/     — 리팩터링 검증 리포트
```

---

## reports/ — 서비스 리포트

각 서비스의 구조 분석 보고서 (정적).

| 문서 | 용도 |
|------|------|
| [preprocess_architecture_report.md](reports/preprocess_architecture_report.md) | C++ 전처리 서버 아키텍처 리포트 |
| [ai_server_structure_report.md](reports/ai_server_structure_report.md) | AI 서버 구조 리포트 |
| [report_api_gateway.md](reports/report_api_gateway.md) | API Gateway 상태 리포트 |
| [report_frontend.md](reports/report_frontend.md) | Frontend 상태 리포트 |

---

## troubleshooting/ — 이슈 해결

개발 중 마주친 문제와 해결 방법 (ADR로 격상된 것은 `architecture/`로 이동).

| 문서 | 용도 |
|------|------|
| [Week4_Final_Runtime_Assertion_Fix.md](troubleshooting/Week4_Final_Runtime_Assertion_Fix.md) | CRT Mismatch / Heap Assertion 해결 |
| [Week4_OpenCV_Static_Linking.md](troubleshooting/Week4_OpenCV_Static_Linking.md) | OpenCV 정적 링킹 문제 |
| [Week3_Issues.md](troubleshooting/Week3_Issues.md) | Week 3 이슈 기록 |
| [NETLIFY_DEPLOY_TROUBLESHOOTING.md](troubleshooting/NETLIFY_DEPLOY_TROUBLESHOOTING.md) | Netlify 배포 문제 해결 |
| [aws-deployment-issues.md](troubleshooting/aws-deployment-issues.md) | AWS 배포 이슈 |
| [Phase4_Step4_TensorRT_Issues.md](troubleshooting/Phase4_Step4_TensorRT_Issues.md) | TensorRT 관련 이슈 |

---

## 역할별 빠른 시작

**개발자 (Claude Code)**
1. [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) — 코딩 표준 확인
2. [planning/WORKFLOW_EXECUTION_PLAN.md](planning/WORKFLOW_EXECUTION_PLAN.md) — 실행 계획 확인
3. [guides/git-workflow-guide.md](guides/git-workflow-guide.md) — 커밋 워크플로우

**아키텍트 (Antigravity)**
1. [architecture/ARCHITECTURE_DECISIONS.md](architecture/ARCHITECTURE_DECISIONS.md) — ADR 검토
2. [guides/MICROSERVICES_EXPLAINED.md](guides/MICROSERVICES_EXPLAINED.md) — 구조 분석
3. [guides/WORKFLOW_AGENTS_GUIDE.md](guides/WORKFLOW_AGENTS_GUIDE.md) — 에이전트 활용

**신입 팀원**
1. [guides/PROJECT_OVERVIEW.md](guides/PROJECT_OVERVIEW.md) — 프로젝트 개요
2. [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) — 표준 학습
3. [learning/](learning/) — 기술 학습

---

## 빠른 검색

| 찾는 것 | 참조 문서 |
|--------|-----------|
| TDD 방법론 | [standards/CODING_STANDARDS.md](standards/CODING_STANDARDS.md) |
| 아키텍처 결정 | [architecture/ARCHITECTURE_DECISIONS.md](architecture/ARCHITECTURE_DECISIONS.md) |
| C++ 개발 | [guides/C++ 실전 개발 가이드.md](<guides/C++ 실전 개발 가이드.md>) |
| Git 워크플로우 | [guides/git-workflow-guide.md](guides/git-workflow-guide.md) |
| 배포 절차 | [guides/DEPLOYMENT_CHECKLIST.md](guides/DEPLOYMENT_CHECKLIST.md) |
| 환경 변수 | [guides/CONFIG_INVENTORY.md](guides/CONFIG_INVENTORY.md) |
| 성능 수치 | [status/BENCHMARKS/](status/BENCHMARKS/) |
| 트러블슈팅 | [troubleshooting/](troubleshooting/) |
| EfficientNet-B2 선택 근거 | [reference/AI/ai_model_recommendation.md](reference/AI/ai_model_recommendation.md) |

---

## 문서 유지 규칙

- 아키텍처 변경 → [architecture/ARCHITECTURE_DECISIONS.md](architecture/ARCHITECTURE_DECISIONS.md)에 ADR 추가
- 이슈 해결 → [troubleshooting/](troubleshooting/) 기록, 중요한 것은 ADR로 격상
- 코드 리뷰 → [status/CODE_REVIEW_HISTORY/](status/CODE_REVIEW_HISTORY/) 저장
- 일일 진행도 → [status/PROGRESS/](status/PROGRESS/) 저장
- 벤치마크 → [status/BENCHMARKS/](status/BENCHMARKS/) 저장

---

**마지막 업데이트**: 2026-04-14
