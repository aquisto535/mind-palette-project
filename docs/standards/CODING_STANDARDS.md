# Mind Palette 코딩 표준 (SSOT)

> 이 문서는 프로젝트 전체의 **단일 진실 소스(Single Source of Truth)**입니다.
> 모든 AI 도구(`CLAUDE.md`, `AGENTS.md`)는 이 문서를 참조합니다.

---

## 1. TDD & Tidy First 방법론

### 최우선 행동 강령
1. **Test First**: 절대 구현 코드를 먼저 작성하지 마십시오. 항상 실패하는 테스트(Red)를 먼저 작성해야 합니다.
2. **Minimal Implementation**: 테스트를 통과할 수 있는 **최소한의 코드**만 작성하세요.
3. **Separation of Concerns**: 구조적 변경(리팩터링)과 기능적 변경(로직 추가)을 절대 같은 커밋/단계에 섞지 마십시오.
4. **Kent Beck Persona**: 켄트 벡(Kent Beck)의 원칙을 따르는 시니어 엔지니어로서 행동하십시오.

### TDD 사이클: Red → Green → Refactor
- 기능의 작은 증분을 정의하는 실패하는 테스트를 작성하는 것으로 시작
- 행동을 설명하는 의미 있는 테스트 이름 사용 (예: `shouldSumTwoPositiveNumbers`)
- 테스트를 통과시키는 데 딱 필요한 만큼의 코드만 작성
- 테스트가 통과하면, 리팩터링이 필요한지 고민
- 결함 수정 시: API 수준 실패 테스트 → 최소 단위 테스트 → 두 테스트 모두 통과

### 정돈 우선 (Tidy First) 접근법
- **구조적 변경 (STRUCTURAL)**: 행동을 변경하지 않고 코드 재배치
- **기능적 변경 (BEHAVIORAL)**: 실제 기능을 추가하거나 수정
- 두 가지가 모두 필요할 때는 항상 **구조적 변경을 먼저** 수행
- 전후로 테스트를 실행하여 구조적 변경이 행동을 바꾸지 않았음을 검증

### 리팩터링 가이드라인
- 테스트가 통과하는 상태("Green" 단계)에서만 리팩터링
- 한 번에 하나의 리팩터링 변경만 수행
- 각 리팩터링 단계 후에 테스트 실행

---

## 2. C++ 개발 가이드라인 (C++17)

### 필수 문법 (C++11/14)
- **Smart Pointers**: `new`/`delete` 금지 → `std::unique_ptr`, `std::shared_ptr`, `std::make_unique<T>()` 사용
- **Nullptr**: `NULL` → `nullptr`
- **Enum Class**: `enum` → `enum class`
- **Override**: 가상 함수 재정의 시 `override` 필수
- **Range-based for**: `for (const auto& item : container)`
- **Auto**: 타입 추론이 명확한 경우 `auto` 사용

### 권장 문법 (C++17)
- Structured Binding: `auto [x, y] = point;`
- 파일 경로: `std::filesystem`
- Nested Namespace: `namespace A::B { ... }`
- if init: `if (auto it = m.find(key); it != m.end())`

---

## 3. 커밋 규율

- **구조적 변경**과 **기능적 변경**은 별도 커밋
- 두 가지가 모두 필요하면 구조적 변경을 먼저 수행
- 커밋 메시지에 구조적/기능적 변경 여부를 명시
- 모든 테스트 통과, 컴파일러/린터 경고 해결 후에만 커밋
- 크고 드문 커밋보다 작고 빈번한 커밋 권장

---

## 4. 코드 품질 표준

- 중복을 무자비하게 제거
- 네이밍과 구조를 통해 의도를 명확히 표현
- 의존성을 명시적으로
- 메서드는 작게 유지, 단일 책임에 집중
- 상태와 부수 효과(side effects) 최소화
- 작동 가능한 가장 단순한 해결책 사용

---

## 5. 지식 설명 가이드 (First Principles Thinking)

복잡한 문제 해결 시 아래 5단계를 적용하세요:

1. **Deconstruct (분해)**: 근본적인 진실(물리적/논리적 팩트)로 잘게 쪼개기
2. **Remove Assumptions (가정 제거)**: "원래 그렇다"는 고정관념 배제
3. **Optimize (최적해 탐색)**: 백지상태에서 가장 효율적인 방법 탐색
4. **Identify Constraints (제약 식별)**: 현실적 제약(비용, 시간, 자원) 명확히
5. **Reconstruct (재구축)**: 직관적인 비유나 논리로 재설명

---

## 6. 데이터 흐름 기반 3단계 프레임워크

| Level | 질문 | 설명 | 검증 대상 |
|-------|------|------|-----------|
| **L1: 데이터 구조 (What)** | "형태가 올바른가?" | 입력/출력/상태의 Shape, Type, 존재 여부 | 타입, 필드, 차원, dtype |
| **L2: 변환 로직 (How)** | "변환이 정확한가?" | 입력 → 출력 변환 과정의 정확성 | 알고리즘 결과, 동등성, 불변 조건 |
| **L3: 제약과 검증 (Why)** | "경계에서도 안전한가?" | 비정상 입력, 성능 한계, 경계 조건 | 에러 처리, 성능 회귀, 리소스 한계 |

- 요구사항 분해: L1 → L2 → L3 순서
- TDD Red 작성: L1(구조) → L2(로직) → L3(경계) 순서로 신뢰도 축적

---

## 7. Gotchas

- Windows 빌드 시 vcpkg triplet은 `x64-windows-static` (DLL 이슈 방지)
- OpenCV 의존성은 vcpkg.json으로 관리 — 수동 설치 금지
