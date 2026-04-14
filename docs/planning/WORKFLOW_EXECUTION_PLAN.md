# Claude Code 핵심 업무 실행 계획서

> **참고 강의**: [클로드 코드 완벽 마스터: AI 개발 워크플로우 기초부터 실전까지](https://www.inflearn.com/course/%ED%81%B4%EB%A1%9C%EB%93%9C-%EC%BD%94%EB%93%9C-%EC%99%84%EB%B2%BD-%EB%A7%88%EC%8A%A4%ED%84%B0-ai-%EA%B0%9C%EB%B0%9C?cid=339317) (짐코딩)
> **프로젝트**: Mind Palette — 아동 인물화(HFD) 지능측정 AI 시스템

---

## 1. 핵심 워크플로우: Explore → Plan → Implement → Commit

강의에서 제시하는 검증된 AI 개발 사이클을 Mind Palette 프로젝트에 맞게 적용합니다.

| 단계 | 도구 | 구체적 방법 |
|------|------|-------------|
| **Explore** | `Grep`, `Glob`, `Read`, Task(Explore 에이전트) | 작업 대상 모듈의 코드·테스트·의존성을 파악. 단순 검색은 Grep/Glob 직접 호출, 넓은 탐색은 Explore 서브에이전트 활용 |
| **Plan** | `EnterPlanMode`, `TodoWrite` | 비단순 작업은 Plan Mode 진입 → 구현 전략 설계 → 사용자 승인. TodoWrite로 단계별 체크리스트 관리 |
| **Implement** | `Edit`, `Write`, `Bash`(빌드/테스트) | TDD Red→Green→Refactor 사이클로 구현. 스킬(`/tdd-workflows-tdd-red`, `/tdd-workflows-tdd-green`, `/tdd-workflows-tdd-refactor`) 활용 |
| **Commit** | `Bash`(git) | 구조적/기능적 변경 분리 커밋. 사용자 요청 시에만 커밋 실행 |

---

## 2. 역할별 실행 방법

### 역할 A: TDD Red-Green 사이클

```
사용자: "go" → plan.md 미완료 항목 확인 → TDD 실행
```

| 단계 | 사용 도구 | 실행 내용 |
|------|-----------|-----------|
| **Red (실패 테스트 작성)** | `/tdd-workflows-tdd-red` 스킬 → `Write`/`Edit` | plan.md의 `[TDD][L1/L2/L3]` 항목에서 테스트 작성 |
| **실패 확인** | `Bash` | C++: `cmake --build ... && ctest ...` / Python: `pytest` |
| **Green (최소 구현)** | `/tdd-workflows-tdd-green` 스킬 → `Edit` | 테스트 통과할 최소한의 코드만 작성 |
| **통과 확인** | `Bash` | 동일 테스트 명령 재실행, 통과 확인 |
| **Refactor** | `/tdd-workflows-tdd-refactor` 스킬, `/refactor-validator` | Tidy First 원칙 준수 검증 후 구조 개선 |

**핵심 원칙**: 한 사이클에 하나의 테스트만. TodoWrite로 현재 진행 중인 테스트 1개를 `in_progress`로 표시.

### 역할 B: C++ preprocess-server 작업

| 상황 | 워크플로우 |
|------|-----------|
| **새 기능 추가** | Explore(해당 모듈 구조 파악) → Plan(EnterPlanMode) → Red → Green → Commit |
| **기존 코드 수정** | `Read`로 대상 파일 확인 → `Edit`로 수정 → `Bash`로 빌드·테스트 |
| **빌드 실패** | `/systematic-debugging` 스킬 → 에러 분석 → 최소 수정 → 재빌드 |

**빌드/테스트 명령** (Bash 도구):
```bash
cmake --build preprocess-server/build --config Release
ctest --test-dir preprocess-server/build --output-on-failure
```

### 역할 C: Git 커밋 관리

**📖 반드시 참고**: [../project-guides/git-workflow-guide.md](../project-guides/git-workflow-guide.md)

#### 역할 분담 (명확한 경계)

| 주체 | 담당 작업 |
|------|-----------|
| **Claude Code** | git status/diff/log 분석, Self-Review 체크, 커밋 메시지 작성, 실행할 명령어 텍스트로 제공 |
| **사용자** | 제공받은 명령어를 터미널에서 직접 실행, GitHub PR 생성·Review·Merge |

#### 커밋 시 Claude Code 실행 순서
```
1. git status / diff / log 분석
2. Self-Review 체크리스트 확인 후 결과 보고
3. Conventional Commits 형식으로 커밋 메시지 초안 작성
4. 아래 명령어를 텍스트로 제공 (사용자가 터미널에서 직접 실행)
   ├── git checkout -b feature/...
   ├── git add <파일>
   ├── git commit -m "..."
   └── git push -u origin feature/...
5. GitHub PR 생성 안내 (사용자가 웹에서 직접 처리)
```

#### 커밋 타입
| 변경 유형 | 커밋 메시지 패턴 | 예시 |
|-----------|-----------------|------|
| 구조적 (Structural) | `refactor: ...` | `refactor: FilterPipeline에서 Strategy 패턴 분리` |
| 기능적 (Behavioral) | `feat: ...` / `fix: ...` | `feat: AdaptiveThreshold 필터 구현` |
| 테스트 | `test: ...` | `test: Canny edge detection L2 변환 로직 검증` |

**규칙**:
- 구조적 변경 먼저 커밋 → 기능적 변경을 별도 커밋
- Feature Branch + Pull Request 방식 (main에 직접 커밋 금지)
- 사용자 명시 요청 시에만 커밋 준비 진행

### 역할 D: 빠른 버그 수정

```
에러 발생 → /systematic-debugging 스킬 자동 호출 → 원인 분석 → Edit로 수정 → Bash로 테스트 → 통과 확인
```

`/debugger`와 `/systematic-debugging` 스킬을 에러 조우 시 즉시 활용합니다.

### 역할 E: 리팩터링 (Tidy First)

| 순서 | 행동 | 검증 |
|------|------|------|
| 1 | 테스트 전체 통과 확인 (Green 상태) | `Bash`로 테스트 실행 |
| 2 | 구조적 변경 1개만 수행 | `Edit` |
| 3 | 테스트 재실행 → 통과 확인 | `Bash` |
| 4 | `/refactor-validator` 스킬로 Tidy First 준수 검증 | 스킬 호출 |
| 5 | 구조적 커밋 (사용자 요청 시) | `Bash`(git) |

---

## 3. 서브에이전트 활용 전략

강의에서 강조하는 서브에이전트(Sub-Agent)를 업무 유형별로 배분합니다.

| 서브에이전트 유형 | 사용 시점 | 예시 |
|-------------------|-----------|------|
| **Explore** | 넓은 범위 코드 탐색 (3회 이상 검색 예상 시) | "preprocess-server에서 ThreadPool 관련 파일 전체 파악" |
| **Plan** | 구현 전략 설계 | "Multi-head 분류 구조의 테스트 설계 전략" |
| **Bash** | 독립적인 빌드/테스트 실행 | 워크트리 격리 빌드 테스트 |
| **general-purpose** | 복합적 조사 필요 시 | "ONNX Runtime 호환성 조사 + 코드 영향 분석" |

**병렬 실행**: 독립적인 작업은 서브에이전트를 동시에 여러 개 실행하여 속도를 높입니다.

---

## 4. 스킬(Skills) 활용 맵

이미 구성된 16개 스킬을 업무 흐름에 매핑합니다.

| 업무 상황 | 호출 스킬 | 목적 |
|-----------|-----------|------|
| TDD Red 단계 진입 | `/tdd-workflows-tdd-red` | 실패 테스트 작성 가이드 |
| TDD Green 단계 진입 | `/tdd-workflows-tdd-green` | 최소 구현 가이드 |
| TDD Refactor 단계 | `/tdd-workflows-tdd-refactor` | 리팩터링 가이드 |
| 리팩터링 검증 | `/refactor-validator` | Tidy First 준수 확인 |
| 버그/에러 발생 | `/systematic-debugging` | 체계적 디버깅 프로세스 |
| 코드 리뷰 요청 | `/code-review` | TDD·품질 기준 리뷰 |
| 벤치마크 리포트 | `/benchmark-reporter` | C++ 성능 측정 리포트 |
| 일일 진행도 | `/daily-progress` | plan.md 기반 진행 현황 |
| Phase 4 착수 | `/phase4-guide` | Python AI 서버 개발 가이드 |
| API 설계 | `/api-design-principles` | REST API 설계 원칙 |

---

## 5. 컨텍스트 관리 (CLAUDE.md + Memory)

| 파일 | 역할 | 관리 방법 |
|------|------|-----------|
| `CLAUDE.md` | 프로젝트 규칙·역할·구조 (Git 추적) | 역할 변경/규칙 추가 시 `Edit`으로 업데이트 |
| `docs/CODING_STANDARDS.md` | 코딩 표준 SSOT | 새 표준 도입 시 업데이트 |
| `plan.md` | TDD 체크리스트·로드맵 | 항목 완료 시 `[x]`로 체크 |
| Memory 디렉토리 | 세션 간 학습 축적 | 반복 패턴·디버깅 인사이트 저장 |

---

## 6. Antigravity와의 협업 프로토콜

| 상황 | Claude Code (나) | Antigravity |
|------|-------------------|-------------|
| 새 Phase 시작 | 대기 | 아키텍처 설계 → implementation_plan.md 작성 |
| 설계 승인 후 | TDD 사이클 실행 | MCP로 기술 조사 지원 |
| 버그 발견 | 즉시 수정 → 테스트 | 근본 원인 분석 (필요 시) |
| 리팩터링 필요 | Tidy First로 실행 | 구조 개선안 제시 |
| 코드 리뷰 | `/code-review` 스킬 활용 | 크로스-모듈 의존성 분석 |

**충돌 방지**: 동일 파일 동시 수정 금지. 작업 영역을 모듈 단위로 명확히 분리.

---

## 7. 현재 다음 액션 (plan.md 기준)

현재 Phase 4의 첫 미완료 항목:

> `[ ] FastAPI 서버 골격 구축`

실행 시 워크플로우:
1. **Explore**: ai-server 디렉토리 구조 확인 (존재 여부)
2. **Plan**: `/phase4-guide` 스킬 호출 → FastAPI + pytest 환경 설계
3. **Red**: `/tdd-workflows-tdd-red` → `/health` 응답 구조 테스트 작성
4. **Green**: `/tdd-workflows-tdd-green` → FastAPI 기본 골격 구현
5. **Commit**: 사용자 승인 후 `feat: FastAPI 서버 골격 및 헬스 체크 구현`

---

## 참고 자료

- [클로드 코드 완벽 마스터 강의 (인프런)](https://www.inflearn.com/course/%ED%81%B4%EB%A1%9C%EB%93%9C-%EC%BD%94%EB%93%9C-%EC%99%84%EB%B2%BD-%EB%A7%88%EC%8A%A4%ED%84%B0-ai-%EA%B0%9C%EB%B0%9C?cid=339317)
- [짐코딩 공식 사이트](https://www.gymcoding.co/)
