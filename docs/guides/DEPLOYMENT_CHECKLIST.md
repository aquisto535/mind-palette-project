# Mind Palette — 배포 전 체크리스트

> **사용 시점**: EC2 배포 전, 핫픽스 배포 전, 신규 기여자 첫 배포 전
> **관련 문서**: [CONFIG_INVENTORY.md](CONFIG_INVENTORY.md) | [ADR-035](../architecture/ARCHITECTURE_DECISIONS.md#adr-035)

---

## 1단계: 환경 변수 설정 확인

### 루트 `.env` (API Gateway 주입용)

- [ ] `ADMIN_PROFILE_KEY` — 강력한 랜덤 값으로 설정 (`SECRET-1234` 같은 예시값 절대 사용 금지)
- [ ] `PREPROCESS_SERVER_URL` — Docker Compose 서비스명 사용 (`http://preprocess-server:8081`)
- [ ] `AI_SERVER_URL` — Docker Compose 서비스명 사용 (`http://ai-server:8082`)
- [ ] `KEEP_IMAGES` — 반드시 `false` (또는 미설정) 확인
- [ ] `NODE_ENV` — `production` 설정 확인 (docker-compose.yml에 하드코딩되어 있음)

### Frontend Docker 빌드 설정

- [ ] `docker-compose.yml`의 `frontend.build.args.VITE_USE_MOCK` — `"false"` 확인
- [ ] `frontend/.dockerignore`에 `.env.local` 이 포함되어 있는지 확인 (의도된 안전 장치)
- [ ] `.env.local` 파일이 서버에 존재하지 않는지 확인 (개발 PC에만 있어야 함)

### AI Server 설정

- [ ] `INFERENCE_BACKEND` — `onnx` (CPU 서버) 또는 `tensorrt_ort` (GPU 서버)

---

## 2단계: 빌드 및 시작 검증

```bash
# 1. 환경 변수 감사 스크립트 실행
bash scripts/ci/audit-env.sh

# 2. Docker Compose 빌드 (캐시 없이)
docker-compose build --no-cache

# 3. 서비스 시작
docker-compose up -d

# 4. 모든 서비스 헬스 체크 대기
docker-compose ps
```

---

## 3단계: 배포 후 가시성 검증

```bash
# API Gateway 런타임 모드 확인
curl -s https://<도메인>/api/health | jq '.mode'
```

예상 응답:

```json
{
  "env": "production",
  "admin_profiling_enabled": true,
  "keep_images": false,
  "preprocess_url": "http://preprocess-server:8081",
  "ai_url": "http://ai-server:8082"
}
```

### 확인 항목

- [ ] `env` = `"production"`
- [ ] `admin_profiling_enabled` = `true` (ADMIN_PROFILE_KEY 설정 확인)
- [ ] `keep_images` = `false`
- [ ] `preprocess_url` / `ai_url` — `localhost` 또는 `127.0.0.1` 이 아닌지 확인

---

## 4단계: 기능 스모크 테스트

```bash
# 연필 스케치 이미지로 실제 분석 요청
curl -s -X POST https://<도메인>/api/analyze \
  -F "image=@tests/fixtures/sample_sketch.jpg" \
  -F 'childInfo={"name":"테스트","age":7,"gender":"male"}' \
  | jq '{score, percentile}'

# 컬러 이미지로 필터링 확인 (422 응답이어야 함)
curl -s -o /dev/null -w "%{http_code}" -X POST https://<도메인>/api/analyze \
  -F "image=@tests/fixtures/sample_color.jpg"
# → 422 기대
```

- [ ] 연필 스케치 → 정상 분석 결과 반환 (score, percentile 존재)
- [ ] 컬러 이미지 → `422 Unprocessable Entity` 반환

---

## 5단계: 로그 확인

```bash
# API Gateway 로그에서 오류 확인
docker-compose logs api-gateway --tail=50 | grep -E "ERROR|WARN"

# C++ 전처리 서버 로그
docker-compose logs preprocess-server --tail=20

# Python AI 서버 로그
docker-compose logs ai-server --tail=20
```

- [ ] ERROR 레벨 로그 없음
- [ ] `[env-check]` 경고가 있으면 내용 확인 후 조치

---

## 알려진 위험 신호 (Red Flags)

| 증상 | 원인 | 조치 |
| ---- | ---- | ---- |
| `/health` 응답의 `keep_images: true` | `KEEP_IMAGES=true` 설정 오류 | `.env`에서 제거 후 재시작 |
| `/health` 응답의 `admin_profiling_enabled: false` | `ADMIN_PROFILE_KEY` 미설정 | `.env`에 키 추가 후 재시작 |
| 컬러 이미지가 422 대신 분석 결과 반환 | Color Filter 우회 (ADR-033 참고) | [ADR033_ColorFilter_Bypass_Fix.md](../architecture/ADR033_ColorFilter_Bypass_Fix.md) 참조 |
| Frontend가 Mock 데이터 반환 | `VITE_USE_MOCK=true`로 빌드됨 | `docker-compose build --no-cache frontend` 후 재배포 |

---

## 참고 문서

- [CONFIG_INVENTORY.md](CONFIG_INVENTORY.md) — 전체 환경 변수 목록 및 위험 등급
- [ADR-033: Color Filter Bypass Fix](../architecture/ADR033_ColorFilter_Bypass_Fix.md)
- [ADR-035: Config Management System](../architecture/ARCHITECTURE_DECISIONS.md#adr-035)
- [git-workflow-guide.md](git-workflow-guide.md) — 배포 브랜치 전략
