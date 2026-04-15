# AI Server 모델 운영 관리 가이드

> 사용 시점: EC2 운영 배포, 모델 교체, 핫픽스 재배포, 장애 점검
> 관련 문서: [DEPLOYMENT_CHECKLIST.md](DEPLOYMENT_CHECKLIST.md) | [AI_SERVER_MODEL_DEPLOYMENT_503_2026-04-15.md](../troubleshooting/AI_SERVER_MODEL_DEPLOYMENT_503_2026-04-15.md)

---

## 목적

이 문서는 `ai-server`가 ONNX 모델 파일을 안정적으로 읽도록 운영 환경에서 관리하는 방법을 정리한다.

특히 아래 유형의 장애를 예방하는 것이 목표다.

- `POST /analyze` 호출 시 `503`
- `{"detail":"모델이 로드되지 않았습니다."}` 응답
- `ai-server` 컨테이너가 떠 있지만 `unhealthy`
- `/health`는 200인데 실제 추론은 불가능한 상태

---

## 핵심 원칙

### 1. 모델 파일은 Git 저장소가 아니라 운영 아티팩트로 관리한다

- 현재 저장소는 `.gitignore`에 `ai-server/models/`가 포함되어 있다.
- 따라서 `git clone`, `git pull`만으로는 운영 서버에 모델 파일이 배포되지 않는다.
- 모델은 별도 운영 디렉터리에 배치하고 Docker bind mount로 공급해야 한다.

### 2. 운영 readiness는 "프로세스 생존"이 아니라 "모델 로드 완료" 기준이어야 한다

- `ai-server`는 서버 프로세스가 떠 있어도 모델이 없을 수 있다.
- 운영 판단은 반드시 `/health` 응답의 `models.male`, `models.female`, `engine_type`까지 본다.

### 3. 모델 교체는 코드 배포와 별도 자산 배포라고 생각한다

- 코드 재배포만으로는 모델이 바뀌지 않는다.
- 모델 업데이트는 파일 업로드, 경로 확인, 컨테이너 재기동, readiness 재검증까지 하나의 절차로 관리한다.

---

## 운영 구조

### 모델 보관 위치

운영 서버의 모델 디렉터리:

```bash
/opt/mind-palette/models
```

컨테이너 내부 마운트 경로:

```bash
/app/models
```

Compose 설정:

```yaml
- ${AI_MODEL_DIR:-./ai-server/models}:/app/models:ro
```

즉, 운영 서버에서 `AI_MODEL_DIR=/opt/mind-palette/models`로 두고, 해당 경로를 읽기 전용으로 컨테이너에 마운트하는 구조다.

---

## 필수 모델 파일

운영 시 최소 아래 4개 파일이 있어야 한다.

- `mind_palette_male.onnx`
- `mind_palette_male.onnx.data`
- `mind_palette_female.onnx`
- `mind_palette_female.onnx.data`

주의:

- `.onnx`만 있고 `.onnx.data`가 없으면 로드 실패 가능성이 높다.
- 파일명은 코드 기본 경로와 정확히 일치해야 한다.

---

## 배포 전 준비

### 1. 서버 디렉터리 준비

```bash
sudo mkdir -p /opt/mind-palette/models
sudo ls -la /opt/mind-palette/models
```

### 2. `.env` 확인

루트 `.env`에 아래 항목이 있어야 한다.

```dotenv
AI_MODEL_DIR=/opt/mind-palette/models
```

확인 포인트:

- 경로 오탈자 없음
- 실제 존재하는 절대 경로 사용
- 상대 경로보다 절대 경로 권장

### 3. 운영용 compose 적용 방식 확인

운영 배포에서는 개발용 override가 섞이지 않도록 주의한다.

권장:

```bash
docker compose -f docker-compose.yml up -d --build
```

주의:

- `docker compose up -d`만 쓰면 `docker-compose.override.yml`이 자동 병합될 수 있다.
- override가 적용되면 `NODE_ENV=development`, `KEEP_IMAGES=true`, 개발용 포트 노출이 섞일 수 있다.

사전 확인:

```bash
docker compose -f docker-compose.yml config | sed -n '/ai-server:/,/api-gateway:/p'
```

---

## 모델 업로드 절차

### 로컬 PC에서 EC2로 업로드

예시:

```bash
scp -i /path/to/your-key.pem \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_male.onnx \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_male.onnx.data \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_female.onnx \
  /mnt/c/Users/user/Documents/GitHub/mind-palette-project/ai-server/models/mind_palette_female.onnx.data \
  ubuntu@<EC2_PUBLIC_IP>:/tmp/
```

### EC2에서 운영 경로로 이동

```bash
sudo mkdir -p /opt/mind-palette/models
sudo mv /tmp/mind_palette_male.onnx /opt/mind-palette/models/
sudo mv /tmp/mind_palette_male.onnx.data /opt/mind-palette/models/
sudo mv /tmp/mind_palette_female.onnx /opt/mind-palette/models/
sudo mv /tmp/mind_palette_female.onnx.data /opt/mind-palette/models/
sudo ls -la /opt/mind-palette/models
```

확인 기준:

- 4개 파일이 모두 보여야 함
- 파일 크기가 0이 아니어야 함

---

## 배포 및 재기동 절차

### 운영 권장 명령

```bash
docker compose -f docker-compose.yml up -d --build ai-server api-gateway
```

이유:

- 운영에서는 기본 compose만 명시적으로 사용해 개발용 override 혼입을 방지하는 편이 안전하다.

### 상태 확인

```bash
docker compose -f docker-compose.yml ps
docker compose -f docker-compose.yml logs ai-server --tail=200
```

---

## 배포 후 검증 절차

### 1. 호스트 모델 디렉터리 확인

```bash
sudo ls -la /opt/mind-palette/models
```

### 2. 컨테이너 내부 마운트 확인

```bash
docker compose -f docker-compose.yml exec ai-server ls -la /app/models
```

정상 기준:

- 호스트와 동일한 4개 파일이 보여야 함

### 3. health 응답 확인

```bash
docker compose -f docker-compose.yml exec ai-server curl -s http://localhost:8082/health
```

정상 예시:

```json
{
  "status": "ok",
  "models": {
    "male": true,
    "female": true
  },
  "engine_type": "onnx"
}
```

확인 포인트:

- `models.male == true`
- `models.female == true`
- `engine_type != "none"`

### 4. analyze 실제 호출 검증

가능하면 정상 연필화 샘플 1장으로 실제 분석 요청까지 확인한다.

검증 목표:

- `503`이 아닌 성공 응답
- IQ/percentile 필드 존재

---

## 장애 대응 표준 절차

### 증상 1. `ai-server`가 `unhealthy`

확인 순서:

```bash
docker compose -f docker-compose.yml logs ai-server --tail=200
docker compose -f docker-compose.yml exec ai-server curl -s http://localhost:8082/health
docker compose -f docker-compose.yml exec ai-server ls -la /app/models
sudo ls -la /opt/mind-palette/models
```

우선 해석:

- `/app/models`가 비어 있으면 거의 항상 운영 모델 파일 미배치 문제

### 증상 2. `/health`는 200인데 `/analyze`는 503

확인 포인트:

- `/health` body의 `models` 값 확인
- `engine_type: "none"`인지 확인
- 시작 로그에 `ONNX model files are missing.`가 있는지 확인

### 증상 3. `docker compose exec ai-server python -c "import os; print(os.environ.get('AI_MODEL_DIR'))"`가 `None`

해석:

- 현재 구조에서는 정상일 수 있다.
- 이 값은 컨테이너 환경변수가 아니라 compose의 host-side 경로 치환용 변수일 수 있다.
- 장애 판단은 이 값이 아니라 `/app/models` 실파일 존재 여부로 해야 한다.

---

## 운영자가 기억해야 할 판단 규칙

### 규칙 1. `/health` 200만 보고 정상이라고 판단하지 않는다

반드시 응답 body의 아래 필드를 확인한다.

- `models.male`
- `models.female`
- `engine_type`

### 규칙 2. 모델 파일이 안 보이면 코드 문제보다 먼저 운영 자산 배치를 의심한다

이번 사례처럼 코드와 compose가 정상이어도 호스트 모델 디렉터리가 비어 있으면 동일 장애가 발생한다.

### 규칙 3. 운영 배포에서는 `docker-compose.override.yml` 자동 병합 여부를 항상 점검한다

확인 예시:

```bash
docker compose config | rg "NODE_ENV|KEEP_IMAGES|published:"
```

운영에서 의도하지 않은 값이 보이면 compose 실행 방식을 수정한다.

---

## 권장 운영 점검 루틴

배포 직후 아래 순서로 확인하는 것을 권장한다.

```bash
sudo ls -la /opt/mind-palette/models
docker compose -f docker-compose.yml ps
docker compose -f docker-compose.yml exec ai-server ls -la /app/models
docker compose -f docker-compose.yml exec ai-server curl -s http://localhost:8082/health
docker compose -f docker-compose.yml logs ai-server --tail=50
```

운영 중 정기 점검 시에는 최소 아래 두 개를 본다.

```bash
docker compose -f docker-compose.yml exec ai-server curl -s http://localhost:8082/health
docker compose -f docker-compose.yml logs ai-server --tail=50
```

---

## 장기 개선 제안

- S3 또는 별도 아티팩트 저장소에서 모델을 자동 다운로드하는 init 단계 도입
- 운영 배포용 compose 파일과 개발 override 파일을 더 명확히 분리
- `DEPLOYMENT_CHECKLIST.md`에 모델 아티팩트 점검 항목 추가
- 모델 버전 관리용 manifest 파일 도입

---

## 참고 문서

- [DEPLOYMENT_CHECKLIST.md](DEPLOYMENT_CHECKLIST.md)
- [aws-deployment-guide.md](aws-deployment-guide.md)
- [AI_SERVER_MODEL_DEPLOYMENT_503_2026-04-15.md](../troubleshooting/AI_SERVER_MODEL_DEPLOYMENT_503_2026-04-15.md)
