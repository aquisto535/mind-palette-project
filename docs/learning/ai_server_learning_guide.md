---
title: "AI Server 딥러닝 실전 학습 가이드"
description: "딥러닝 강의 이론(ai_model_principle.md)을 실제 ai-server Python 코드에서 복습하고 체화하기 위한 제1원칙 기반 3단계 실전 가이드"
---

## 🧠 AI Server 딥러닝 실전 학습 가이드 (제1원칙 기반)

딥러닝 강의(`ai_model_principle.md`)에서 배운 이론을 자신이 직접 구축한 `ai-server` Python 코드로 복습하고 체화하기 위한 실전 가이드입니다.

> **핵심 원칙**: 강의에서 배운 9단계 프로세스는 "왜 모델이 수렴하는가"의 이론입니다. `ai-server`의 코드는 "왜 이 모델이 **지금 이 Shape으로, 이 파라미터로, 이 에러 처리와 함께** 서빙되는가"의 실전입니다. 이 둘을 왕복하면서 **"변조 → 관찰 → 이해"** 사이클을 반복하는 것이 가장 효과적인 학습법입니다.

---

## 📋 코드 ↔ 이론 완전 매핑표

아래 표는 `ai-server`에서 **실제 사용 중인** PyTorch 함수/구조와 강의 이론의 1:1 대응 관계입니다. 학습 시 이 표를 옆에 놓고 코드를 읽으세요.

| 코드 위치 | PyTorch 구현 | 강의 이론 영역 | ai_model_principle 섹션 |
|:---|:---|:---|:---|
| `model.py:27` | `efficientnet_b2(weights=DEFAULT)` | 사전학습 가중치 로드 (Transfer Learning) | §11. 초기화 (Weight Init) |
| `model.py:28` | `backbone_full.features` | Feature Extractor 분리 | §13. CNN 아키텍처 (Conv2d) |
| `model.py:29` | `backbone_full.avgpool` | Global Average Pooling | §13. Pooling (공간 축소) |
| `model.py:32-33` | `param.requires_grad = False` | Backbone 동결 (Transfer Learning) | §11. 전이학습 |
| `model.py:38-43` | `nn.ModuleDict({'head_a': nn.Linear(1408→19), ...})` | 분류기 Head (FC Layer, ModuleDict로 관리) | §1/§8. nn.Module / Linear |
| `model.py:56-58` | `backbone(x) → avgpool → flatten(1)` | Feature 추출 후 1D 변환 | §13. Flatten (3D → 1D) |
| `model.py:61-66` | `heads['head_a'/'b'/'c'/'d'](features)` 반환 | Multi-head Raw Logits 출력 | §2. Loss 계산 전 원시 출력 |
| `preprocessing.py:15` | `transforms.Resize((260, 260))` | 입력 크기 정규화 | §1. 데이터 준비 |
| `preprocessing.py:16` | `transforms.ToTensor()` | [0,255] uint8 → [0,1] float32 | §1. Tensor 변환 |
| `preprocessing.py:17` | `transforms.Normalize(mean, std)` | 채널별 정규화 (평균0, 분산1) | §12. Batch/Layer Norm 원리 |
| `config.py:9` | `input_size: int = 260` | EfficientNet-B2 원논문 입력 크기 | §4. Architecture 설계 |
| `config.py:13` | `backbone_feature_dim: int = 1408` | EfficientNet-B2 피처 차원 | §13. CNN 출력 채널 |
| `config.py:25-26` | `normalize_mean/std = 스케치 데이터셋 통계` | 실제 입력 도메인(아동 연필화) 기반 통계 | §12. 정규화 파라미터 |
| `model_loader.py:18-24` | `male_loaded / female_loaded` (property) | 엔진 상태 추적 (ONNX/TensorRT) | (실전) Graceful Degradation |
| `model_loader.py:51` | `os.path.exists() → engine=None` | 가중치 미존재 시 안전 처리 | (실전) L3 제약 방어 |
| `model_loader.py:27` | `torch.cuda.is_available()` | TensorRT → ONNX Fallback 전략 | §9/§12. Regularization/Norm |


---

## 🔥 Phase 1: 데이터 구조 감각 타격하기 (L1: What - 형태가 올바른가?)

> **목적**: 텐서의 Shape, Type, 차원이 파이프라인 각 단계에서 어떻게 변형되는지 **테스트 코드로 눈으로 확인**하기
> **강의 매핑**: `ai_model_principle.md` **1단계(Tensor)** & **4단계(Architecture)**

교재에서는 "CNN의 출력은 flatten된 1D 벡터"라고 한 줄로 끝나지만, 실전에서는 **feature dimension이 1개라도 틀어지면 Linear Layer가 즉시 크래시**합니다. `ai-server`의 아키텍처 테스트는 이 변환 흐름을 직접 추적하기에 최적의 코드입니다.

### 🚀 실전 행동 지침

#### 실습 1-1: 모델 아키텍처의 물리적 형태 추적 (핵심 ⭐)

`tests/test_model_architecture.py`를 **한 줄씩** 읽으며 다음을 확인하세요:

```text
📍 추적할 지점 (test_model_architecture.py 기준)
────────────────────────────────────────────────
1. test_model_has_backbone        → backbone의 children이 몇 개인가?
                                     efficientnet_b2의 원논문 구조와 대응시켜 보세요
2. test_model_has_four_heads      → nn.Linear 4개가 각각 (1408→19), (1408→14),
                                     (1408→16), (1408→11)인 이유는?
3. test_output_shapes_with_dummy  → (1,3,260,260) 입력이 4개의 출력 텐서로 변환되는
                                     과정에서 차원이 어떻게 줄어드는가?
4. test_backbone_input_features   → 1408이라는 숫자는 어디서 나왔는가?
                                     (EfficientNet-B2 원논문의 feature dimension)
5. test_total_output_items        → 전체 출력 항목 합계가 60인 이유는?
                                     (HFD 인물화 검사 원본 60문항 기준)
```

**질문하며 읽기**:

- *"`config.py:13`의 `backbone_feature_dim: int = 1408`은 강의 노트 섹션 13 CNN에서 배운 `(C,H,W) → flatten → (C×H×W)` 변환의 결과값과 어떻게 대응되는가?"*
- *"강의 노트의 Shape 변환 패턴 `(3,32,32) → Conv+Pool → ... → Flatten → (1024) → Linear → (10)`과 `ai-server`의 `(3,260,260) → EfficientNet → avgpool → flatten → (1408) → Linear → (19)` 패턴의 공통 원리는?"*

#### 실습 1-2: 전처리 파이프라인 Shape 추적

`tests/test_preprocessing.py`를 실행하며 각 단계의 텐서 변환을 직접 확인하세요:

| 테스트 | 입력 | 출력 | 강의 노트 매핑 |
|:---|:---|:---|:---|
| `test_input_image_has_three_channels` | `np.zeros(512,512,3)` uint8 | 채널 3개 확인 | 1단계: Tensor 구조 |
| `test_pipeline_output_shape` | PIL 이미지 (512×512) | `(3, 260, 260)` 텐서 | 1단계: DataLoader 변환 |
| `test_pipeline_output_dtype` | PIL 이미지 | `torch.float32` | 1단계: Tensor dtype |
| `test_pipeline_output_value_range` | 랜덤 이미지 | `[-3.0, 3.0]` 범위 | 3단계: Normalization 효과 |



#### 실습 1-3: 고의로 Shape 망가뜨리기

```python
# preprocessing.py의 Resize를 주석 처리하고 512x512 그대로 넣어보면?
# → model(tensor)에서 Shape mismatch 에러 발생
# 💡 강의 노트 4단계: "입력 크기와 아키텍처가 일치해야 한다"는 원칙의 체감

# config.py의 input_size를 224로 바꿔보면?
# → 모델은 돌아가지만, EfficientNet-B2 원논문의 260과 불일치
# 💡 강의 노트 2단계: 초기화에서 "어디서 시작하느냐"가 성능을 결정함을 실감
```

---

## 🛠️ Phase 2: 변환 로직의 설계 의도 파악하기 (L2: How - 변환이 정확한가?)

> **목적**: 강의에서 배운 알고리즘이 왜 **'이런 구조와 파라미터'**로 코드에 배치되었는지 역추적하기
> **강의 매핑**: `ai_model_principle.md` **2단계(초기화)**, **3단계(정규화)**, **11단계(Transfer Learning)**

### 🚀 실전 행동 지침

#### 실습 2-1: Transfer Learning 동결의 의미 (강의 섹션 11 ↔ 코드)

`tests/test_inference.py`의 3개 테스트가 강의 노트 섹션 11의 핵심을 직접 검증합니다:

| 테스트 | 검증하는 것 | 강의 원리 |
|:---|:---|:---|
| `test_backbone_parameters_frozen` | backbone의 모든 param이 `requires_grad=False` | "이미 성공한 지도를 가져와서 출발지로 사용" |
| `test_head_parameters_trainable` | head의 모든 param이 `requires_grad=True` | "새 도메인에 맞게 미세조정할 부분만 학습" |
| `test_backbone_param_count_vs_head` | backbone 파라미터 > head × 10 | "전이 학습의 경제성: 거대한 지식을 동결하고, 소수만 학습" |



**직접 실험**:

```python
# model.py에서 backbone 동결을 해제해 보세요:
# for param in self.backbone.parameters():
#     param.requires_grad = False  ← 이 줄을 주석 처리

# 그리고 test_backbone_parameters_frozen을 실행하면?
# → 테스트 FAIL. requires_grad=True가 됨
# → 이는 backbone 수백만 개 파라미터가 전부 역전파 대상이 됨을 의미
# 💡 강의 노트 결론의 "보폭의 최적화"와 직결:
#    학습할 필요 없는 것을 학습하면 비효율+과적합
```

#### 실습 2-2: Normalization 파라미터의 실체 (강의 섹션 12 ↔ 코드)

`config.py:25-26`의 `normalize_mean`과 `normalize_std`는 강의에서 배운 정규화의 **구체적 수치**입니다:

```python
# 현재: 스케치 데이터셋 통계값 (ImageNet이 아님!)
# R(Gray): ~0.97, G(Binary): ~0.03, B(Dist): ~0.01
normalize_mean: Tuple[float, float, float] = (0.972, 0.031, 0.012)
normalize_std: Tuple[float, float, float] = (0.156, 0.174, 0.074)
```

**질문하며 읽기**:

- *"강의 노트 3단계에서 `평균 0, 분산 1`로 조정한다고 배웠다. 이 `(0.972, 0.031, 0.012)`은 무엇의 평균인가?"* → 아동 연필화 스케치를 전처리한 3채널 이미지(Gray/Binary/Distance)의 채널별 평균. R채널이 0.97에 가까운 이유는 흰 배경(Gray≈255)이 대부분이기 때문
- *"왜 ImageNet 표준값(0.485, 0.456, 0.406)을 그대로 쓰지 않는가?"* → ImageNet은 자연 사진 기반 통계. 아동 연필화는 채널 구성 자체가 다름(Gray/Binary/Distance). 잘못된 통계로 정규화하면 모델이 학습된 분포와 괴리가 생김
- *"`test_pipeline_uses_config_normalization_params`에서 `mean=0.5, std=0.5`로 바꾸고 흰 이미지를 넣으면 `(255/255 - 0.5) / 0.5 = 1.0`이 나옴을 검증한다. 이 테스트가 왜 중요한가?"* → 하드코딩 시 config 변경에도 파이프라인이 반영 안 되는 버그를 원천 방지

**직접 실험**:

```python
# config.py의 normalize_mean을 ImageNet 값 (0.485, 0.456, 0.406)으로 바꾸고 추론 결과 관찰
# → R채널 평균이 0.97인 데이터를 0.485 기준으로 정규화하면 입력 분포가 크게 왜곡됨
# 💡 강의 노트 3단계:
#    "레이어를 거칠 때마다 데이터 분포가 요동치면 학습이 불안정해진다"의 추론 시점 버전
```

#### 실습 2-3: Sigmoid 배제 결정의 근거 (강의 섹션 2 Loss ↔ 코드)

`test_inference.py`의 `test_forward_returns_logits_not_probabilities`:

```python
# 이 테스트는 forward()가 raw logits(음수 또는 >1 포함)을 반환하는지 검증합니다.
# 왜 forward() 안에 Sigmoid를 넣지 않았을까?

# 강의 노트 5단계(Loss Function) 매핑:
# → BCEWithLogitsLoss는 내부에서 Sigmoid + BCE를 합쳐서 수치적으로 더 안정적
# → forward()에서 Sigmoid를 적용하면 Loss 계산 시 이중 Sigmoid가 걸림
# → 또한 ONNX 변환 시 Sigmoid 위치를 유연하게 제어 가능 (ADR-018 참조)
```

#### 실습 2-4: 결정론적 추론의 의미 (강의 섹션 9 Regularization ↔ 코드)

`test_inference.py`의 `test_deterministic_output_with_fixed_seed`:

```python
# 동일 seed → 동일 입력 → 동일 출력을 보장하는 테스트
# model.eval()이 호출된 상태에서만 이 테스트가 통과합니다.

# 💡 강의 노트 8단계(과적합 방지)에서 배운 Dropout과 연결:
#    model.train() 모드에서는 Dropout이 랜덤하게 뉴런을 끄므로 결과가 달라짐
#    model.eval() 모드에서는 Dropout이 비활성화되어 결정론적 출력이 보장됨
#    → 추론 서버에서는 반드시 eval() 모드로 전환해야 하는 이유
```

---

## 🛡️ Phase 3: 제약과 검증의 시뮬레이터 (L3: Why - 경계에서도 안전한가?)

> **목적**: 강의에서는 다루지 않는 **실전 장애 시나리오, 서비스 안정성, 배포 최적화**를 코드에서 체화하기
> **강의 매핑**: `ai_model_principle.md` **8단계(과적합 방지)** & **결론(Trade-off)**

### 🚀 실전 행동 지침

#### 실습 3-1: 모델 없이도 서버가 살아남는 구조 (Graceful Degradation)

`tests/test_health.py`에서 가장 중요한 테스트:

```python
# test_health_server_runs_without_model: 가중치 파일이 없어도 200 OK
# test_health_model_loaded_false_when_no_weights: models.male==False, models.female==False

# 💡 강의에서 배우지 않는 것: "학습이 끝나도 .onnx 파일이 있다는 보장은 없다"
# 프로덕션에서는 경로 오타, 디스크 고장, 컨테이너 볼륨 미마운트 등이 빈번
# model_loader.py의 load_models()가 os.path.exists() 확인 후 engine=None 상태를
# 유지하는 패턴이 이 제약을 방어 (ADR-027: ONNX/TensorRT 이중 Fallback)
```

**코드 역추적**:

```text
test_health.py:app_no_models fixture
  → ModelConfig(male_onnx_path="nonexistent/male.onnx")
    → create_app(model_config=config)
      → load_models(config)
        → os.path.exists() == False → engine 미할당
          → ModelState(male_engine=None) → male_loaded == False
            → /health → {"models": {"male": false}} + 200 OK
```

#### 실습 3-2: E2E 추론 테스트에서 배치 처리 검증

`tests/test_e2e_real_image.py`의 `test_real_image_batch_inference`:

```python
# batch_size=4로 동일 이미지 4장을 묶어서 추론 → Shape (4, 19), (4, 14) 등 검증

# 💡 강의 노트 1단계(DataLoader)에서 배운 batch_size의 실전 적용:
# "배치가 클수록 GPU 활용도가 높지만, 메모리 한계가 있다"
# → plan.md L3의 "GPU OOM 시뮬레이션 테스트"와 직결
```

#### 실습 3-3: 남녀 모델 분리 로딩의 설계 의도 (아키텍처 관점)

`model_loader.py`의 `ModelState`는 데이터클래스로 남녀 추론 엔진을 독립 관리합니다:

```python
@dataclass
class ModelState:
    male_engine: Optional[Any] = None    # 남아용 엔진 (ONNX 또는 TensorRT)
    female_engine: Optional[Any] = None  # 여아용 엔진 (ONNX 또는 TensorRT)
    engine_type: str = "none"            # "tensorrt" | "onnx" | "none"

    @property
    def male_loaded(self) -> bool:
        return self.male_engine is not None
```

**질문하며 읽기**:

- *"왜 모델을 하나로 합치지 않고 남녀로 분리했는가?"* → HFD 인물화 검사의 성별별 채점 기준이 다르기 때문
- *"한쪽 엔진만 로드 실패하면 다른 쪽은 정상 서비스가 가능한가?"* → `load_models()`에서 남녀 경로를 독립적으로 `os.path.exists()` 확인하므로 가능
- *"`engine_type`이 `"none"`인 상태에서 추론을 요청하면 어떻게 되는가?"* → `male_loaded == False`이므로 `/analyze` 엔드포인트에서 503 또는 적절한 에러 응답
- *"이 패턴은 강의의 어떤 원리와 대응되는가?"* → 섹션 8의 Regularization 관점에서 도메인 분리는 과적합 방지 전략의 일종

---

## 📈 추천 학습 순서

### Week 1: L1 (데이터 구조) — "눈으로 확인하기"

```text
1일차: test_model_architecture.py 전체 실행 + 각 assert문과 강의 섹션 13 아키텍처 대응
2일차: test_preprocessing.py 실행 + 전처리 전/후 텐서 Shape 직접 print해서 관찰
3일차: 고의 파괴 실험 (input_size를 224로 변경, 정규화 제거 등 → 에러 관찰)
```

### Week 2: L2 (변환 로직) — "왜 이 설계 결정인가?"

```text
1일차: test_inference.py 실행 + requires_grad 동결/해제 실험 + 강의 섹션 11 복습
2일차: Normalize 파라미터 변경 실험 + config 분리 이유 이해 + 강의 섹션 12 복습
3일차: forward() Logits vs Sigmoid 비교 + BCEWithLogitsLoss 수치 안정성 이해 + 강의 섹션 2 복습
4일차: E2E 테스트 (test_e2e_real_image.py) 실행 + 전체 파이프라인 흐름 추적
```

### Week 3: L3 (제약/시스템) — "프로덕션에서 살아남기"

```text
1일차: test_health.py 실행 + model_loader.py Fallback 구조 분석
2일차: plan.md에 남은 L3 항목 중 하나 직접 TDD Red 작성 시도 (예: 비정상 입력 테스트)
3일차: plan.md Step 2(ONNX) 리서치 → "Logits 반환 구조가 ONNX 변환에 왜 유리한가"
4일차: 강의 노트 전체 복습 + 코드와의 매핑 완성 → 종합 정리 문서 작성
```

---

## 💡 요약: "코드를 어떻게 읽고 성장할 것인가?"

절대 단순히 위에서 아래로 스크롤하며 코드를 읽지 마세요.

1. **`tests/` 폴더를 가장 먼저 엽니다.** 테스트는 "이 코드가 무엇을 보장하는가"의 명세서입니다.
2. 테스트 함수 하나를 고르고, 그 내부에서 호출되는 `src/`의 객체 흐름을 타고 들어갑니다.
3. 머릿속의 강의 지식(예: Transfer Learning의 동결, Normalization의 mean/std)이 **코드의 어느 줄에 어떤 형태로 숨어있는지** 1:1로 매핑합니다.
4. **상단 매핑표를 참조하세요.** 이 문서의 "코드 ↔ 이론 완전 매핑표"가 네비게이션 역할을 합니다.
5. **값을 변조하고 테스트를 강제로 실패(Red)시켜 보세요.** AI 모델도 영상처리와 마찬가지로 **고의로 하이퍼파라미터와 구조를 망가뜨려 봐야** 그것이 어떤 수학적 의미를 갖는지 꿰뚫을 수 있습니다.

---

## 📚 참고 자료

| 자료 | 경로 | 용도 |
|:---|:---|:---|
| 딥러닝 강의 핵심 요약 | `docs/reference/AI/ai_model_principle.md` | 이론 복습 (9단계 프로세스) |
| 프로젝트 개발 계획 | `plan.md` (Phase 4) | TDD 체크리스트 및 미완성 항목 확인 |
| 전처리 서버 학습 가이드 | `docs/learning/preprocess_server_learning_guide.md` | 동일 방법론의 C++ 파이프라인 버전 |
| 모델 아키텍처 테스트 | `ai-server/tests/test_model_architecture.py` | L1 학습 시작점 |
| 추론 검증 테스트 | `ai-server/tests/test_inference.py` | L2 학습 시작점 |
| 전처리 파이프라인 테스트 | `ai-server/tests/test_preprocessing.py` | 정규화/Shape 변환 학습 |
| 헬스 체크 테스트 | `ai-server/tests/test_health.py` | L3 Graceful Degradation 학습 |
| E2E 통합 테스트 | `ai-server/tests/test_e2e_real_image.py` | 전체 파이프라인 흐름 추적 |
| 모델 설정 (Config) | `ai-server/src/config.py` | 하이퍼파라미터 중앙 관리 |
| 모델 로더 | `ai-server/src/infra/model_loader.py` | 장애 방어 패턴 |
