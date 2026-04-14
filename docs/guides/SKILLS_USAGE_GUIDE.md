# Mind Palette — AI Skills 사용 가이드

> 이 프로젝트에 설치된 11개 스킬의 목적, 호출 방법, 사용 시점을 정리한 문서입니다.
>
> **설치 위치**: `.agent/skills/` (Antigravity) | `.claude/skills/` (Claude Code)
>
> **호출 방법**:
> - Antigravity: 에이전트 모드에서 `Use [skill-name] skill...`
> - Claude Code: `>> /skill-name ...`

---

## 🔴🟢🔵 TDD 사이클 스킬 (4개)

Mind Palette의 핵심 개발 방법론인 **Red → Green → Refactor** 각 단계에 대응하는 스킬입니다.

---

### 1. `tdd-workflow` — TDD 전체 가이드

**목적**: TDD 3법칙과 전체 사이클(Red→Green→Refactor)의 원칙 참조

**언제 사용하는가**:
- 새로운 기능 개발을 시작할 때 방향을 잡고 싶을 때
- TDD 사이클을 어기고 있는 것 같을 때 (예: 테스트 없이 구현부터 작성)
- 팀원에게 TDD 규칙을 설명할 때

**호출 예시**:
```
Use tdd-workflow skill to guide me through implementing the image analysis feature
```

**핵심 내용**:
- 3법칙: 실패 테스트 없이 구현 금지 / 테스트는 하나의 실패만 / 통과할 만큼만 구현
- AAA 패턴: Arrange → Act → Assert
- 안티패턴 목록 (Red 단계 건너뛰기, 구현 후 테스트 작성 등)

---

### 2. `tdd-workflows-tdd-red` — Red 단계: 실패 테스트 작성

**목적**: 특정 기능/동작에 대한 **포괄적인 실패 테스트** 자동 생성

**언제 사용하는가**:
- 새 기능의 TDD Red 단계 시작 시
- 엣지 케이스 테스트가 떠오르지 않을 때
- C++ 또는 TypeScript 테스트 코드 뼈대가 필요할 때

**호출 예시**:
```
Use tdd-workflows-tdd-red skill for: ImageAnalyzer::extractFeatures() 함수
- 정상 입력(유효한 이미지 mat) 처리
- 빈 Mat 입력 시 예외 발생
- 지원하지 않는 채널 수 입력 시 오류 반환
```

**지원 언어**: C++ (Google Test), TypeScript/Jest, Python (pytest), Go, Ruby (RSpec)

---

### 3. `tdd-workflows-tdd-green` — Green 단계: 최소 구현

**목적**: 실패 중인 테스트를 통과시키기 위한 **최소한의 구현**만 작성

**언제 사용하는가**:
- Red 단계 완료 후, 테스트를 통과시킬 구현을 작성할 때
- "최소한"의 기준이 모호할 때 (YAGNI 원칙 적용)
- 구현이 과도하게 복잡해지려 할 때 제동을 걸고 싶을 때

**호출 예시**:
```
Use tdd-workflows-tdd-green skill: 현재 실패 중인 테스트를 통과시킬 최소 구현 작성
[실패 테스트 코드 붙여넣기]
```

**핵심 원칙**: YAGNI (You Aren't Gonna Need It) — 테스트 통과 이상의 코드는 금지

---

### 4. `tdd-workflows-tdd-refactor` — Refactor 단계: 구조 개선

**목적**: 테스트가 모두 통과한 상태에서 코드 품질 개선 (기능 변경 없이)

**언제 사용하는가**:
- Green 단계 완료 후, 코드 냄새(Code Smell)를 제거할 때
- 중복 코드, 긴 함수, 매직 넘버 등을 정리할 때
- SOLID 원칙 위반 여부를 점검하고 싶을 때

**호출 예시**:
```
Use tdd-workflows-tdd-refactor skill: [리팩터링할 코드 붙여넣기]
- 중복 제거 및 Extract Method 적용
- Smart Pointer 패턴 통일 (C++17)
```

**핵심 체크리스트**:
- 리팩터링 전후 모든 테스트 Green 유지
- 구조 변경 커밋 ≠ 기능 변경 커밋 (Tidy First 원칙)
- SOLID 원칙 적용 여부 확인

---

## 🐛 디버깅 스킬 (2개)

---

### 5. `systematic-debugging` — 체계적 디버깅 (심층)

**목적**: 근본 원인(Root Cause)을 먼저 규명한 뒤 수정하는 **4단계 디버깅 프로세스**

**언제 사용하는가**:
- 버그가 명확하게 이해되지 않는 상태에서 수정을 시도하려 할 때 (가장 중요)
- 동일한 문제에 수정을 2회 이상 시도했는데 해결 안 될 때
- 멀티 컴포넌트 시스템(C++ preprocess-server ↔ api-gateway ↔ frontend)에서 문제가 발생했을 때

**호출 예시**:
```
Use systematic-debugging skill: preprocess-server가 이미지를 처리한 후
api-gateway로 결과를 전송하는 과정에서 데이터가 손실됨
```

**4단계 프로세스**:
1. **Root Cause Investigation**: 에러 메시지 정독, 재현, 최근 변경사항 확인, 증거 수집
2. **Pattern Analysis**: 동작하는 유사 코드 탐색, 차이점 비교
3. **Hypothesis Testing**: 단일 가설 수립 → 최소한의 변경으로 검증
4. **Implementation**: 수정 테스트 먼저, 원인에서 수정, 검증

> ⚠️ **Iron Law**: 근본 원인 규명 없이 수정 먼저 시도하는 것은 금지

---

### 6. `debugger` — 빠른 디버깅 (가벼운 이슈)

**목적**: 에러 메시지/스택 트레이스 기반 빠른 진단 및 수정

**언제 사용하는가**:
- 명확한 에러 메시지가 있고 빠른 해결이 필요할 때
- 컴파일 에러, 타입 에러 등 단순한 이슈

**호출 예시**:
```
Use debugger skill: 다음 컴파일 에러 해결해줘 [에러 메시지 붙여넣기]
```

> 💡 **선택 기준**: 이슈가 간단하고 원인이 명확하면 `debugger`, 원인 불명이거나 2회 이상 수정 실패면 `systematic-debugging`

---

## 💻 언어/프레임워크 스킬 (3개)

---

### 7. `typescript-expert` — TypeScript 전문가

**목적**: TypeScript strict mode, 고급 타입 패턴, 빌드 성능, 마이그레이션 전문 가이드

**언제 사용하는가**:
- `any` 타입 없이 복잡한 제네릭이나 조건부 타입을 작성해야 할 때
- TypeScript 컴파일 에러 (`Excessive stack depth`, `inferred type cannot be named` 등)
- `api-gateway/`, `frontend/` 코드의 타입 안전성 강화 시

**호출 예시**:
```
Use typescript-expert skill: WebSocket 메시지 페이로드에 대한
discriminated union 타입 설계 (이미지 처리 결과 / 에러 / 진행상황)
```

**핵심 기능**:
- Branded Types (도메인 원시 타입 안전 보장)
- `satisfies` 연산자, const assertions, Template Literal Types
- TypeScript 성능 진단 (`tsc --extendedDiagnostics`)
- Code Review 체크리스트 (no `any`, strict null checks 등)

---

### 8. `react-patterns` — React 패턴

**목적**: 프로덕션 수준의 React 컴포넌트 설계, Hooks, 성능 최적화

**언제 사용하는가**:
- `frontend/` 내 새 컴포넌트를 설계할 때
- 상태 관리 방식을 결정해야 할 때 (useState vs Context vs Zustand)
- 불필요한 리렌더링 최적화가 필요할 때
- React 19 신규 Hooks (`useActionState`, `useOptimistic`) 적용 시

**호출 예시**:
```
Use react-patterns skill: 이미지 업로드 진행상황을 실시간으로 표시하는
컴포넌트 설계 (낙관적 업데이트 포함)
```

**핵심 패턴**:

| 상황 | 선택 |
|---|---|
| 단일 컴포넌트 상태 | `useState` |
| 부모-자식 공유 | State 끌어올리기 |
| 서브트리 공유 | Context |
| 서버 상태 | React Query / SWR |
| 앱 전역 상태 | Zustand |

---

### 9. `api-design-principles` — API 설계 원칙

**목적**: REST API 설계 표준 (엔드포인트 설계, 에러 형식, 버저닝, 인증)

**언제 사용하는가**:
- `api-gateway/`에 새 REST 엔드포인트를 추가할 때
- 기존 API의 응답 형식이나 에러 코드를 표준화할 때
- C++ preprocess-server ↔ api-gateway 간 내부 API 계약 설계 시

**호출 예시**:
```
Use api-design-principles skill: 이미지 처리 요청/응답 API 설계
- POST /api/analyze (이미지 업로드 + 분석 요청)
- GET /api/analyze/:jobId (결과 조회)
- 에러 응답 표준 형식 정의
```

---

## 🏛️ 아키텍처 / 지식 스킬 (2개)

---

### 10. `architecture` — 아키텍처 의사결정

**목적**: 아키텍처 결정을 ADR(Architecture Decision Record) 형식으로 문서화, 트레이드오프 분석

**언제 사용하는가**:
- 새로운 기술 스택 도입 여부를 결정할 때
- 컴포넌트 분리/통합 등 구조적 변경을 고려할 때
- 기술적 선택의 근거를 문서로 남겨야 할 때

**호출 예시**:
```
Use architecture skill: preprocess-server와 ai-server 간 통신 방식 결정
- 옵션 A: REST HTTP
- 옵션 B: gRPC
- 옵션 C: 공유 메모리
각 트레이드오프 분석 및 ADR 작성
```

**핵심 원칙**: "Simplicity is the ultimate sophistication" — 복잡성은 필요가 증명된 후에만 추가

---

### 11. `wiki-qa` — 코드베이스 Q&A

**목적**: 소스 코드를 직접 분석하여 코드베이스에 관한 질문에 **증거 기반으로** 답변

**언제 사용하는가**:
- 특정 함수/클래스가 어디에 정의되어 있는지 탐색할 때
- "이 컴포넌트가 어떻게 동작하는지" 설명이 필요할 때
- 신규 팀원 온보딩 또는 코드 파악 시

**호출 예시**:
```
Use wiki-qa skill: preprocess-server에서 이미지 전처리 파이프라인이
어떻게 구성되어 있는지 설명해줘
```

> ⚠️ **중요**: 이 스킬은 반드시 소스 코드 증거에만 기반하여 답변합니다. 추측이나 외부 지식을 사용하지 않습니다.

---

## 📋 빠른 선택 기준표

| 상황 | 사용할 스킬 |
|---|---|
| 새 기능 개발 시작 | `tdd-workflow` → `tdd-workflows-tdd-red` |
| 실패 테스트 작성 | `tdd-workflows-tdd-red` |
| 최소 구현 작성 | `tdd-workflows-tdd-green` |
| 코드 정리/리팩터링 | `tdd-workflows-tdd-refactor` |
| 명확한 에러 수정 | `debugger` |
| 원인 불명 버그 / 2회 이상 수정 실패 | `systematic-debugging` |
| TypeScript 타입 이슈 | `typescript-expert` |
| React 컴포넌트 설계 | `react-patterns` |
| REST API 설계/검토 | `api-design-principles` |
| 기술 결정 문서화 | `architecture` |
| 코드 구조 파악/탐색 | `wiki-qa` |

---

## 🔗 기존 커스텀 스킬과의 관계

이 스킬들은 프로젝트 전용 스킬인 `.claude/skills/code-review`와 **상호 보완** 관계입니다:

- **개발 중**: `tdd-workflow` 계열 → 구현
- **검토 시**: `code-review` → 품질 기록 (`docs/status/CODE_REVIEW_HISTORY/` 저장)
- **문제 발생 시**: `systematic-debugging` 또는 `debugger` → `tdd-workflows-tdd-red`로 재진입

---

*최초 작성: 2026-02-20 | 스킬 버전: antigravity-awesome-skills v5.8.0*
