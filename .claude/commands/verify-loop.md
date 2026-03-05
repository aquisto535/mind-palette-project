---
description: TDD Red-Green-Refactor 사이클을 반복 실행하며 구현 완성도를 검증
---

# Verify Loop — TDD 사이클 검증 루프

구현이 완료될 때까지 Red-Green-Refactor 사이클을 체계적으로 반복합니다.

## 루프 실행 순서

### 🔴 Phase 1: Red (테스트 실패 확인)
1. `/tdd-workflows-tdd-red $ARGUMENTS` 스킬로 실패 테스트 작성
2. 테스트 실행 → 실패 확인
3. 실패 이유가 "구현 없음"인지 확인 (문법 에러 X)

**C++ 테스트 실행:**
```bash
cd build && ctest --output-on-failure
```

**Python 테스트 실행:**
```bash
pytest -v --tb=short
```

**TypeScript 테스트 실행:**
```bash
npm test -- --watchAll=false
```

---

### 🟢 Phase 2: Green (최소 구현)
1. `/tdd-workflows-tdd-green $ARGUMENTS` 스킬로 최소 구현 작성
2. 테스트 실행 → 통과 확인
3. 모든 이전 테스트도 통과하는지 확인 (회귀 없음)

---

### 🔵 Phase 3: Refactor (코드 개선)
1. `/tdd-workflows-tdd-refactor $ARGUMENTS` 스킬로 리팩터링
2. 테스트 실행 → 여전히 통과하는지 확인
3. `/refactor-validator` 스킬로 Tidy First 원칙 검증

---

### 🔁 반복 결정
- **다음 기능** → Phase 1부터 반복
- **모든 기능 완료** → `/quick-commit`으로 커밋 준비

---

## 현재 루프 상태 추적

| 단계 | 상태 | 메모 |
|------|------|------|
| Red | ⬜ | |
| Green | ⬜ | |
| Refactor | ⬜ | |

---

> 💡 **Tidy First 원칙**: 구조적(Structural) 변경과 기능적(Behavioral) 변경은 항상 별도 커밋으로 분리하세요.

$ARGUMENTS
