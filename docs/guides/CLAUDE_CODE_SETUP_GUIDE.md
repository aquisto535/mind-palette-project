# Claude Code 설정 현황 가이드

> Mind Palette 프로젝트에서 Claude Code에 적용된 모든 설정을 한 곳에서 관리합니다.
> **마지막 업데이트**: 2026-03-02

---

## 전체 구조 요약

```
mind-palette-project/
├── CLAUDE.md                          ← 핵심 지시문 (자동 로드)
├── plan.md                            ← 작업 계획 추적
├── .claude/
│   ├── settings.json                  ← 환경 변수 + 훅 설정
│   ├── agents/                        ← 커스텀 서브에이전트 (5개)
│   │   ├── cpp-builder.md
│   │   ├── tdd-runner.md
│   │   ├── code-reviewer.md
│   │   ├── git-workflow.md
│   │   └── researcher.md
│   ├── hooks/                         ← 자동 실행 스크립트 (4개)
│   │   ├── code-quality-reminder.sh
│   │   ├── secret-guard.sh
│   │   ├── context-sync.sh
│   │   └── session-checkpoint.sh
│   ├── commands/                      ← 사용자 호출 커맨드 (3개)
│   │   ├── quick-commit.md
│   │   ├── handoff-verify.md
│   │   └── verify-loop.md
│   └── skills/                        ← 자동 호출 스킬 (16개)
├── .agent/
│   └── rules/                         ← 상시 적용 규칙 (4개)
│       ├── code-style-guide.md
│       ├── role-strategy.md
│       ├── testing.md
│       └── performance.md
└── ~/.claude.json                     ← MCP 서버 등록 (5개)
```

---

## 1. CLAUDE.md (핵심 지시문)

**경로**: `CLAUDE.md` (프로젝트 루트)
**로드 시점**: 매 세션 시작 시 자동 로드

### 포함 내용

| 섹션 | 내용 |
|------|------|
| 역할 전략 | TDD·터미널·Git 관리 전문가. Antigravity와 역할 분담 |
| 프로젝트 개요 | 마이크로서비스 구조 (C++ → Python → Node.js → React) |
| 워크플로우 | Explore → Plan → Implement → Commit 사이클 |
| Git 커밋 규칙 | Conventional Commits, Self-Review 체크리스트, 금지사항 |
| Gotchas | vcpkg triplet, git 명령어 실행 방침 등 |

### 참조하는 문서

| 참조 | 경로 |
|------|------|
| 코딩 표준 | `docs/CODING_STANDARDS.md` |
| 워크플로우 실행 계획 | `docs/WORKFLOW_EXECUTION_PLAN.md` |
| Git 워크플로우 가이드 | `docs/project-guides/git-workflow-guide.md` |

---

## 2. 커스텀 서브에이전트 (5개)

**경로**: `.claude/agents/`
**로드 시점**: 세션 시작 시 자동 등록. Claude가 작업 유형에 따라 자동 위임.

| 에이전트 | 모델 | 도구 | 메모리 | 용도 |
|----------|------|------|--------|------|
| **cpp-builder** | haiku | Bash, Read, Grep, Glob | 없음 | C++ 빌드/테스트 실행, 컴파일 에러 분석 |
| **tdd-runner** | inherit | Read, Write, Edit, Bash, Grep, Glob | project | TDD Red-Green-Refactor 사이클 실행 |
| **code-reviewer** | inherit | Read, Grep, Glob, Bash | project | 코드 리뷰 (TDD/Tidy First 준수 검증) |
| **git-workflow** | haiku | Bash, Read, Grep, Glob | 없음 | Self-Review + 커밋 메시지 생성 |
| **researcher** | haiku | Read, Grep, Glob, WebFetch, WebSearch | 없음 | 기술 리서치 (Context7 + Fetch MCP 연동) |

### 내장 서브에이전트와의 차이

| | 내장 (Explore, Plan 등) | 커스텀 |
|---|---|---|
| 시스템 프롬프트 | 범용 | **프로젝트 맞춤** |
| 도구 제한 | 유형별 고정 | **세밀하게 제어** |
| 모델 선택 | 고정 | **에이전트별 선택** |
| 영구 메모리 | 없음 | **세션 간 학습** |
| MCP 서버 | 전체 상속 | **필요한 것만** |

### 자동 위임 흐름

```
사용자: "preprocess-server 빌드하고 테스트 실행해줘"
  → Claude 판단: C++ 빌드/테스트 → cpp-builder 에이전트 위임
  → cpp-builder: cmake build → ctest → 결과 요약 반환

사용자: "이 코드 리뷰해줘"
  → Claude 판단: 코드 리뷰 → code-reviewer 에이전트 위임
  → code-reviewer: git diff 분석 → 체크리스트 검토 → 결과 반환
```

### 명시적 호출

```
"cpp-builder 에이전트로 빌드 에러 분석해줘"
"tdd-runner 에이전트로 이 기능의 TDD 사이클 진행해줘"
"researcher 에이전트로 OpenCV GaussianBlur API 확인해줘"
```

### 관리

```bash
/agents              # 에이전트 목록 확인·관리 (대화형)
claude agents        # CLI에서 목록 확인
```

---

## 3. 환경 변수 (settings.json)

**경로**: `.claude/settings.json`

```json
{
  "env": {
    "ENABLE_TOOL_SEARCH": "true",
    "MAX_MCP_OUTPUT_TOKEN": "25000"
  }
}
```

| 변수 | 값 | 효과 |
|------|-----|------|
| `ENABLE_TOOL_SEARCH` | `true` | MCP 도구를 항상 필요할 때만 동적 로드. 컨텍스트 윈도우 절약 |
| `MAX_MCP_OUTPUT_TOKEN` | `25000` | MCP 응답 최대 토큰 제한. 과도한 출력 방지 |

**변경 가능 값 (ENABLE_TOOL_SEARCH)**:
- `auto` — 10% 초과 시 자동 활성화 (기본값)
- `auto:N` — N% 초과 시 자동 활성화
- `true` — 항상 활성화 (현재 설정)
- `false` — 비활성화 (모든 도구 미리 로드)

---

## 4. MCP 서버 (5개)

**등록 위치**: `~/.claude.json` (프로젝트 스코프)

### 1순위 (핵심)

| 서버 | 전송 방식 | 용도 | 명령어 |
|------|----------|------|--------|
| **context7** | stdio | 라이브러리 최신 문서 조회 (OpenCV, Crow, FastAPI 등) | `npx -y @upstash/context7-mcp` |
| **github** | http | PR/Issue/CI 관리 | `https://api.githubcopilot.com/mcp/` |
| **fetch** | stdio | 웹 콘텐츠 가져오기 | `npx -y @modelcontextprotocol/server-fetch` |

### 2순위 (작업별)

| 서버 | 전송 방식 | 용도 | 명령어 |
|------|----------|------|--------|
| **mcp-cmake** | stdio | CMake 빌드/테스트 구조화 분석 | `python -m mcp_cmake.server -w .../preprocess-server` |
| **code-checker** | stdio | pylint + pytest 코드 품질 검사 | `mcp-code-checker` |

### 관리 명령어

```bash
claude mcp list              # 전체 목록 확인
claude mcp get <name>        # 특정 서버 상세
claude mcp remove <name>     # 서버 제거
/mcp                         # 세션 내 상태 확인 + GitHub 인증
```

### 설치 명령어 (재설치 필요 시)

```bash
# 1순위
claude mcp add --transport stdio context7 -- cmd /c npx -y @upstash/context7-mcp
claude mcp add --transport http github https://api.githubcopilot.com/mcp/
claude mcp add --transport stdio fetch -- cmd /c npx -y @modelcontextprotocol/server-fetch

# 2순위
pip install git+https://github.com/hiono/mcp-cmake.git
claude mcp add --transport stdio mcp-cmake -- cmd /c python -m mcp_cmake.server -w <preprocess-server-path>

pip install git+https://github.com/MarcusJellinghaus/mcp-code-checker.git
claude mcp add --transport stdio code-checker -- cmd /c mcp-code-checker
```

---

## 4. 훅 (4개) — 자동 실행

**경로**: `.claude/hooks/`
**설정**: `.claude/settings.json`의 `hooks` 섹션

| 훅 | 이벤트 | 트리거 | 동작 |
|----|--------|--------|------|
| **code-quality-reminder.sh** | PostToolUse (Write/Edit) | C++/Python/TS 파일 수정 시 | 테스트·빌드 명령어 리마인더 |
| **secret-guard.sh** | PreToolUse (Write) | .env 파일 또는 시크릿 패턴 감지 시 | 보안 경고 출력 |
| **context-sync.sh** | PostToolUse (Bash) | cmake/ctest/pytest 실행 후 | MEMORY.md 업데이트 제안 |
| **session-checkpoint.sh** | Stop | 매 응답 종료 시 | 체크포인트 리마인더 |

### 비활성화 방법

`.claude/settings.json`에서 해당 이벤트 블록을 제거하면 됩니다.

```
예: session-checkpoint가 너무 노이즈할 경우
→ settings.json에서 "Stop" 블록 전체 삭제
```

---

## 5. 커맨드 (3개) — `/명령어`로 호출

**경로**: `.claude/commands/`

| 커맨드 | 호출 | 용도 |
|--------|------|------|
| **quick-commit** | `/quick-commit` | Self-Review 체크리스트 + Conventional Commits 메시지 생성 |
| **handoff-verify** | `/handoff-verify` | Claude Code ↔ Antigravity 작업 인수인계 검증 |
| **verify-loop** | `/verify-loop` | TDD Red-Green-Refactor 반복 루프 가이드 |

### 사용 예시

```
/quick-commit              → 변경사항 분석 + 커밋 메시지 제안
/handoff-verify            → 인수인계 체크리스트 생성
/verify-loop 이미지 전처리  → 해당 기능의 TDD 사이클 반복
```

---

## 6. 스킬 (16개) — 자동 호출

**경로**: `.claude/skills/`

### TDD 관련

| 스킬 | 호출 | 용도 |
|------|------|------|
| tdd-workflow | `/tdd-workflow` | TDD 원칙 가이드 |
| tdd-workflows-tdd-red | `/tdd-workflows-tdd-red` | 실패 테스트 생성 (Red) |
| tdd-workflows-tdd-green | `/tdd-workflows-tdd-green` | 최소 구현 (Green) |
| tdd-workflows-tdd-refactor | `/tdd-workflows-tdd-refactor` | 리팩터링 (Refactor) |

### 디버깅·검증

| 스킬 | 호출 | 용도 |
|------|------|------|
| systematic-debugging | `/systematic-debugging` | 체계적 디버깅 프로세스 |
| debugger | `/debugger` | 에러 및 테스트 실패 디버깅 |
| refactor-validator | `/refactor-validator` | Tidy First 원칙 준수 검증 |
| code-review | `/code-review` | 코드 리뷰 (TDD, First Principles) |

### 기술별 전문가

| 스킬 | 호출 | 용도 |
|------|------|------|
| typescript-expert | `/typescript-expert` | TypeScript/JavaScript 전문 |
| react-patterns | `/react-patterns` | React 패턴·성능 |
| api-design-principles | `/api-design-principles` | REST/GraphQL API 설계 |
| architecture | `/architecture` | 아키텍처 의사결정 |

### 프로젝트 운영

| 스킬 | 호출 | 용도 |
|------|------|------|
| phase4-guide | `/phase4-guide` | Phase 4 AI Server 개발 가이드 |
| daily-progress | `/daily-progress` | 일일 진행도 리포트 생성 |
| benchmark-reporter | `/benchmark-reporter` | C++ 벤치마크 리포트 생성 |
| wiki-qa | `/wiki-qa` | 코드베이스 Q&A |

---

## 7. 규칙 파일 (4개) — 상시 적용

**경로**: `.agent/rules/`

| 파일 | 내용 |
|------|------|
| **code-style-guide.md** | `docs/CODING_STANDARDS.md` 참조 연결 |
| **role-strategy.md** | 작업 유형별 행동 가이드 (설계/UI/리뷰/조사) |
| **testing.md** | TDD 강제 규칙, L1/L2/L3 깊이 분류, 네이밍, 금지사항 |
| **performance.md** | C++ 최적화 패턴, Smart Pointer, 벤치마크 기준 |

---

## 8. 메모리

**경로**: `~/.claude/projects/.../memory/MEMORY.md`
**특성**: 세션 간 지속. 자동 로드됨.

### 포함 내용
- 핵심 실행 전략 (Explore → Plan → Implement → Commit)
- 역할별 실행 방법 (TDD, C++, Git, 디버깅, 리팩터링)
- 스킬 활용 맵
- Antigravity와의 협업 규칙
- 다음 액션 (Phase 4 FastAPI 서버)

---

## 설정 로드 순서 (우선순위)

```
세션 시작
  ↓
1. CLAUDE.md 로드 (핵심 지시문)
  ↓
2. .agent/rules/*.md 로드 (상시 규칙)
  ↓
3. .claude/settings.json 로드 (환경 변수 + 훅)
  ↓
4. memory/MEMORY.md 로드 (세션 간 메모리)
  ↓
5. MCP 서버 연결 (Tool Search로 필요 시에만 도구 로드)
  ↓
6. 스킬/커맨드 목록 등록 (필요 시 호출)
```

---

## 자주 쓰는 명령어 모음

| 명령어 | 용도 |
|--------|------|
| `/quick-commit` | 커밋 준비 (Self-Review + 메시지 생성) |
| `/verify-loop` | TDD 사이클 반복 |
| `/handoff-verify` | Antigravity 인수인계 |
| `/tdd-workflows-tdd-red` | 실패 테스트 작성 |
| `/tdd-workflows-tdd-green` | 최소 구현 |
| `/systematic-debugging` | 버그 디버깅 |
| `/daily-progress` | 일일 진행 리포트 |
| `/mcp` | MCP 서버 상태 확인 |
| `/compact` | 컨텍스트 압축 |
