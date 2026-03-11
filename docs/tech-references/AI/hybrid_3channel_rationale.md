# 🧠 HFD AI: Hybrid 3-Channel Input Strategy Rationale

## 1. Problem Statement
스케치 형태의 아동 인물화(HFD)는 일반적인 ImageNet 사진 데이터셋과 본질적으로 다릅니다.
- **정보의 휘발성**: 95% 이상이 여백(흰색)이며, 정보는 얇은 선(검정색)에 집중되어 있습니다.
- **다층적 정보**: 선의 굵기(필압), 선의 연결성(형태), 선의 골격(구조)이라는 세 가지 차원의 정보가 공존합니다.
- **전이 학습의 한계**: Pretrained EfficientNet-B2는 3채널 RGB 입력을 기대하지만, 1채널 이진화 이미지만 사용할 경우 정보 손실이 발생합니다.

## 2. Proposed Solution: Hybrid 3-Channel Strategy
단순한 RGB 복제가 아닌, 채널별로 서로 다른 물리적 의미를 부여하여 모델의 특징 추출 능력을 극대화합니다.

| Channel | Data Type | Engineering Intent (왜 필요한가?) |
|:---:|:---:|:---|
| **R** | **Grayscale** | **필압 및 질감(Texture)** 보존. 가우시안 블러 처리된 그레이스케일은 선의 농담을 유지하여 아동의 심리적 상태(필압의 강약)를 반영합니다. |
| **G** | **Binary (Inverted)** | **형태 및 윤곽(Shape)** 강조. 배경을 제거한 순수 이진화 데이터는 모델이 객체의 외곽선과 토폴로지를 명확히 이해하게 돕습니다. |
| **B** | **Distance Transform** | **골격 및 구조(Skeleton)** 강화. 선으로부터의 거리를 부호화하여 선의 중심부일수록 밝게 표현합니다. 이는 선이 끊어져 있어도 구조적 연결성을 모델이 추론하게 돕습니다. |

## 3. Expected Benefits
1.  **Semantic Enrichment**: 동일한 픽셀 위치에서 명암, 형태, 구조 세 가지 정보를 동시에 관찰함으로써 모델의 Attention 성능을 향상시킵니다.
2.  **Domain Adaptation**: White Background로 통일된 3채널 구조는 ImageNet Pretrained Weights와의 Feature Gap을 줄여 전이 학습 수렴 속도를 높입니다.
3.  **Robustness**: 선이 얇거나 일부 소실된 경우에도 B채널(Distance Transform)의 구조적 힌트를 통해 모델이 강건하게 추론할 수 있습니다.

## 4. Implementation Details
- **Preprocessing (C++)**: `cv::cvtColor`, `cv::adaptiveThreshold`, `cv::distanceTransform`을 사용하여 고속 병렬 처리.
- **Normalization (Python)**: ImageNet 기본값(`0.485, ...`)이 아닌, 실제 스케치 데이터셋에서 추출한 `mean`, `std`를 사용하여 도메인 최적화 수행.

## 5. Experimental Data: Domain Gap Analysis
통계 분석 결과, 일반 이미지와 HFD 스케치 데이터 사이에는 상당한 통계적 차이가 존재함이 확인되었습니다. (Synthetic Sketch Dataset N=20 기준)

| Metric | ImageNet (Standard) | HFD Sketch (Target) | Gap Analysis |
|:---|:---:|:---:|:---|
| **Mean (Channel 0)** | 0.485 | **0.972** | +100.4% (배경이 압도적으로 밝음) |
| **Mean (Channel 1)** | 0.456 | **0.031** | -93.2% (선 정보가 매우 희소함) |
| **Std (Channel 0)** | 0.229 | **0.156** | -31.8% (픽셀 편차가 낮음) |

**Conclusion**: ImageNet 기본 정규화 파라미터를 그대로 사용할 경우, 모델은 입력 데이터를 비정상적인 노이즈로 오인할 가능성이 큽니다. 실제 데이터 통계(`0.972`, `0.031` 등)를 적용하여 모델이 **'흰 배경 위의 희소한 선'**이라는 도메인 특성을 최적으로 인식하도록 최적화했습니다.
