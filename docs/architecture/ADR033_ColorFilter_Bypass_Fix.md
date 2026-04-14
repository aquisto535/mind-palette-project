# ADR-033 컬러 이미지 필터링 우회 근본 수정

**발생 환경**: EC2 c5.large 실배포  
**발생 시기**: 2026-04-13  
**영향 범위**: preprocess-server, api-gateway, ai-server, frontend

---

## 증상

색상이 가득한 아동화(크레파스, 수채화 등)를 업로드해도 거부되지 않고 지능 측정 결과까지 반환됨. ADR-033의 "Two-Tier 검증 및 조기 Reject" 전략이 프로덕션에서 작동하지 않음.

---

## 근본 원인: 3개 지점의 Fail-Open 버그

### Bypass #1 (주 원인): API Gateway Fail-Open 폴백

**파일**: `api-gateway/src/services/analysisService.ts`

preprocess-server에서 422(COLOR_VALIDATION_FAILED) 이외의 에러(네트워크 타임아웃, Crow 빈 응답, 500 응답 등)가 발생하면, catch 블록이 `logger.warn`만 찍고 조용히 통과시켜 **원본 컬러 이미지가 그대로 AI 서버로 전달**됨.

```typescript
// 수정 전 (Fail-Open):
} catch (error: unknown) {
  if (axios.isAxiosError(error) && error.response?.status === 422) throw error;
  // 그 외 모든 에러 → warn만 찍고 원본 이미지 사용
  logger.warn('L6 Sanitization skipped: preprocessing failed, using original image');
}
return { processedImagePath, sanitized, serverTiming }; // 원본 이미지가 흘러나감
```

### Bypass #2: C++ 파이프라인 빈 입력 조용한 실패

**파일**: `preprocess-server/src/core/image_processor.cpp`

`cv::imread` 실패 시(경로 불일치, 권한, 손상 파일) `input.empty()`가 true가 되고, **ColorValidationFilter를 포함한 파이프라인 전체가 실행되지 않음**. 빈 Mat를 반환 → preprocess-server가 500 에러 → Bypass #1과 결합해 원본 컬러 이미지가 AI 서버로 직행.

```cpp
// 수정 전 (Fail-Open):
cv::Mat ImageProcessor::Preprocess(const cv::Mat& input) {
    if (input.empty()) return cv::Mat();  // 조용히 통과 → 파이프라인 미실행
    auto pipeline = PipelineFactory::createHybridPipeline();
    return pipeline.execute(input);
}
```

### Bypass #3: AI Server Tier 2가 색상을 검사하지 않음

**파일**: `ai-server/src/routes/analyze.py`

AI 서버의 Tier 2는 `mean_confidence < 0.3`(의미론적 신뢰도)만 검사. **색상 정보는 전혀 확인하지 않음**. 선명한 크레파스 그림은 컨투어가 명확해 오히려 신뢰도가 높게 나와 Tier 2조차 통과.

---

## 수정 내용: Fail-Closed + Defense-in-Depth

### Fix 1: API Gateway Fail-Closed 전환

**파일**: `api-gateway/src/services/analysisService.ts`

```typescript
// 수정 후 (Fail-Closed):
} catch (error: unknown) {
  if (axios.isAxiosError(error) && error.response?.status === 422) throw error;
  if (axios.isAxiosError(error) &&
      (error.response?.status === 400 || error.response?.status === 500) &&
      error.response?.data?.error === 'COLOR_VALIDATION_FAILED') {
    error.response.status = 422;
    throw error;
  }
  // 그 외 모든 에러 → Fail-Closed: 503으로 사용자 통보
  logger.error('Preprocessing failed — request rejected (Fail-Closed policy):', {
    error: error instanceof Error ? error.message : String(error),
    requestId
  });
  const serviceError = new Error('PREPROCESS_SERVICE_UNAVAILABLE');
  serviceError.name = 'PreprocessServiceError';
  throw serviceError;
}
```

**파일**: `api-gateway/src/routes/analyze.ts`

```typescript
// 503 핸들러 추가:
if (error instanceof Error && error.name === 'PreprocessServiceError') {
  return res.status(503).json({
    error: 'PREPROCESS_SERVICE_UNAVAILABLE',
    message: '전처리 서버에 일시적인 문제가 발생했습니다. 잠시 후 다시 시도해주세요.'
  });
}
```

**효과**: preprocess 성공 응답(processedPath 반환)이 아닌 모든 경우 요청 거부. 원본 이미지가 AI 서버로 흘러가는 경로 차단.

---

### Fix 2: C++ 파이프라인 명시적 실패 (Fail-Closed)

**파일**: `preprocess-server/src/core/image_processor.cpp`

```cpp
// 수정 후 (Fail-Closed: throw):
cv::Mat ImageProcessor::Preprocess(const cv::Mat& input, const std::string& requestId) {
    if (input.empty()) throw std::runtime_error("PIPELINE_FAILED: empty input image");
    auto pipeline = PipelineFactory::createHybridPipeline();
    return pipeline.execute(input);
}
```

**파일**: `preprocess-server/src/core/server.h`

```cpp
// ProcessImageFile() — 파일 로드 실패 시 throw:
if (img.empty()) {
    LOG_ERROR(requestId, "Failed to load image: {}", imagePath);
    throw std::runtime_error("IMAGE_LOAD_FAILED: " + imagePath);
}

// 새 catch 블록 추가:
} catch (const std::runtime_error& e) {
    LOG_ERROR(requestId, "Image processing error: {}", e.what());
    crow::json::wvalue body;
    body["error"] = "IMAGE_LOAD_FAILED";
    body["message"] = std::string(e.what());
    crow::response res(400, body.dump());
    res.add_header("Content-Type", "application/json");
    return res;
}
```

**효과**: 파이프라인이 실행되지 않는 조용한 실패 경로 제거. 어떤 상황에서도 `ColorValidationFilter` 미실행 시 HTTP 에러 반환.

---

### Fix 3: Python AI Server Tier 2 색상 검사 추가

**파일**: `ai-server/src/routes/analyze.py`

```python
_COLOR_SAT_THRESHOLD = 30   # HSV S채널 임계값 (0-255)
_COLOR_PIXEL_RATIO   = 0.05 # 색상 픽셀 비율 임계값 (5%)

def _validate_grayscale_input(content: bytes) -> None:
    """Tier 2 색상 방어선 — Tier 1이 우회되어도 독립적으로 차단"""
    pil_image = Image.open(io.BytesIO(content)).convert("HSV")
    arr = np.array(pil_image, dtype=np.float32)
    saturation = arr[:, :, 1]
    color_pixels = np.sum(saturation > _COLOR_SAT_THRESHOLD)
    total_pixels = saturation.size
    ratio = color_pixels / total_pixels if total_pixels > 0 else 0.0
    if ratio >= _COLOR_PIXEL_RATIO:
        raise HTTPException(
            status_code=422,
            detail=f"컬러 이미지가 감지되었습니다. 연필로 그린 흑백 그림만 처리할 수 있습니다. "
                   f"(color_ratio={ratio:.1%}, threshold={_COLOR_PIXEL_RATIO:.1%})"
        )

# 엔드포인트에서 _validate_image_file 직후 호출:
_validate_grayscale_input(content)
```

**효과**: Gateway/preprocess-server 우회 시에도 AI 서버가 최종 수문장 역할. C++ Tier 1과 동일한 HSV 채도 기반 알고리즘 적용.

---

### Fix 4: C++ 색상 비율 로그 레벨 승격

**파일**: `preprocess-server/src/filters/color_validation_filter.cpp`

```cpp
// 수정 전:
spdlog::debug("[ColorValidationFilter] color ratio = ...");

// 수정 후:
spdlog::info("[ColorValidationFilter] color ratio = {:.1f}% (threshold = {:.1f}%)",
             ratio * 100.0, colorPixelRatio_ * 100.0);
```

**효과**: EC2에서 `docker logs preprocess-server | grep "color ratio"`로 실제 이미지의 색상 비율 추적 가능. 임계값 튜닝 데이터 수집.

---

## 부수적 발견 및 수정: Frontend Mock 데이터 문제

### 증상

실배포 환경에서 분석 결과가 70~95 사이의 무작위 점수로 반환됨(실제 AI 추론이 아닌 Mock 데이터).

### 원인

`frontend/.dockerignore`가 `.env.local`을 빌드 컨텍스트에서 제외 → Docker 빌드 시 `VITE_USE_MOCK` 미정의 → `undefined !== 'false'`가 `true` → Mock 모드 항상 활성화.

```
# frontend/.dockerignore (라인 12):
.env.local  ← VITE_USE_MOCK=false 설정 파일이 빌드에서 제외됨
```

### 수정

**파일**: `frontend/src/api/uploadApi.ts`

```typescript
// 수정 전 (Mock이 기본값):
const useMock = import.meta.env.VITE_USE_MOCK !== 'false';

// 수정 후 (실제 API가 기본값):
const useMock = import.meta.env.VITE_USE_MOCK === 'true';
```

**교훈**: Vite 환경 변수는 빌드 타임에 embed됨. `.dockerignore`가 `.env.local`을 제외하면 해당 변수는 `undefined`가 됨. **Default는 반드시 안전한 방향(실제 API 사용)으로 설정해야 함**. 로컬 개발 시 Mock이 필요하면 `.env.local`에 `VITE_USE_MOCK=true` 명시.

---

## CTest 실패 수정: Preprocess_EmptyInputThrows

### 증상

GitHub CI에서 CTest 98개 중 1개 실패:

```
[  FAILED  ] ImageProcessorTest.Preprocess_EmptyInputReturnsEmpty
C++ exception with description "PIPELINE_FAILED: empty input image" thrown in the test body
```

### 원인

Fix 2가 `Preprocess()` 빈 입력 동작을 변경: 빈 Mat 반환 → `std::runtime_error` throw.  
기존 테스트는 `EXPECT_TRUE(result.empty())`를 기대하고 있었으나 throw가 발생해 테스트 자체가 예외 종료.

### 수정

**파일**: `preprocess-server/tests/test_main.cpp`

```cpp
// 수정 전:
TEST_F(ImageProcessorTest, Preprocess_EmptyInputReturnsEmpty) {
    cv::Mat empty;
    cv::Mat result = processor.Preprocess(empty);
    EXPECT_TRUE(result.empty());
}

// 수정 후 (ADR-033 Fail-Closed 행동 검증):
TEST_F(ImageProcessorTest, Preprocess_EmptyInputThrows) {
    // ADR-033 Fail-Closed: 빈 입력은 조용히 통과시키지 않고 runtime_error를 throw해야 함
    cv::Mat empty;
    EXPECT_THROW(processor.Preprocess(empty), std::runtime_error);
}
```

`#include <stdexcept>` 헤더도 추가.

---

## 검증 방법 (EC2 재배포 후)

```bash
# 1. 색상 이미지 거부 확인 (422 기대)
curl -F "file=@tests/fixtures/crayon_drawing.jpg" http://localhost/api/analyze
# → {"error": "COLOR_VALIDATION_FAILED", ...}

# 2. 연필 이미지 정상 통과 확인 (200 기대)
curl -F "file=@tests/fixtures/pencil_hfd.jpg" http://localhost/api/analyze
# → {"score": ..., "percentile": ..., ...}

# 3. preprocess-server 다운 시 Fail-Closed 확인 (503 기대)
docker compose stop preprocess-server
curl -F "file=@tests/fixtures/any.jpg" http://localhost/api/analyze
# → {"error": "PREPROCESS_SERVICE_UNAVAILABLE", ...}

# 4. 색상 비율 로그 확인
docker logs preprocess-server | grep "color ratio"
```

---

## 임계값 튜닝 가이드

현재 임계값 (`satThreshold=30`, `colorPixelRatio=5%`)이 너무 관대하다면:

| 파라미터 | 현재값 | 강화값 | 설명 |
|----------|--------|--------|------|
| `satThreshold` (C++) | 30 | 20 | HSV S채널 채도 기준 (낮을수록 더 엄격) |
| `colorPixelRatio` (C++) | 0.05 (5%) | 0.03 (3%) | 색상 픽셀 최소 비율 |
| `_COLOR_SAT_THRESHOLD` (Python) | 30 | 20 | AI 서버 동일 기준 |
| `_COLOR_PIXEL_RATIO` (Python) | 0.05 | 0.03 | AI 서버 동일 기준 |

**주의**: False Positive(연필 스케치가 컬러로 오인) 위험이 있으므로 **실제 로그 데이터 관찰 후** 조정할 것.
