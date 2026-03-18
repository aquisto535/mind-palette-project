# 🛡️ Claude Code Rate Limit 서바이벌 가이드

> **목적**: Claude Code Pro 한도 초과(Rate Limit)로 코딩이 강제 중단되었을 때, 멍하게 기다리는 대신 **가장 압축적으로 실력을 키우는 황금 시간**으로 활용하기 위한 실전 대처 매뉴얼입니다.

---

## 🚨 Phase 0: 멈춘 즉시 해야 할 일 (1분 컷)

AI가 멈추면 맥락(Context)도 증발하기 쉽습니다. 다시 켤 때를 위해 흔적을 남기세요.

1. **현재 상태 커밋 (임시)**
   ```bash
   git add .
   git commit -m "wip: [Claude Limit] 진행 중이던 작업 임시 저장"
   ```
2. **Claude 컴백용 프롬프트 메모**
   - 현재 수정 중이던 파일 최상단이나 `TODO.md`에 주석으로 남깁니다.
   - *"Claude가 돌아오면: `test_onnx_conversion.py`의 32번째 줄 Shape 불일치 에러 해결부터 지시할 것"*

---

## 🧠 Phase 1: "Active 읽기" 모드 전환 (학습 가이드 활용)

가장 위험한 행동은 "눈으로만 코드를 훑는 것"입니다. 작성해둔 **실전 학습 가이드**(`ai_server_learning_guide.md`, `preprocess_server_learning_guide.md`)를 열고 **능동적 파괴(Active Destruction)**를 시작하세요. AI가 없어도 할 수 있는 최고의 스터디입니다.

### 🎯 선택 1: 영상처리 (C++) 감각 키우기
방금 전까지 `preprocess-server`를 만졌다면 Visual Studio를 켭니다.

1. `test_main.cpp` 또는 `test_filters.cpp` 하나를 엽니다.
2. **무조건 브레이크포인트(F9)**를 걸고 디버그 모드(F5)로 실행합니다.
3. `F10`을 누르며 한 줄씩 넘어가면서, `cv::Mat`의 내용물(Shape, Type: `CV_8UC1` 등)이 어떻게 변하는지 **변수 창(Watch)**에서 눈으로 확인하세요.
4. **파괴 실험 (L1~L2)**:
   - 가우시안 커널을 홀수가 아닌 `짝수`로 하드코딩 해보세요.
   - `INTER_LINEAR`를 `INTER_NEAREST`로 바꾸고 테스트를 돌려 결과 이미지를 눈으로 확인해 보세요.
   - 에러가 나면, 교재 논문(`영상처리_OpenCV_완전통합표.md`)의 어느 구절 때문인지 매핑합니다.

### 🎯 선택 2: 딥러닝 (Python) 원리 파헤치기
방금 전까지 `ai-server`를 만졌다면 VSCode를 켭니다.

1. `tests/test_model_architecture.py`를 엽니다.
2. **파괴 실험 (L2)**:
   - `model.py`로 가서 `requires_grad = False`를 `True`로 슬쩍 바꿔봅니다.
   - `pytest`를 돌려서 어떤 테스트가 **왜(Why)** 터지는지 확인합니다. (이때 강의 노트 §11 전이학습 개념을 떠올립니다.)
   - `config.py`의 `input_size` 260을 224로 바꿔서 레이어 차원이 무너지는 꼴을 직접 목격합니다.
3. **Logits의 맛 관찰**:
   - `test_inference.py`에서 `print(logits_a)`를 삽입하여, Sigmoid를 통과하기 전의 raw 값들(음수, 양수 뻥튀기 된 값)이 어떻게 생겼는지 구경합니다.

---

## 🛠️ Phase 2: "나라면 어떻게 짰을까?" (L3 제약과 엣지 케이스 고민)

Claude 코드가 멈춘 시간은 사실 **설계자(Architect)로서 고민할 수 있는 유일한 숨통**입니다. 

1. **문서 투어 (Doc Tour)**
   - `docs/ARCHITECTURE_DECISIONS.md`를 열고 최신 ADR(예: ADR-018 HFD 모델 아키텍처)을 읽어봅니다.
   - *"왜 우리는 모델을 4개 Head로 쪼갰을까?"* *"왜 Softmax 대신 Sigmoid를 썼을까?"* 에 대한 해답이 거기 있습니다.
2. **테스트 빈틈 찾기 (Red)**
   - Claude가 짜놓은 테스트 코드들(`tests/`)을 읽어보며, **"내가 악성 사용자라면 이 시스템을 어떻게 터뜨릴까?"** 고민해 봅니다.
   - 예: "만약 C++ 전처리기가 가로 1픽셀, 세로 1픽셀짜리 이미지를 던져주면 Tensor가 어떻게 반응할까?"
   - 깨달은 엣지 케이스가 있다면 테스트 코드 맨 아래에 `def test_1x1_image_handling(): pass` 처럼 껍데기만 만들어 놓습니다. (Claude가 돌아오면 채워달라고 하기 위해)

---

## ⚔️ Phase 3: Claude 부활 시 (Re-ignition)

몇 시간 뒤, (또는 다음 날) Rate Limit이 해제되면 이렇게 지시하며 압도적인 속도로 복귀합니다.

1. 깃 복구: `git reset HEAD~1` (WIP 커밋 취소)
2. **Claude Code 호출**:
   > "내가 빈둥거리는 동안 `test_1x1_image_handling` 이라는 엣지 케이스 테스트 껍데기를 만들어 놨어. 그리고 아까 발생하던 Shape 불일치 에러는 내가 디버거로 확인해보니 Config의 input_size 문제 같아. 이 두 가지를 반영해서 다시 TDD 사이클 이어나가 줘."

---

### 💡 요약: 한도 초과는 끝이 아니라 "복습의 강제 트리거"입니다.
- **막혔다(Block)고 생각하지 마세요.** AI가 코드를 찍어내는 속도가 너무 빠르기 때문에, 사실 이 빈 시간(Downtime)이 없으면 코드가 모두 `남의 코드`가 되어버립니다.
- 한도가 차면 기뻐하십시오. **"아, 드디어 내가 직접 코드를 뜯어보고 흡수할 시간이 주어졌구나"**라고 마인드셋을 전환하는 것이 이 매뉴얼의 핵심입니다.
