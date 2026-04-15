# AI Server 모델 미로드로 인한 503 트러블슈팅

작성일: 2026-04-15 ~ 2026-04-16

## 증상

- 배포 후 `남자사람_8_남_06463.jpg` 테스트 시 `api-gateway`에서 `503 Service Unavailable` 발생
- `api-gateway` 로그:

```text
Analysis Error: Request failed with status code 503
upstreamData: {"detail":"모델이 로드되지 않았습니다."}
```

- `ai-server` 로그:

```text
POST /analyze HTTP/1.1" 503 Service Unavailable
```

## 1차 해석

- 요청은 `api-gateway -> ai-server`까지 정상 전달됨
- 실패 지점은 업로드/전처리 단계가 아니라 `ai-server` 내부 추론 직전
- [ai-server/src/routes/analyze.py](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/src/routes/analyze.py:292) 기준으로 `model_state.engine_type == "none"`일 때 `503`을 반환하는 구조

문제 분기:

```python
model_state = getattr(request.app.state, "model_state", None)
if not model_state or getattr(model_state, "engine_type", "none") == "none":
    raise HTTPException(status_code=503, detail="모델이 로드되지 않았습니다.")
```

## 코드 관찰 결과

### 모델 로드 조건

- [ai-server/src/infra/model_loader.py](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/src/infra/model_loader.py:27) 에서 ONNX 모델 두 개가 모두 존재해야 엔진 로드
- 기본 경로:
  - `models/mind_palette_male.onnx`
  - `models/mind_palette_female.onnx`

### 기존 배포 설정 문제

- `docker-compose.yml`의 `ai-server`는 원래 `shared_volume`만 마운트하고 있었음
- 저장소의 [`.gitignore`](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/.gitignore:93)에 `ai-server/models/`가 포함되어 있어 서버에서 `git clone/pull`로는 모델 파일이 따라오지 않음
- 결과적으로 컨테이너는 뜨지만 `/app/models`는 비어 있고, 서버는 살아 있으나 모델은 로드되지 않는 상태가 됨

## 이번에 반영한 코드 수정

### 1. 모델 디렉터리 바인드 마운트 추가

파일: [docker-compose.yml](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/docker-compose.yml:28)

```yaml
ai-server:
  volumes:
    - shared_volume:/shared_volume
    - ${AI_MODEL_DIR:-./ai-server/models}:/app/models:ro
```

의도:

- 운영 서버의 실제 모델 디렉터리를 컨테이너 `/app/models`로 마운트
- Git에 모델 파일을 넣지 않아도 운영 시 외부 경로에서 공급 가능

### 2. readiness healthcheck 강화

파일: [docker-compose.yml](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/docker-compose.yml:40)

```yaml
healthcheck:
  test:
    [
      "CMD",
      "python",
      "-c",
      "import json, sys, urllib.request; data = json.load(urllib.request.urlopen('http://localhost:8082/health')); sys.exit(0 if data.get('models', {}).get('male') and data.get('models', {}).get('female') else 1)",
    ]
```

의도:

- 기존에는 `/health`가 200이면 healthy 판정
- 수정 후에는 남/여 모델이 실제 로드된 경우만 healthy
- 서버가 “살아 있지만 모델이 없는 상태”를 운영 레벨에서 즉시 감지 가능

### 3. 모델 누락 로그 추가

파일: [ai-server/src/infra/model_loader.py](/mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/src/infra/model_loader.py:33)

```python
missing_onnx_paths = [
    path
    for path in (config.male_onnx_path, config.female_onnx_path)
    if not Path(path).exists()
]
if missing_onnx_paths:
    logger.error("ONNX model files are missing.", missing_paths=missing_onnx_paths)
```

의도:

- 추론 시점까지 기다리지 않고 앱 시작 직후 누락 경로를 로그에 남김

## 운영 환경에서 실제 확인한 증거

### `.env`

```dotenv
PREPROCESS_WORKERS=2
CACHE_TTL_SECONDS=3600
NODE_ENV=production
ADMIN_PROFILE_KEY=SECRET-9396
AI_MODEL_DIR=/opt/mind-palette/models
```

### `docker compose config`

중요 확인점:

- `/opt/mind-palette/models -> /app/models` 바인드 마운트가 정상적으로 해석됨

```yaml
volumes:
  - type: bind
    source: /opt/mind-palette/models
    target: /app/models
    read_only: true
```

판단:

- compose 변수 치환 자체는 정상
- `AI_MODEL_DIR` 설정 누락 문제는 아님

### 호스트 경로 확인

실행:

```bash
sudo ls -la /opt/mind-palette/models
```

결과:

```text
total 8
drwxr-xr-x 2 root root 4096 Apr 15 14:42 .
drwxr-xr-x 3 root root 4096 Apr 15 14:42 ..
```

판단:

- 호스트 모델 디렉터리가 완전히 비어 있음

### 컨테이너 내부 확인

실행:

```bash
docker compose exec ai-server ls -la /app/models
```

결과:

- 컨테이너 내부 `/app/models`도 빈 디렉터리

### health 응답 확인

실행:

```bash
docker compose exec ai-server curl -s http://localhost:8082/health
```

결과:

```json
{
  "status":"ok",
  "models":{"male":false,"female":false},
  "engine_type":"none"
}
```

판단:

- 프로세스는 살아 있지만 모델은 로드되지 않음
- 새 healthcheck가 의도대로 `unhealthy` 판정

### `AI_MODEL_DIR` 환경변수 확인 시 `None`이 나온 이유

실행:

```bash
docker compose exec ai-server python -c "import os; print(os.environ.get('AI_MODEL_DIR'))"
```

결과:

```text
None
```

해석:

- 이 값은 현재 컨테이너 `environment:`로 주입한 값이 아니라, compose 파일의 **호스트 볼륨 source 경로 치환용 변수**로만 사용 중
- 따라서 컨테이너 내부에서 `None`이 보이는 건 비정상이 아님
- 실제 장애 판단에는 영향을 주지 않음

## 최종 원인

이번 503의 직접 원인은 다음과 같다.

- 운영 서버의 `/opt/mind-palette/models` 경로가 비어 있었다.
- 그 결과 `ai-server` 컨테이너의 `/app/models`도 비어 있었다.
- ONNX 모델 파일이 없어 `ai-server`는 `engine_type = "none"`으로 기동했다.
- `/analyze` 요청 시 `모델이 로드되지 않았습니다.`로 `503`을 반환했다.
- 강화된 healthcheck가 이 상태를 `unhealthy`로 판정하면서 `api-gateway`의 의존성 기동도 함께 막혔다.

즉, **compose 설정 불량이 아니라 운영 모델 아티팩트 미배포 문제**였다.

## 부수 관찰

`docker compose config` 출력에서 다음이 보였다.

- `NODE_ENV: development`
- `KEEP_IMAGES: "true"`
- `3000`, `8082`, `5173` 호스트 포트 직접 노출

이는 `docker-compose.override.yml`도 함께 적용되고 있다는 뜻이다.

이 항목들은 이번 503의 직접 원인은 아니지만, 운영 배포에서는 확인이 필요하다.

- 개발용 override가 운영에 섞였는지
- 운영에서 `KEEP_IMAGES=true`가 의도인지
- `NODE_ENV=development`가 실제 의도인지

## 복구 절차

### 1. 모델 파일 업로드

필요 파일:

- `mind_palette_male.onnx`
- `mind_palette_male.onnx.data`
- `mind_palette_female.onnx`
- `mind_palette_female.onnx.data`

예시 `scp`:

```bash
scp -i /path/to/your-key.pem \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_male.onnx \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_male.onnx.data \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_female.onnx \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_female.onnx.data \
  ubuntu@<EC2_PUBLIC_IP>:/tmp/
```

### 2. 서버에 배치

```bash
sudo mkdir -p /opt/mind-palette/models
sudo mv /tmp/mind_palette_male.onnx /opt/mind-palette/models/
sudo mv /tmp/mind_palette_male.onnx.data /opt/mind-palette/models/
sudo mv /tmp/mind_palette_female.onnx /opt/mind-palette/models/
sudo mv /tmp/mind_palette_female.onnx.data /opt/mind-palette/models/
sudo ls -la /opt/mind-palette/models
```

### 3. 재기동

```bash
docker compose up -d --build ai-server api-gateway
```

### 4. 검증

```bash
docker compose exec ai-server ls -la /app/models
docker compose exec ai-server curl -s http://localhost:8082/health
docker compose logs ai-server --tail=200
```

정상 기준:

- `/app/models`에 4개 파일 존재
- `/health`에서 `models.male == true`, `models.female == true`
- `engine_type == "onnx"` 또는 기대하는 엔진 타입

## 재발 방지 제안

- 배포 체크리스트에 “운영 모델 아티팩트 존재 여부”를 명시적으로 추가
- `/health`와 별도로 readiness 성격을 문서화
- 모델을 수동 복사하지 않도록 장기적으로는 S3/아티팩트 저장소 + init 단계 자동 다운로드 도입 검토
- 운영 배포 시 `docker-compose.override.yml`이 섞이지 않도록 실행 명령 또는 파일 전략 점검
