# 2026-04-15 ai-server 모델 미로드 503 진단 메모

## 증상

- 배포 후 `남자사람_8_남_06463.jpg` 업로드 시 `api-gateway`에서 `503` 발생
- `api-gateway` 로그의 upstream body:
  - `{"detail":"모델이 로드되지 않았습니다."}`
- `ai-server`는 `/analyze` 진입까지는 성공했지만, 추론 직전 `model_state.engine_type == "none"`으로 중단

## 원인

- `ai-server/src/routes/analyze.py`는 앱 시작 시 적재된 `app.state.model_state`를 사용한다.
- `ai-server/src/infra/model_loader.py`는 `models/mind_palette_male.onnx`, `models/mind_palette_female.onnx`가 둘 다 있어야 ONNX 엔진을 로드한다.
- 그런데 현재 배포 구성의 `docker-compose.yml`은 `shared_volume`만 마운트하고, 모델 디렉터리(`/app/models`)는 컨테이너에 공급하지 않는다.
- 동시에 저장소의 `.gitignore`에 `ai-server/models/`가 포함되어 있어, 서버에서 Git clone/pull 기반으로 배포하면 모델 파일이 작업트리에 존재하지 않을 가능성이 높다.
- 결과적으로 컨테이너는 기동되지만 `RUN mkdir -p models`로 생성된 빈 디렉터리만 가진 채 올라오고, 첫 분석 요청에서 `503`을 반환한다.

## 이번 수정

파일:

- `docker-compose.yml`
- `ai-server/src/infra/model_loader.py`

변경 내용:

- `ai-server` 서비스에 `${AI_MODEL_DIR:-./ai-server/models}:/app/models:ro` 바인드 마운트 추가
- `ai-server` healthcheck가 단순 `200 OK`가 아니라 `models.male == true && models.female == true`일 때만 healthy 판정하도록 강화
- 모델 파일이 없으면 `model_loader`가 누락 경로를 에러 로그로 남기도록 보강

## 배포 시 즉시 할 일

1. 배포 서버에 실제 모델 파일 4개를 준비
   - `mind_palette_male.onnx`
   - `mind_palette_male.onnx.data`
   - `mind_palette_female.onnx`
   - `mind_palette_female.onnx.data`
2. 서버 `.env` 또는 쉘 환경에 모델 경로 지정
   - 예: `AI_MODEL_DIR=/opt/mind-palette/models`
3. 해당 경로에 위 파일들을 배치
4. `docker compose up -d --build ai-server api-gateway`
5. `curl http://localhost:8082/health`에서 `models.male`, `models.female`가 모두 `true`인지 확인

## 재발 방지 포인트

- 현재 `health` API는 200을 주더라도 모델 미로드일 수 있으므로, readiness는 응답 body까지 확인해야 한다.
- 모델 아티팩트가 Git에 포함되지 않는 운영 방식이라면, 장기적으로는 S3/아티팩트 스토어에서 내려받는 init step 또는 별도 배포 문서가 필요하다.

## 운영 로그 추가 해석

- `.env`에 `AI_MODEL_DIR=/opt/mind-palette/models`가 있어도, 그 값은 현재 `docker-compose.yml`에서 **볼륨 source 경로 치환용**으로만 사용된다.
- 따라서 `docker compose exec ai-server python -c "import os; print(os.environ.get('AI_MODEL_DIR'))"`가 `None`인 것은 이상 현상이 아니다. 컨테이너 환경변수로는 주입하지 않았기 때문이다.
- 진짜 핵심은 `docker compose exec ai-server ls -la /app/models` 결과가 비어 있다는 점이다.
- 이 상태는 보통 두 경우 중 하나다.
  - 호스트의 `/opt/mind-palette/models`가 비어 있음
  - 호스트의 `/opt/mind-palette/models`가 아예 없어서 Docker가 빈 디렉터리를 자동 생성함
- `/health` 응답의 `"models":{"male":false,"female":false},"engine_type":"none"`는 위 해석과 정확히 일치한다.

## 2026-04-16 운영 확인 결과

- `sudo ls -la /opt/mind-palette/models` 결과가 빈 디렉터리였다.
- `docker compose config`에서도 `source: /opt/mind-palette/models -> target: /app/models` 바인드 마운트가 정상적으로 잡혔다.
- 따라서 이번 장애의 결론은 **compose 설정 불량이 아니라 호스트 모델 아티팩트 부재**다.
- 추가로 `docker compose config`에 `NODE_ENV: development`, `KEEP_IMAGES: "true"`, `8082/3000/5173` 직접 포트 노출이 보이므로 `docker-compose.override.yml`도 함께 적용된 상태다.
- 이 override 적용은 현재 503의 직접 원인은 아니지만, 운영 배포라면 의도 여부를 다시 확인하는 편이 좋다.

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
