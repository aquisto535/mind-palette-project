# 파이프라인 단계별 시각화

각 모듈에서 이미지/데이터가 단계마다 어떻게 변하는지, 실제 이미지로 확인할 수 있는 폴더입니다.  
**왜 이 단계가 이 방식으로 구현되었는지**를 시각적으로 납득하는 것이 목적입니다.

---

## 문서 목록

| 문서 | 내용 |
|------|------|
| [preprocess-server.md](preprocess-server.md) | C++ 전처리 파이프라인 (Stage 00~04, 12개 이미지) |
| [ai-server.md](ai-server.md) | Python AI 서버 전처리 (Stage 01~03, 3개 이미지) |

---

## 이미지 생성 방법

이미지는 repo에 포함되어 있지 않습니다. 아래 도구를 실행하면 직접 생성할 수 있습니다.

### 1. C++ 전처리 단계 이미지 생성

빌드 후 `visualize_pipeline.exe`를 실행합니다:

```bash
# CMake 빌드 (preprocess-server 빌드가 이미 되어 있는 경우 생략)
cd preprocess-server
cmake --build build --target visualize_pipeline

# 실행 (프로젝트 루트에서)
./preprocess-server/build/bin/visualize_pipeline.exe \
  <샘플_인물화.jpg> \
  docs/pipeline-stages/images/preprocess
```

출력 파일 (12개):

```
images/preprocess/
├── 00-original.png          원본 입력
├── 01-resize-768.png        Stage 1: ResizeFilter
├── 02-denoised.png          Stage 2: DenoiseFilter
├── 03a-grayscale.png        Hybrid 내부: 그레이스케일
├── 03b-binarized.png        Hybrid 내부: 적응형 이진화
├── 03c-morphology.png       Hybrid 내부: MORPH_CLOSE
├── 03d-roi-detected.png     Hybrid 내부: ROI 감지 (빨간 박스)
├── 03e-letterbox-512.png    Hybrid 내부: 512×512 Letterbox
├── 04-ch-R-gray.png         최종 R채널 (grayscale)
├── 04-ch-G-binary.png       최종 G채널 (inverted binary)
├── 04-ch-B-distance.png     최종 B채널 (distance map)
└── 04-final-3ch.png         최종 3채널 병합
```

### 2. Python AI 서버 단계 이미지 생성

C++ 전처리 결과(`04-final-3ch.png`)를 입력으로 사용합니다:

```bash
cd <프로젝트 루트>
python ai-server/tools/generate_pipeline_stages.py \
  docs/pipeline-stages/images/preprocess/04-final-3ch.png \
  docs/pipeline-stages/images/ai-server
```

출력 파일 (3개):

```
images/ai-server/
├── 01-input-512x512.png       입력 이미지 + 채널 분리
├── 02-resize-260.png          PIL Resize 260×260
└── 03-channel-normalized.png  ImageNet 정규화 후 분포
```

---

## 전체 파이프라인 흐름

```
원본 인물화 (임의 크기)
        │
        ▼  [C++ 전처리 서버]
   00-original
        │
        ▼  ResizeFilter(768)
   01-resize-768
        │
        ▼  DenoiseFilter(Gaussian 5x5)
   02-denoised
        │
        ▼  HybridPreprocessFilter 내부
   03a grayscale → 03b binarize → 03c morphology
        │
        ▼  ROI 감지 + Letterbox
   03d roi-detected → 03e letterbox-512
        │
        ▼  3채널 병합 (R/G/B)
   04-final-3ch  (512×512×3)
        │
        ▼  [Python AI 서버]
   01-input-512x512
        │
        ▼  PIL Resize
   02-resize-260  (260×260)
        │
        ▼  ImageNet Normalize
   03-channel-normalized
        │
        ▼  EfficientNet-B2 backbone
   1408-dim features
        │
        ▼  4개 Linear Heads
   60개 문항 이진 결과 → IQ 산출
```
