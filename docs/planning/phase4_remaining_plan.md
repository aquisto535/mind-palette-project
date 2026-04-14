# Phase 4 이후 미구현 항목 구현 계획

## Context
plan.md의 "Phase 4 이후 실행 (ROI 순)" 6개 + "Phase 4 연계 항목 (AI 서버와 함께)" 5개, 총 11개 미구현 항목을 TDD Red-Green-Refactor로 구현한다. 의존성과 ROI를 고려해 6개 Work Unit으로 묶어 순차 진행한다.

---

## 실행 순서 요약

| WU | 항목 | 난이도 | 의존성 |
|----|------|--------|--------|
| 1 | B2(정규화 하드코딩 수정) + A2(보간법 최적화) | 소 | 없음 |
| 2 | A4(CLAHE 필터) + A6(fastNlMeansDenoising) | 중 | 없음 |
| 3 | A3(Otsu 자동 Threshold) + A5(PSNR/SSIM 메트릭) | 중 | 없음 |
| 4 | B1(필압 분석) + B4(선 떨림 보정) | 대 | 없음 |
| 5 | B3(Channel Dropout 증강) | 소 | 없음 |
| 6 | B5(하이브리드 결합) + A1(파라미터 문서화) | 중 | B1, B4 완료 필요 |

---

## Work Unit 1: Quick Wins (B2 + A2)

### B2: augmentation.py 정규화 하드코딩 제거

**문제**: `augmentation.py`에 mean/std가 하드코딩됨. `config.py`에서 읽어야 함.

**수정 파일**:
- `ai-server/src/core/augmentation.py` — `get_train_transform()`, `get_val_transform()` 시그니처에 `config: ModelConfig | None = None` 추가
- `ai-server/tests/test_augmentation.py` — config 주입 시 커스텀 mean/std 반영 검증 테스트 추가

**TDD**:
- Red: 커스텀 config 전달 시 Normalize에 해당 값이 반영되는지 테스트
- Green: `config=None`이면 `ModelConfig()` 기본값 사용 (하위호환)

**커밋**: `refactor(ai-server): extract normalization params from config in augmentation`

### A2: ResizeFilter 보간법 최적화

**문제**: `ResizeFilter::apply()`가 `cv::resize()` 호출 시 보간법 미지정 (기본 INTER_LINEAR). 축소 시 INTER_AREA, 확대 시 INTER_CUBIC 사용해야 함.

**수정 파일**:
- `preprocess-server/src/filters/resize_filter.cpp` — scale < 1.0이면 INTER_AREA, 아니면 INTER_CUBIC
- `preprocess-server/tests/test_filters.cpp` — 1000x1500→512 (축소), 100x100→512 (확대) 테스트 추가

**참고**: `HybridPreprocessFilter`는 이미 INTER_AREA/INTER_NEAREST 사용 중 (변경 불필요)

**커밋**:
1. `test(preprocess): add interpolation tests for ResizeFilter`
2. `feat(preprocess): use INTER_AREA/INTER_CUBIC in ResizeFilter`

---

## Work Unit 2: 새 필터 (A4 + A6)

### A4: CLAHE 히스토그램 평활화 필터

**새 파일**:
- `preprocess-server/src/filters/clahe_filter.h/.cpp`

**구현**: `IFilter` 구현, `cv::createCLAHE(clipLimit=2.0, tileGridSize=8x8)` 사용. BGR 입력 시 그레이스케일 변환 후 처리.

**TDD**:
- Red: 저대비 이미지 → CLAHE 후 stddev 증가 검증, BGR 입력 처리, 빈 입력 처리
- Green: `clahe->apply(gray, result)`

**커밋**:
1. `test(preprocess): add CLAHE filter tests`
2. `feat(preprocess): add ClaheFilter for uneven lighting compensation`

### A6: NlMeansDenoiseFilter (엣지 보존 노이즈 제거)

**새 파일**:
- `preprocess-server/src/filters/nlmeans_denoise_filter.h/.cpp`

**설계**: 기존 `DenoiseFilter`는 유지 (하위호환). 새 `NlMeansDenoiseFilter`를 대안으로 추가. `cv::fastNlMeansDenoising(h=10, templateWindow=7, searchWindow=21)` 사용.

**TDD**:
- Red: 엣지+노이즈 이미지 처리, BGR 입력(fastNlMeansDenoisingColored), 빈 입력
- Green: 채널 수에 따라 grayscale/colored 버전 분기

**커밋**:
1. `test(preprocess): add NlMeansDenoiseFilter tests`
2. `feat(preprocess): add NlMeansDenoiseFilter for edge-preserving denoising`

**CMakeLists.txt**: 두 필터 소스를 `FILTER_SOURCES`에 추가

---

## Work Unit 3: 분석 유틸 (A3 + A5)

### A3: Otsu 기반 자동 Canny Threshold

**새 파일**:
- `preprocess-server/src/filters/otsu_canny_filter.h/.cpp`

**구현**: `IFilter` 구현. Otsu로 최적 threshold 산출 → `low = (1-sigma)*otsu`, `high = (1+sigma)*otsu`로 Canny 적용. 기본 sigma=0.5.

**TDD**:
- Red: 흑백 사각형 이미지 → 엣지 검출 성공, 커스텀 sigma 적용, 빈 입력
- Green: `cv::threshold(..., THRESH_OTSU)`로 otsu값 획득 → `cv::Canny(gray, edges, low, high)`

**커밋**:
1. `test(preprocess): add OtsuCannyFilter auto-threshold tests`
2. `feat(preprocess): add OtsuCannyFilter with Otsu-based auto thresholding`

### A5: PSNR/SSIM 품질 메트릭 유틸리티

**새 파일**:
- `preprocess-server/src/utils/quality_metrics.h/.cpp`
- `preprocess-server/tests/test_quality_metrics.cpp`

**구현**: 필터가 아닌 유틸리티 클래스. `QualityMetrics::computePSNR()` (cv::PSNR 래핑), `QualityMetrics::computeSSIM()` (Gaussian 기반 SSIM 수동 구현).

**TDD**:
- Red: 동일 이미지 → PSNR>50/SSIM≈1.0, 다른 이미지 → 유한값/1.0 미만, 빈 입력 → 음수
- Green: PSNR은 `cv::PSNR()`, SSIM은 Gaussian window + 공분산 기반 계산

**커밋**:
1. `test(preprocess): add PSNR/SSIM quality metrics tests`
2. `feat(preprocess): add QualityMetrics utility`

---

## Work Unit 4: C++ 기하 분석 (B1 + B4)

### B1: 필압 분석 (R채널 히스토그램)

**새 파일**:
- `preprocess-server/src/analysis/pressure_analyzer.h/.cpp`
- `preprocess-server/tests/test_pressure_analyzer.cpp`

**구현**: 3채널 이미지의 R채널(BGR에서 index 2) 히스토그램 분석. `pressureScore = 1.0 - (meanIntensity / 255.0)`. 반환: `PressureResult { pressureScore, histogram[256], meanIntensity, stdIntensity }`.

**서버 연동**: `server.h`에 `POST /analyze-pressure` 엔드포인트 추가 → JSON 반환.

**TDD**:
- Red: 어두운 R채널(강한 필압) → score > 0.5, 밝은 R채널(약한 필압) → score < 0.5, 빈 입력
- Green: `cv::split()` → `cv::calcHist()` → `cv::meanStdDev()`

**커밋**:
1. `test(preprocess): add PressureAnalyzer tests`
2. `feat(preprocess): add PressureAnalyzer for R-channel pressure scoring`
3. `feat(preprocess): add /analyze-pressure endpoint`

### B4: 선 떨림 보정 (Contour Moment)

**새 파일**:
- `preprocess-server/src/analysis/tremor_analyzer.h/.cpp`
- `preprocess-server/tests/test_tremor_analyzer.cpp`

**구현**: 이진 이미지에서 윤곽선 추출 → 인접 3점 간 각도 기반 곡률(curvature) 계산 → 곡률 분산이 클수록 떨림. 반환: `TremorResult { tremorScore, contourCount, avgCurvatureVariance, huMoments[7] }`.

**서버 연동**: `server.h`에 `POST /analyze-tremor` 엔드포인트 추가.

**TDD**:
- Red: 직선 → tremorScore < 0.3, 지그재그 → tremorScore > 0.3, 빈 입력
- Green: `cv::findContours()` → 각도 기반 곡률 분산 → Hu moments

**커밋**:
1. `test(preprocess): add TremorAnalyzer tests`
2. `feat(preprocess): add TremorAnalyzer with curvature variance analysis`
3. `feat(preprocess): add /analyze-tremor endpoint`

---

## Work Unit 5: Channel Dropout (B3)

**수정 파일**:
- `ai-server/src/core/augmentation.py` — `ChannelDropout` 클래스 추가
- `ai-server/tests/test_augmentation.py` — 테스트 추가

**구현**: `ChannelDropout(p=0.1)` — 확률적으로 3채널 중 하나를 0으로. `get_train_transform()`의 `ToTensor()` 뒤, `Normalize()` 앞에 삽입.

**TDD**:
- Red: p=1.0 → 정확히 1채널이 0, p=0.0 → 원본 유지, train_transform에 ChannelDropout 포함 확인
- Green: `random.randint(0, 2)`로 채널 선택 → `tensor[ch] = 0.0`

**커밋**:
1. `test(ai-server): add ChannelDropout augmentation tests`
2. `feat(ai-server): add ChannelDropout for training robustness`

---

## Work Unit 6: 통합 + 문서화 (B5 + A1)

### B5: 하이브리드 결과 결합 (C++ 기하 + AI 추론)

**새 파일**:
- `ai-server/src/core/hybrid_combiner.py`
- `ai-server/tests/test_hybrid_combiner.py`

**수정 파일**:
- `ai-server/src/routes/analyze.py` — `/analyze` 응답에 geometric 분석 결과 통합

**구현**: `HybridCombiner.combine(ai_result, geometric)` → `{ ai_analysis, geometric_analysis, combined_confidence }`. tremor_score가 높으면 confidence 감소.

**TDD**:
- Red: 모든 소스 포함 검증, 빈 geometric → AI만 반환, 높은 tremor → 낮은 confidence
- Green: confidence = 1.0 - (tremor_score * 0.3), clamp [0, 1]

**커밋**:
1. `test(ai-server): add HybridCombiner tests`
2. `feat(ai-server): add HybridCombiner for C++ + AI result fusion`
3. `feat(ai-server): integrate geometric analysis into /analyze endpoint`

### A1: 파라미터 근거 문서화

**새 파일**: `docs/standards/ADR-parameter-rationale.md`

**내용**: WU3의 QualityMetrics + 기존 벤치마크 인프라로 각 파라미터 3가지 값 비교 실험 결과 기록.

| 필터 | 파라미터 | 테스트값 | 선택값 |
|------|---------|---------|--------|
| BinarizeFilter | blockSize | 7, 11, 15 | 11 |
| BinarizeFilter | C | 1, 2, 3 | 2 |
| DenoiseFilter | gaussianSize | 3, 5, 7 | 5 |
| ClaheFilter | clipLimit | 1.0, 2.0, 4.0 | 2.0 |
| NlMeansDenoiseFilter | h | 5, 10, 15 | 10 |
| OtsuCannyFilter | sigma | 0.33, 0.5, 0.66 | 0.5 |

**커밋**: `docs(preprocess): add ADR for filter parameter rationale with benchmarks`

---

## 검증 방법

### C++ (각 WU 완료 후)
1. Visual Studio 테스트 탐색기에서 `unit_tests` 전체 실행
2. 새 테스트 모두 Green 확인
3. 기존 테스트 회귀 없음 확인

### Python (각 WU 완료 후)
```bash
cd ai-server && python -m pytest tests/ -v
```
- 기존 108개 + 새 테스트 모두 통과 확인

### E2E (WU6 완료 후)
1. C++ 서버 기동 → `/analyze-pressure`, `/analyze-tremor` 엔드포인트 응답 확인
2. Python 서버 기동 → `/analyze` 응답에 `geometric_analysis` 필드 포함 확인
3. API Gateway → C++ → Python 전체 파이프라인 통합 테스트
