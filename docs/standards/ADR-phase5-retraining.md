# ADR: Phase 5+ 모델 재학습 전략 — C++ 전처리 파이프라인 통합

## 상태
**Proposed** (Phase 4 완료 후, Phase 5에서 실행 예정)

## 맥락

### 현재 Train-Inference Mismatch 문제

**학습 시**:
```
data/raw/sample_001.png (원본 스캔 크롭)
         ↓
  torchvision.transforms.Resize(260) + Normalize
  (mean=(0.972, 0.031, 0.012), std=(0.156, 0.174, 0.074))
         ↓
  모델이 RGB 원본 이미지로 학습
```

**추론 시 (실제 서비스)**:
```
사용자 업로드 이미지
         ↓
  C++ HybridPipeline
  (Denoise → GetContentROI(Dominant) → Letterbox → 3채널 합성)

  R채널 = Gray (필압/음영)
  G채널 = InvertedBinary (형태/윤곽선)
  B채널 = DistanceTransform (선의 골격)
         ↓
  모델이 합성 3채널 이미지로 추론
```

**문제**: 학습 데이터 형식 ≠ 추론 시 입력 형식

---

## 의사결정

### Phase 5+ 실행 조건

Phase 5+ 재학습은 다음 조건을 **모두 만족**할 때 실행합니다:

1. ✅ **C++ 전처리 파이프라인 안정화** (Phase 3 완료)
   - HybridPipeline 필터 파라미터 확정
   - GetContentROI 로직 확정 (현재: Dominant only)

2. ✅ **AI 모델 아키텍처 확정** (Phase 4 완료)
   - EfficientNet-B2 + Multi-head 분류 확정
   - 정규화 파라미터 확정: `mean=(0.972, 0.031, 0.012)`
   - 이 값이 이미 3채널 C++ 출력 기준으로 계산됨 (증거: `config.py` L23-24)

3. ❌ **추가 데이터 수집** (아직)
   - 현재: 실제 샘플 20개 (학습에 부족)
   - 필요: 추가 아동화 샘플 수집 필요

### 현재 단계에서 재학습하지 않는 이유

| 이유 | 설명 |
|------|------|
| **데이터 부족** | 20개 샘플로는 어떤 방식이든 일반화 성능이 낮음 |
| **파이프라인 변경 위험** | 학습 후 C++ 필터 파라미터 변경 시 전체 재학습 필요 |
| **MVP 우선** | 포트폴리오 목적상 파이프라인 구조 증명이 학습 정확도보다 중요 |
| **비용 대비 효과** | 20개 데이터 재전처리 비용 < 향후 대량 데이터 수집 후 재학습 비용 |

---

## 구체적 실행 계획 (Phase 5+)

### Step 1: C++ 전처리로 학습 데이터 재생성

**1.1 실제 아동화 데이터 재전처리**

```bash
# preprocess-server 실행
./out/build/x64-Debug/bin/preprocess_server &

# 각 이미지를 C++ API로 전처리
for img in data/raw/sample_*.png; do
  curl -X POST http://localhost:8081/preprocess \
    -H "Content-Type: application/json" \
    -d "{\"imagePath\": \"$(realpath $img)\"}"
done
# 출력: shared_volume/processed/ 폴더에 처리된 이미지 저장
```

**1.2 합성 데이터 500장도 동일하게 처리**

```bash
# data/synthetic_sketches/images/ 폴더의 모든 이미지를
# 동일하게 C++ 파이프라인으로 처리하여
# data/synthetic_preprocessed/ 폴더에 저장
```

**결과**:
- `data/preprocessed/sample_001.png` (20장)
- `data/synthetic_preprocessed/` (500장)

각 이미지는 C++ 전처리 결과 (3채널: R=Gray, G=InvBinary, B=Dist)

### Step 2: annotations.json 업데이트

```json
// 현재
{
  "samples": [
    {
      "id": "sample_001",
      "image_path": "raw/sample_001.png",  // ← 변경 필요
      "child_age": "7-0",
      ...
    }
  ]
}

// 변경 후
{
  "samples": [
    {
      "id": "sample_001",
      "image_path": "preprocessed/sample_001.png",  // ← C++ 처리 결과
      "child_age": "7-0",
      ...
    }
  ]
}
```

동일하게 합성 데이터 annotations도 경로 변경.

### Step 3: 학습 설정 유지 (변경 최소화)

```python
# train.py 실행 명령 — 변경 없음
python scripts/train.py --gender male
python scripts/train.py --gender female

# 변경사항:
# - HFDDataset이 자동으로 변경된 이미지 경로 로드
# - augmentation의 Resize/Normalize는 유지
#   (이미 C++에서 처리된 데이터를 받으므로)
```

### Step 4: 모델 재생성 및 변환

```bash
# 재학습된 가중치 저장
python scripts/train.py --gender male   # → mind_palette_male.pt
python scripts/train.py --gender female # → mind_palette_female.pt

# ONNX 변환
python scripts/export_model.py

# TensorRT 엔진 생성
python scripts/build_tensorrt_engine.py
```

---

## 정규화 파라미터 검증

### 현재 설정이 3채널 C++ 출력 기준임을 입증

`src/config.py` L23-24:
```python
normalize_mean: Tuple[float, float, float] = (0.972, 0.031, 0.012)
normalize_std: Tuple[float, float, float] = (0.156, 0.174, 0.074)
```

이 값의 의미:
- **mean=(0.972, 0.031, 0.012)**
  - R채널(Gray)의 평균이 매우 높음 (0.972)
  - G, B 채널의 평균이 거의 0에 가까움 (0.031, 0.012)
  - 이는 3채널 합성 이미지에서 R(그레이)이 지배적이고, G(이진화)와 B(거리변환)는 희박함을 의미

- **std=(0.156, 0.174, 0.074)**
  - R채널의 표준편차가 낮음 (0.156) — 그레이 이미지는 동적범위가 작음
  - G채널의 표준편차가 중간 (0.174) — 이진화는 0/255의 이분화
  - B채널의 표준편차가 낮음 (0.074) — 거리변환은 제한된 범위

**결론**: 이미 Phase 4에서 계산된 정규화 파라미터는 C++ 3채널 출력을 기준으로 설계됨.

---

## 위험 및 완화 전략

### 위험 1: 데이터 부족으로 인한 낮은 정확도

**현상**: 20개 샘플 재학습도 정확도 향상이 미미할 수 있음

**완화**:
- Phase 5 기한: 추가 아동화 샘플 최소 50장 이상 수집 후 재학습 고려
- 현재 모델 유지: Phase 4 모델도 충분히 동작하므로 즉시 배포 가능

### 위험 2: C++ 파이프라인 변경 시 재학습 필요

**현상**: `GetContentROI` 파라미터 변경 → 학습 데이터 재전처리 → 재학습 필요

**완화**:
- C++ 파라미터는 Phase 5+ 동안 확정 단계(Locked)로 유지
- 긴급한 버그 수정만 수행

### 위험 3: 추론 성능 회귀

**현상**: 새 모델이 기존 모델보다 성능이 나쁠 수 있음

**완화**:
- 재학습 후 A/B 테스트 필수
- 성능 회귀 시 기존 모델 롤백 가능

---

## Acceptance Criteria

Phase 5+에서 실제 재학습을 시작할 때 다음을 확인:

- [ ] C++ 전처리 파이프라인이 안정적으로 3채널 이미지 생성
- [ ] `data/preprocessed/` 및 `data/synthetic_preprocessed/` 폴더에 전체 데이터 저장
- [ ] annotations.json 경로 업데이트 완료
- [ ] 재학습 후 모델 성능(Loss, F1 score)이 Phase 4 이상
- [ ] 모델 변환(ONNX, TensorRT) 성공 및 추론 결과 동등성 검증

---

## 참고

### Phase 4에서 이미 준비된 것

- ✅ 정규화 파라미터 (3채널 기준)
- ✅ `augmentation.py`의 `ChannelDropout` (R/G/B 채널 독립성 강화)
- ✅ `config.py`의 중앙화된 설정
- ✅ ONNX/TensorRT 변환 파이프라인

### Phase 5에서 추가 작업

- 학습 데이터 C++ 전처리
- annotations.json 경로 업데이트
- 모델 재학습
- 모델 변환 재실행

---

## 관련 문서

- `plan.md` — Phase 5 계획
- `ADR-parameter-rationale.md` — 필터 파라미터 선정 근거
- `ARCHITECTURE_DECISIONS.md` — 전체 의사결정 기록
