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
