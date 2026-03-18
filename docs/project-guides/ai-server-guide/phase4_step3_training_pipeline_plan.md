# AI 학습 파이프라인 구축 계획

## Context (왜 이 작업이 필요한가)

현재 AI 서버는 **인프라만 완성**된 상태입니다. 모델 구조(EfficientNet-B2 + 4Head), ONNX 변환, FastAPI 서버가 모두 갖춰져 있지만, **학습된 가중치가 없어서** `/analyze` 엔드포인트가 랜덤 숫자를 반환하고 있습니다.

이 계획은 "껍데기 → 실제 AI"로 전환하기 위한 **데이터셋 구축 + 학습 파이프라인** 작업입니다.

---

## 전체 흐름 한눈에 보기

```
[Step 0] 데이터 준비 (수동)
    스캔 이미지 10장 → 그림 20개 크롭 + 채점표에서 60문항 레이블 읽기
         │
[Step 1] 문항↔Head 매핑 모듈         [Step 7] IQ 점수 계산 모듈
    "문항 1번 = head_a[0]"              "원점수 40 → IQ 122 → 상위 93%"
         │                                        │
[Step 2] Dataset 클래스              [Step 3] 데이터 증강
    이미지+레이블 로드                  회전/이동/뒤집기로 20장→수백장
         │                                  │
[Step 4] 합성 데이터 생성기 (개선)          │
    막대인간 500장 + 자동 레이블           │
         │                                  │
         └──────────┬───────────────────────┘
                    │
[Step 5] 학습 루프 (train.py)
    Phase A: 합성 데이터로 사전학습 (50 epoch)
    Phase B: 실제 데이터로 미세조정 (100 epoch, LOOCV)
                    │
[Step 6] 평가 모듈
    Head별 정확도, F1, 원점수 오차 측정
                    │
[Step 9] 모델 내보내기 (.pt → .onnx)
                    │
[Step 10] model_loader.py 버그 수정
                    │
[Step 8] /analyze 엔드포인트 연동
    랜덤값 → 실제 60문항 추론결과 + IQ + 백분위
```

---

## Step 0: 데이터 준비 (수동 작업)

### 할 일
1. 스캔 이미지 10장에서 **그림 영역만 크롭** (총 ~20개)
2. 각 그림의 **채점표에서 O 표시를 읽어** JSON으로 수동 기록
3. 메타데이터(아동 성별, 그림 성별, 연령, 원점수) 함께 기록

### 디렉토리 구조
```
ai-server/data/
  raw/                          # 크롭된 원본 그림들
    sample_001.png              # 남아-남자상 MA=10-6
    sample_002.png              # ...
  labels/
    annotations.json            # 모든 레이블 (아래 형식)
```

### 레이블 JSON 형식
```json
{
  "samples": [{
    "id": "sample_001",
    "image_path": "raw/sample_001.png",
    "child_gender": "male",
    "figure_gender": "male",
    "child_age": 10,
    "raw_score": 29,
    "items": { "1": 1, "2": 0, "3": 1, ..., "60": 0 },
    "head_labels": {
      "head_a": [1, 1, 0, ...],  // 19개
      "head_b": [0, 1, 1, ...],  // 14개
      "head_c": [1, 0, 0, ...],  // 16개
      "head_d": [1, 1, 0, ...]   // 11개
    }
  }]
}
```

### 검증 규칙
- `sum(items.values()) == raw_score` (레이블 합계 = 기록된 원점수)

---

## Step 1: 문항-Head 매핑 모듈

**새 파일**: `ai-server/src/core/item_mapping.py`

60문항이 어떤 Head에 매핑되는지 코드로 정의합니다.

```
Head A (머리/얼굴 19개): 문항 1,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21
Head B (몸통/비례 14개): 문항 2,3,29,30,40,41,42,43,44,45,46,47,48,49
Head C (사지/말단 16개): 문항 22,23,24,25,26,27,28,31,32,33,34,35,36,37,38,39
Head D (의복/질적 11개): 문항 50,51,52,53,54,55,56,57,58,59,60
```

- 여자척도(Female Scale) 예외 6문항도 여기서 관리
- `items_to_head_labels(items_dict, scale) → head_labels_dict` 변환 함수

**TDD**: 총 문항 수 60개 검증, Head별 크기 검증, 남녀 척도 차이 검증

---

## Step 2: Dataset 클래스

**새 파일**: `ai-server/src/core/dataset.py`

```python
class HFDDataset(Dataset):
    def __init__(self, annotations_path, transform, figure_gender=None):
        # annotations.json 로드
        # figure_gender로 필터링 (남자상/여자상 모델 분리 학습용)

    def __getitem__(self, idx):
        return image_tensor, {"head_a": tensor, "head_b": tensor, ...}
```

**TDD**: 출력 shape 검증 (3,260,260), 레이블 이진값 검증, 성별 필터링 검증

---

## Step 3: 데이터 증강 파이프라인

**새 파일**: `ai-server/src/core/augmentation.py`

스케치 이미지에 적합한 증강만 선별:

| 증강 | 설정 | 근거 |
|------|------|------|
| RandomAffine | 회전 ±15°, 이동 10%, 스케일 85~115% | 스캔 각도/위치 차이 |
| HorizontalFlip | p=0.5 | 왼손잡이 아동의 거울상 그림 |
| RandomPerspective | 왜곡 10% | 종이 평면 아닌 경우 |
| **NO** VerticalFlip | - | 사람이 뒤집힌 건 다른 그림 |
| **NO** ColorJitter | - | 스케치는 흑백, 색 변환 무의미 |
| fill=255 (흰색) | - | 스케치 배경 = 흰색 |

**효과**: 20장 × 약 20배 증강 = **에폭당 ~400장** 학습 효과

---

## Step 4: 합성 데이터 생성기 (개선)

**수정 파일**: `ai-server/scripts/generate_synthetic_data.py`

현재 "랜덤 선" → **"막대인간(stick figure) + 자동 레이블"** 로 업그레이드

```
기존: 랜덤 직선 5~15개 → 레이블 없음
개선: 머리 원 + 몸통 사각형 + 팔/다리 선 → 그린 부위에 맞는 레이블 자동 생성
```

- 500~1000장 생성하여 **사전학습(Pre-training)** 에 사용
- 난이도 조절: 단순(머리+몸통만) ~ 복잡(손가락, 옷, 세부묘사)

---

## Step 5: 학습 루프 (핵심)

**새 파일**: `ai-server/scripts/train.py`

### 2단계 학습 전략

```
┌─────────────────────────────────────────────────┐
│  Phase A: 합성 사전학습 (Pre-training)            │
│  데이터: 합성 막대인간 500장                        │
│  Epochs: 50 | LR: 1e-3 | 증강: 보통              │
│  목적: "신체 부위 유무" 기초 개념 학습               │
├─────────────────────────────────────────────────┤
│  Phase B: 실제 데이터 미세조정 (Fine-tuning)        │
│  데이터: 실제 아동화 ~20장 (LOOCV)                  │
│  Epochs: 100 | LR: 1e-4 (10배 낮춤) | 증강: 강하게  │
│  목적: 실제 그림 특성에 적응                        │
│  조기종료: patience=15                             │
└─────────────────────────────────────────────────┘
```

### 핵심 설정

| 항목 | 설정 | 근거 |
|------|------|------|
| Loss | `BCEWithLogitsLoss` | 60개 독립 이진 분류 (multi-label) |
| Optimizer | `AdamW` (weight_decay=1e-4) | 소규모 데이터셋 과적합 방지 |
| Scheduler | `CosineAnnealingLR` | 학습률 부드럽게 감소 |
| Batch size | 4 | 데이터 적으므로 작은 배치 |
| 학습 대상 | **Head(Linear) 레이어만** | Backbone(EfficientNet-B2)은 동결 유지 |

### 검증 방법: Leave-One-Out Cross-Validation (LOOCV)

20장밖에 없으므로 일반적인 train/test 분할은 무의미합니다.

```
Round 1: 19장 학습, 1장 검증 → 원점수 오차 기록
Round 2: 19장 학습, 1장 검증 → 원점수 오차 기록
...
Round 20: 19장 학습, 1장 검증 → 원점수 오차 기록
→ 평균 오차 산출
```

`ai-server/src/config.py`에 `TrainingConfig` 클래스 추가

**TDD**: loss 감소 확인, backbone gradient=0 확인, 체크포인트 저장/로드 검증

---

## Step 6: 평가 모듈

**새 파일**: `ai-server/src/core/evaluate.py`

| 메트릭 | 설명 |
|--------|------|
| Head별 Accuracy | 각 Head의 이진 분류 정확도 |
| Head별 F1 Score | 불균형 데이터 대응 |
| 원점수 MAE | 예측 원점수와 실제 원점수의 평균 절대 오차 |
| Threshold 최적화 | Head별 최적 임계값 탐색 (기본 0.5가 아닐 수 있음) |

**TDD**: LOOCV 결과 구조 검증, 원점수 계산 정확성 검증

---

## Step 7: IQ 점수 계산 모듈

**새 파일**: `ai-server/src/core/iq_scorer.py`

규준 데이터(`전국 규준 통계치와 백분위 매핑 데이터.md`)를 코드로 구현합니다.

```
입력: 원점수=40, 연령=10, 아동성별=남, 그림성별=남
  ↓
규준 조회: M=28.8, SD=7.6 (전국 남아, 만 10세, 남자상)
  ↓
IQ 산출: 100 + 15 × ((40 - 28.8) / 7.6) = 122
  ↓
백분위 조회: IQ 122 → 상위 93%
  ↓
출력: { "iq": 122, "percentile": 93, "raw_score": 40 }
```

- 지원 연령: 만 3~13세
- 지원 조합: 남아/여아 × 남자상/여자상 (4가지)
- IQ 범위: 67~133 (규준표 범위로 클램핑)

**TDD**: 문서 예시(만10세 남아 40점 → IQ 122) 검증, 평균 점수→IQ 100 검증, 범위 외 클램핑 검증

---

## Step 8: `/analyze` 엔드포인트 연동

**수정 파일**: `ai-server/src/routes/analyze.py`

현재 (lines 125-138):
```python
"score": random.randint(75, 98)  # ← 삭제
```

변경 후 응답:
```json
{
  "items": { "1": 1, "2": 0, ..., "60": 1 },
  "head_scores": { "head_a": 12, "head_b": 8, "head_c": 10, "head_d": 5 },
  "raw_score": 35,
  "iq": 113,
  "percentile": 81,
  "child_info": { "age": 10, "child_gender": "male", "figure_gender": "male" },
  "date": "2026-03-16"
}
```

- 추가 입력 파라미터: `age`, `child_gender`, `figure_gender`
- ONNX 추론 → sigmoid → threshold → 60문항 이진결과 → IQ 계산

---

## Step 9: 모델 내보내기

**새 파일**: `ai-server/scripts/export_model.py`

학습 완료된 `.pt` → `.onnx` 변환. 기존 `ai-server/src/core/onnx_converter.py`의 `export_to_onnx()`를 재사용합니다.

---

## Step 10: model_loader.py 버그 수정

**수정 파일**: `ai-server/src/infra/model_loader.py`

현재 **중복된 HFDClassifier stub** (빈 껍데기)이 정의되어 있고, `load_state_dict` 호출이 주석 처리되어 있습니다.

```python
# 수정 전 (line 13-18): 빈 HFDClassifier 정의 → 삭제
# 수정 후: from src.core.model import HFDClassifier

# 수정 전 (line 56): load_state_dict 주석 처리 → 활성화
```

---

## 예상 성능과 한계 (정직한 기대치)

| 지표 | 예상값 | 설명 |
|------|--------|------|
| 문항별 정확도 | 65~80% | 랜덤(50%)보다 확실히 좋지만 프로덕션 수준은 아님 |
| 원점수 MAE | ±5~8점 | 60점 만점 기준 |
| 잘 맞는 항목 | 머리, 눈, 동체 존재 여부 | 시각적으로 뚜렷한 특징 |
| 못 맞는 항목 | 비율, 대칭성, 질적 수준 | 미세한 판단이 필요한 항목 |

> 20개 샘플로는 프로덕션 수준 정확도는 달성 불가. 하지만 **"실제 AI 추론이 동작하는 엔드투엔드 파이프라인"**을 증명하는 것이 포트폴리오 목표이므로 충분합니다.

---

## 파일 변경 요약

### 새로 생성 (10개)
| 파일 | 용도 |
|------|------|
| `src/core/item_mapping.py` | 60문항 ↔ 4 Head 매핑 |
| `src/core/dataset.py` | HFDDataset (PyTorch Dataset) |
| `src/core/augmentation.py` | 스케치 전용 데이터 증강 |
| `src/core/evaluate.py` | 평가 메트릭 (Accuracy, F1, LOOCV) |
| `src/core/iq_scorer.py` | IQ/백분위 계산 |
| `scripts/train.py` | 2단계 학습 루프 |
| `scripts/export_model.py` | .pt → .onnx 내보내기 |
| `tests/test_item_mapping.py` | 매핑 테스트 |
| `tests/test_dataset.py` | 데이터셋 테스트 |
| `tests/test_iq_scorer.py` | IQ 계산 테스트 |

### 수정 (4개)
| 파일 | 변경 내용 |
|------|-----------|
| `src/routes/analyze.py` | 랜덤 응답 → 실제 추론 + IQ 결과 |
| `src/infra/model_loader.py` | 중복 HFDClassifier 제거, load_state_dict 활성화 |
| `src/config.py` | TrainingConfig 클래스 추가 |
| `scripts/generate_synthetic_data.py` | 막대인간 + 자동 레이블 생성기로 업그레이드 |

### 데이터 (수동 작업)
| 파일 | 용도 |
|------|------|
| `data/raw/*.png` | 크롭된 아동화 ~20장 |
| `data/labels/annotations.json` | 수동 채점 레이블 |

---

## 검증 방법 (End-to-End)

1. **단위 테스트**: `pytest ai-server/tests/` — 모든 새 모듈의 TDD 테스트 통과
2. **학습 검증**: `python scripts/train.py` — loss가 감소하고, LOOCV 결과 출력
3. **모델 파일 생성 확인**: `models/mind_palette_male.pt` + `.onnx` 존재
4. **통합 테스트**: `/analyze`에 실제 이미지 POST → 60문항 결과 + IQ + 백분위 JSON 반환
5. **기존 테스트 회귀 없음**: 기존 `test_health.py`, `test_analyze.py` 등 모두 통과

---

## 실행 순서 (의존성 기반)

```
병렬 가능 ─┬─ Step 0: 데이터 준비 (수동)
           ├─ Step 1: item_mapping.py
           ├─ Step 3: augmentation.py
           └─ Step 7: iq_scorer.py
               │
순차 실행 ─── Step 2: dataset.py (← Step 0, 1 필요)
           ── Step 4: 합성 데이터 생성기 (← Step 1 필요)
           ── Step 5: train.py (← Step 2, 3, 4 필요)
           ── Step 6: evaluate.py (← Step 2, 5 필요)
           ── Step 10: model_loader.py 수정
           ── Step 9: export_model.py (← Step 5 필요)
           ── Step 8: analyze.py 연동 (← Step 7, 9, 10 필요)
```
