# MCP 통합 워크플로우 가이드 (MCP_WORKFLOWS.md)

본 문서는 Mind Palette 프로젝트에서 Shrimp, Sequential Thinking, Context7 MCP 도구들을 유기적으로 결합하여 사용하는 표준 운영 절차(SOP)를 정의합니다.

---

## 🏗️ 1. 신규 기능 구현 워크플로우 (Implementation Pipeline)

복잡한 기능을 처음부터 구현할 때 사용하는 절차입니다.

| 단계 | 도구 | 목적 | 주요 프롬프트 예시 |
| :--- | :--- | :--- | :--- |
| **1. Plan** | **Shrimp** | 작업을 TDD 단위로 분해 | "이 기능을 제1원칙 사고를 적용해 TDD 가능한 최소 단위 작업으로 쪼개줘." |
| **2. Research** | **Context7** | 최신 기술 스택/API 검증 | "이 작업에 사용될 [라이브러리명]의 최신 Best Practice와 Modern C++ 예제를 찾아줘." |
| **3. Design** | **Sequential Thinking** | 논리적 설계 및 제약 식별 | "Context7에서 찾은 정보를 바탕으로, 성능 제약(100ms)을 만족하는 최적의 논리적 설계를 분석해줘." |
| **4. Execute** | **Antigravity** | 코드 작성 (Red-Green) | "분석된 설계를 바탕으로 먼저 실패하는 테스트(Red)를 작성하고, 최소한의 코드로 구현(Green)해." |
| **5. Verify** | **Shrimp** | 작업 완료 검증 | "`verify_task`를 호출해 현재 구현이 인수 조건을 만족하는지 점수화해줘." |

---

## 🔍 2. 코드 리뷰 워크플로우 (Review Pipeline)

제출된 코드의 품질을 다각도로 검토할 때 사용합니다.

### 절차 (AI 에이전트 지침)
1.  **Context7 (규격 검사)**: 사용된 API가 최신 표준(C++17, OpenCV 5.x 등)을 준수하는지 팩트체크합니다.
2.  **Sequential Thinking (논리 검사)**: 코드의 흐름이 `CODE_REVIEW_GUIDELINES.md`의 제1원칙 및 RAII를 준수하는지 심층 분석합니다.
3.  **Shrimp (최종 판정)**: 위 분석 결과를 바탕으로 점수를 매기고, 수정이 필요한 경우 새로운 태스크를 생성합니다.

---

## 💡 활용 팁
- **이관(Hand-over)**: 복잡한 코딩 중 길을 잃었을 때 "Shrimp, 현재까지 완료된 작업과 남은 작업을 정리해줘"라고 요청하여 맥락을 복구하십시오.
- **TDD 강제**: AI가 구현부터 하려 할 때 "Shrimp 계획에 따라 Red 단계부터 진행해"라고 지시하여 품질을 유지하십시오.
- **결정 근거(Rationale)**: 중요한 코드를 짤 때 "Sequential Thinking의 사고 과정을 주석으로 기록해줘"라고 요청하여 추후 코드 리뷰를 용이하게 하십시오.
