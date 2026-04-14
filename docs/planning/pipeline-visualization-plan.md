# 파이프라인 단계별 시각화 폴더 구축 계획

## Context

이미지 처리 파이프라인의 각 단계가 왜 그 방식으로 설계되었는지를 직관적으로 이해하기 위해, 각 단계의 실제 입출력 이미지를 모아 놓는 폴더가 필요하다. 특히 `HybridPreprocessFilter`의 3채널 분리(R=grayscale, G=inverted binary, B=distance map) 같은 비직관적 설계 결정은, 중간 단계 이미지를 보지 않고서는 왜 그렇게 해야 했는지 이해하기 어렵다.

**대상 모듈**: C++ 전처리 서버 + Python AI 서버 (이미지 변환이 실질적으로 일어나는 모듈)

---

## 최종 폴더 구조

```
docs/pipeline-stages/                    ← 신규 생성
├── README.md                            ← 전체 인덱스, 이미지 생성 방법
├── preprocess-server.md                 ← C++ 파이프라인 전 단계 문서
├── ai-server.md                         ← Python AI 파이프라인 문서
└── images/
    ├── preprocess/                      ← C++ 각 단계별 출력 이미지
    │   ├── 00-original.png              ← 원본 입력
    │   ├── 01-resize-768.png            ← Stage 1: ResizeFilter 출력
    │   ├── 02-denoised.png              ← Stage 3: DenoiseFilter 출력
    │   ├── 03a-grayscale.png            ← Hybrid 내부: 그레이스케일 변환
    │   ├── 03b-binarized.png            ← Hybrid 내부: 적응형 이진화
    │   ├── 03c-morphology.png           ← Hybrid 내부: MORPH_CLOSE 적용
    │   ├── 03d-roi-detected.png         ← Hybrid 내부: ROI 감지 (bounding box 표시)
    │   ├── 03e-letterbox-512.png        ← Hybrid 내부: Letterbox 리사이즈
    │   ├── 04-ch-R-gray.png             ← 최종 R채널 (grayscale)
    │   ├── 04-ch-G-binary.png           ← 최종 G채널 (inverted binary)
    │   ├── 04-ch-B-distance.png         ← 최종 B채널 (distance map)
    │   └── 04-final-3ch.png             ← 최종 3채널 병합 결과
    └── ai-server/
        ├── 01-input-512x512.png         ← C++ 출력 그대로 (3채널)
        ├── 02-resize-260.png            ← PIL Resize 260x260
        └── 03-channel-normalized.png    ← 정규화된 텐서의 채널별 시각화
```

---

## 구현 단계

### Step 1: C++ 단계 생성 도구 작성

**대상 파일**: `preprocess-server/src/utils/visualize_pipeline.cpp`

현재 이 파일은 최종 결과만 저장함. 이를 확장해 **중간 단계를 모두 저장**하도록 수정.

구현 전략: `HybridPreprocessFilter`의 내부를 수정하지 않고, **개별 필터 클래스들을 직접 순서대로 호출**해 단계별 결과를 저장. 이렇게 하면 프로덕션 코드 변경 없이 파이프라인의 각 단계를 독립적으로 시각화할 수 있음.

저장 단계 목록:
1. `00-original.png` — 원본 입력
2. `01-resize-768.png` — `ResizeFilter(768, false).Process()`
3. `02-denoised.png` — `DenoiseFilter(5, 0).Process()`
4. `03a-grayscale.png` — `cv::cvtColor(BGR→GRAY)`
5. `03b-binarized.png` — `BinarizeFilter.Process()` (AdaptiveThreshold)
6. `03c-morphology.png` — `MorphologyFilter.Process()` (MORPH_CLOSE 3x3)
7. `03d-roi-detected.png` — ROI bounding box를 원본 위에 그려서 저장
8. `03e-letterbox-512.png` — ROI 영역을 512x512 letterbox로 리사이즈
9. `04-ch-R/G/B.png`, `04-final-3ch.png` — 채널 분리 및 병합 결과

**CMakeLists.txt 확인 필요**: `visualize_pipeline.cpp`가 이미 `add_executable` 타겟으로 등록되어 있는지 확인. 등록되어 있지 않으면 추가.

관련 헤더:
- `preprocess-server/src/filters/resize_filter.h`
- `preprocess-server/src/filters/denoise_filter.h`
- `preprocess-server/src/filters/binarize_filter.h`
- `preprocess-server/src/filters/morphology_filter.h`
- `preprocess-server/src/filters/hybrid_preprocess_filter.h`

### Step 2: Python AI 단계 생성 도구 작성

**신규 파일**: `ai-server/tools/generate_pipeline_stages.py`

C++ 출력(512x512x3 PNG)을 입력받아 AI 서버 내부 전처리 단계를 시각화:

1. `01-input-512x512.png` — 입력 그대로 (3채널 분리해서 R/G/B 나란히 표시)
2. `02-resize-260.png` — PIL 260x260 리사이즈
3. `03-channel-normalized.png` — ImageNet 정규화 후 각 채널 값 분포 시각화 (min-max 스케일링으로 가시화)

의존성: PIL, numpy, matplotlib (별도 pip install 불필요 — ai-server 환경에 이미 있는 라이브러리 사용)

### Step 3: 샘플 이미지로 실행 및 이미지 생성

1. C++ 도구 빌드: CMake 리빌드 후 `visualize_pipeline.exe` 실행
   ```
   ./visualize_pipeline.exe <샘플_인물화.jpg> docs/pipeline-stages/images/preprocess/
   ```
2. Python 도구 실행:
   ```
   python ai-server/tools/generate_pipeline_stages.py \
     docs/pipeline-stages/images/preprocess/04-final-3ch.png \
     docs/pipeline-stages/images/ai-server/
   ```

**샘플 이미지**: 임의의 연필 인물화 이미지를 제공하거나, `preprocess-server/tests/` 내 기존 테스트 이미지 활용.

### Step 4: 마크다운 문서 작성

**`docs/pipeline-stages/preprocess-server.md`** 구조:

```markdown
# C++ 전처리 파이프라인 단계별 시각화

## 파이프라인 개요
[ASCII 플로우 다이어그램]

## Stage 1: ResizeFilter (768×768)
| 입력 | 출력 |
|------|------|
| ![original](images/preprocess/00-original.png) | ![resize](images/preprocess/01-resize-768.png) |

**Why**: 이후 모든 연산을 768×768 고정 해상도에서 수행해 레이턴시 47% 감소 (183ms→97ms)
**파라미터 근거**: `docs/architecture/ADR-parameter-rationale.md` 참조
**코드**: `preprocess-server/src/filters/resize_filter.cpp`

## Stage 3: DenoiseFilter
...

## Stage 4: HybridPreprocessFilter (내부 단계)

### 4a. 그레이스케일 변환
### 4b. 적응형 이진화
### 4c. 형태학 연산 (MORPH_CLOSE)
### 4d. Smart Crop (ROI 감지)
### 4e. Letterbox 리사이즈
### 4f. 3채널 병합 결과
```

**`docs/pipeline-stages/ai-server.md`** 구조: 동일 패턴으로 AI 추론 전처리 단계 설명.

**`docs/pipeline-stages/README.md`**: 전체 인덱스, 이미지 재생성 방법, 각 문서 링크.

---

## 수정/생성 대상 파일 목록

| 파일 | 작업 |
|------|------|
| `preprocess-server/src/utils/visualize_pipeline.cpp` | 중간 단계 저장 기능으로 확장 |
| `preprocess-server/CMakeLists.txt` | visualize_pipeline 빌드 타겟 확인/추가 |
| `ai-server/tools/generate_pipeline_stages.py` | 신규 생성 |
| `docs/pipeline-stages/README.md` | 신규 생성 |
| `docs/pipeline-stages/preprocess-server.md` | 신규 생성 |
| `docs/pipeline-stages/ai-server.md` | 신규 생성 |
| `docs/pipeline-stages/images/preprocess/*.png` | 생성 도구 실행으로 생성 |
| `docs/pipeline-stages/images/ai-server/*.png` | 생성 도구 실행으로 생성 |

---

## 검증 방법

1. **이미지 파일 확인**: `docs/pipeline-stages/images/preprocess/` 폴더에 12개 PNG 파일 존재
2. **마크다운 렌더링**: VS Code에서 `preprocess-server.md` 미리보기 열어 이미지가 올바르게 표시되는지 확인
3. **단계별 시각적 검증**:
   - 03b(이진화)에서 선이 흰색, 배경이 검은색인지
   - 03c(MORPH_CLOSE)에서 끊어진 선이 연결되는지
   - 03d(ROI)에서 인물 주변에 bounding box가 정확히 그려지는지
   - 04 채널들이 각각 의미 있게 다른 정보를 담고 있는지
4. **재생성 테스트**: 새 이미지로 도구를 다시 실행해 동일한 단계 파일들이 생성되는지 확인
