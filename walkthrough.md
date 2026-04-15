# 2026-04-15 업로드 예외처리 안정화 메모

## 이번 수정의 목적

- `남자사람_8_남_06463.jpg` 같은 정상 연필화를 `422`로 잘못 막는 오탐을 줄인다.
- `twi001t2960745.jpg` 같은 실제 컬러/비정상 입력은 계속 `422`로 차단한다.
- preprocess 장애가 생겼을 때 원본 이미지가 우회 통과하거나, 상태 코드가 뒤섞이는 문제를 줄인다.

## 핵심 변경

### 1. ColorValidationFilter 판정 기준 개선

파일: `preprocess-server/src/filters/color_validation_filter.*`

- 기존: `어두운 픽셀 또는 고채도 픽셀`을 넓게 모수로 잡고 `HSV saturation` 중심으로 판정
- 변경: `실제 스트로크에 가까운 어두운 픽셀(V <= threshold)`만 모수로 잡고,
  `HSV saturation` + `RGB 채널 편차(chroma)`를 동시에 만족할 때만 컬러 스트로크로 카운트

이유:
- 종이 황변, 촬영 조명, JPEG 색번짐은 saturation이 약간 생겨도 채널 편차는 작다.
- 반대로 크레파스/색연필 스트로크는 saturation과 채널 편차가 둘 다 크다.

### 2. 회귀 테스트 추가

파일: `preprocess-server/tests/test_filters.cpp`

- `PassesWarmTintedPencilStroke`
- `ThrowsOnHighChromaStroke`

의도:
- 스캔/촬영된 연필화의 미세한 색 틴트는 통과
- 실제 컬러 채색은 거부

### 3. Gateway 정책 테스트 정렬

파일: `api-gateway/tests/analysisService.test.ts`

- preprocess `500`은 더 이상 조용히 우회하지 않고 `PreprocessServiceError`로 거부
- preprocess `422`는 그대로 상위로 전파

의도:
- `422 / 503 / 500` 경계가 테스트에서도 운영 정책과 일치하도록 고정

## 배포 후 확인 포인트

1. `남자사람_8_남_06463.jpg` 업로드 시 preprocess 로그의 `color ratio`가 5% 미만인지 확인
2. `twi001t2960745.jpg` 업로드 시 `COLOR_VALIDATION_FAILED`와 함께 `422`가 유지되는지 확인
3. preprocess 비정상 시 gateway가 원본 이미지로 우회하지 않고 `503`을 반환하는지 확인

## 남은 권장 작업

- 배포 서버에서 `남자사람_8_남_06463.jpg`와 실제 컬러 샘플 5~10장을 고정 회귀셋으로 등록
- `color ratio`, `stroke pixel count`, `chroma` 통계를 로그나 메트릭으로 수집해 임계값을 데이터로 조정
- 가능하면 preprocess 응답 body에 진단 필드(`colorRatio`, `strokePixels`)를 넣어 운영 디버깅 시간을 줄일 것

---

# 2026-04-15 ColorValidationFilter 밝은 컬러 회귀 수정 메모

## 증상

- `ColorValidationFilterTest.ThrowsOnColorImage`
- `ColorValidationFilterTest.ThrowsOnMixedColorImage`
- `ColorValidationFilterTest.ThrowsOnCrayonStrokeOnWhitePaper`
- `ColorValidationFilterTest.PipelineThrowsForColorImage`

위 4개 테스트가 모두 `ValidationException` 대신 통과해 실패했다.

로그 공통점:

- `color ratio = 0.0% (threshold = 5.0%, V<=220, S>=50, chroma>=26)`

## 원인

- 기존 구현은 `V <= 220`인 어두운 픽셀만 모수로 잡았다.
- 이 때문에 `BGR(0,0,255)` 같은 밝은 순색 입력은 `V=255`라서 전부 배경으로 제외됐다.
- 결과적으로 밝은 빨강/파랑 스트로크와 전체 컬러 이미지가 `color ratio = 0%`로 계산되는 회귀가 발생했다.

## 이번 수정

파일:

- `preprocess-server/src/filters/color_validation_filter.cpp`
- `preprocess-server/src/filters/color_validation_filter.h`
- `preprocess-server/src/core/pipeline_factory.cpp`

변경 내용:

- 컬러 후보는 기존대로 `S >= 50` 그리고 `chroma >= 26`으로 계산
- 종이 배경은 `V > 220`이면서 동시에 `color candidate`가 아닌 픽셀만 제외
- 최종 비율은 `stroke pixels`가 아니라 `non-paper pixels` 대비 컬러 픽셀 비율로 계산

의도:

- 흰 종이는 계속 제외
- 밝은 크레파스/색연필 스트로크는 더 이상 배경으로 누락되지 않음
- 스캔 틴트나 연필화의 약한 색편차는 `sat/chroma` 조건에서 계속 걸러짐

## 검증 상태

- 로컬 WSL에서 `preprocess-server/build`는 Windows용 CMake 캐시라 재사용 불가
- 새 WSL 빌드 디렉터리 구성도 `CrowConfig.cmake` 부재로 중단
- 따라서 이 세션에서는 실제 `ctest` 재실행까지 완료하지 못했고, 테스트 케이스와 입력값 기준으로 로직 일치 여부를 수동 검증했다

## 다음 검증 명령

의존성이 준비된 환경에서 아래 순서로 확인:

```bash
cmake --build preprocess-server/build --config Release
ctest --test-dir preprocess-server/build --output-on-failure -R ColorValidationFilterTest
ctest --test-dir preprocess-server/build --output-on-failure
```
