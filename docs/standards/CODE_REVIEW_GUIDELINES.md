# Mind Palette Code Review Guidelines

본 문서는 Mind Palette 프로젝트의 코드 품질을 유지하고, 기술적 부채를 최소화하며, 모든 팀원이 프로젝트의 핵심 원칙을 공유하기 위한 가이드라인입니다.

## 1. 핵심 철학 (Core Philosophy)

### 🧪 TDD (Test Driven Development)
*   **Red**: 항상 실패하는 테스트가 먼저 작성되었는가?
*   **Green**: 테스트를 통과하기 위한 **최소한의 코드**만 작성했는가? (미래를 예측한 과잉 구현 금지)
*   **Refactor**: 기능 변경 없이 구조만 개선했는가?

### 🧹 Tidy First (정돈 우선)
*   **구조적 변경(Structural)**과 **기능적 변경(Behavioral)**이 명확히 분리되어 있는가?
*   절대 한 커밋/Step에 두 가지를 섞지 않는다. 구조 개선이 필요하면 구조 개선만 먼저 수행한다.

### 🧠 제1원칙 사고 (First Principles Thinking)
*   **Rationale (이유)**: "왜 이 방식을 선택했는가?"에 대해 근본적인 진실(논리적/물리적 팩트)로 설명할 수 있는가?
*   **Alternatives (대안)**: 다른 더 효율적인 방법은 없었는가? "관행이라서" 혹은 "원래 그래서"라는 답변은 배제한다.
*   **Optimization**: 백지상태에서 시작했을 때 가장 최적의 해결책인가?

### ✨ 클린 코드 (Clean Code)
*   **SOLID**: 객체지향 설계 원칙을 준수하는가?
*   **DRY (Don't Repeat Yourself)**: 중복이 제거되었는가?
*   **KISS (Keep It Simple, Stupid)**: 불필요하게 복잡하지 않고 단순한가?
*   **Readability**: 변수와 함수 이름만으로 의도가 명확히 전달되는가?

---

## 2. 언어별 체크리스트 (Language Specifics)

### ⚛️ React (Frontend)
*   [ ] 함수형 컴포넌트와 Hooks를 올바르게 사용하는가?
*   [ ] 컴포넌트가 단일 책임(SRP)을 가지며 재사용 가능한가?
*   [ ] 불필요한 리렌더링이 발생하지 않도록 최적화되었는가?
*   [ ] 디자인 시스템의 일관성을 유지하고 반응형 레이아웃을 지원하는가?

### 🟢 Node.js (API Gateway)
*   [ ] `Async/Await`와 에러 핸들링이 적절히 처리되었는가?
*   [ ] 비즈니스 로직이 `server.js` 등에 몰려있지 않고 모듈화되었는가?
*   [ ] `Winston`을 이용한 구조화된 로깅이 포함되었는가?
*   [ ] 보안(Input Validation, Rate Limiting 등) 고려사항이 반영되었는가?

### 🔵 C++ (Preprocess Server)
*   [ ] **C++17 표준**을 준수하는가? (`unique_ptr`, `nullptr`, `auto` 등)
*   [ ] **RAII**를 통해 자원 누수가 원천 차단되었는가? (`new`/`delete` 금지)
*   [ ] `spdlog`를 통한 성능 및 오류 로깅이 적절한가?
*   [ ] OpenCV 알고리즘의 시간 복잡도와 효율성을 검토했는가?

### 🐍 Python (AI Server)
*   [ ] **PEP 8** 스타일 가이드를 준수하는가?
*   [ ] FastAPI 및 PyTorch/ONNX Runtime 최적화 기법을 활용하는가?
*   [ ] 추론 파이프라인에서 메모리 및 성능 병목이 없는가?
*   [ ] `structlog`를 이용해 추론 시간 및 결과를 구조화하여 기록하는가?

---

## 3. 리뷰 이력 관리
모든 코드 리뷰 결과는 [CODE_REVIEW_HISTORY](../status/CODE_REVIEW_HISTORY/)에 기록하여 중복 리뷰를 방지하고 품질 이력을 추적합니다.
