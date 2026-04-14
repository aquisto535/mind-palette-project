# Mind Palette — 환경 변수 인벤토리

> **목적**: 4개 서비스 전체의 환경 변수를 한 곳에서 파악하여 설정 누락·오남용을 예방한다.
> **마지막 업데이트**: 2026-04-13

---

## 범례

| 기호 | 의미 |
|------|------|
| ✅ | 프로덕션 필수 — 미설정 시 서비스 정상 동작 불가 |
| ⚠️ | 프로덕션 권장 — 기본값이 있으나 환경에 맞게 조정 필요 |
| 🔧 | 개발/디버그 전용 — 프로덕션에서 기본값(false/비활성) 유지 |
| 🔒 | 시크릿 — `.gitignore` 대상, 절대 코드에 하드코딩 금지 |

---

## 1. API Gateway (`api-gateway/`)

소스: `api-gateway/src/services/analysisService.ts`, `cacheService.ts`, `server.ts`

| 키 | 타입 | 기본값 | 중요도 | 설명 | 위험 (미설정 시) |
|----|------|--------|--------|------|-----------------|
| `NODE_ENV` | string | `development` | ✅ | `production` 설정 시 morgan 로그 포맷·레이트리미터 활성화 | 로그 과다 출력, Rate-limit 미적용 |
| `PORT` | number | `3000` | ⚠️ | API Gateway 리스닝 포트 | 포트 충돌 가능 |
| `PREPROCESS_SERVER_URL` | string | `http://127.0.0.1:8081` | ✅ | C++ 전처리 서버 URL | Docker 내 서비스 간 통신 불가 |
| `AI_SERVER_URL` | string | `http://127.0.0.1:8082` | ✅ | Python AI 서버 URL | 동상 |
| `ADMIN_PROFILE_KEY` | string | *(없음)* | 🔒✅ | `X-Admin-Profile-Key` 헤더 검증용 시크릿 | 미설정 시 프로파일링 엔드포인트 모두 차단 |
| `KEEP_IMAGES` | boolean | `false` | 🔧 | `true` 시 처리 후 이미지 삭제 안 함 | **프로덕션에서 true 설정 시 디스크 누수** |
| `CACHE_TTL_SECONDS` | number | `3600` | ⚠️ | LRU 캐시 항목 TTL(초) | 캐시 과도 유지 또는 미사용 |
| `CACHE_MAX_SIZE` | number | `100` | ⚠️ | LRU 캐시 최대 항목 수 | 메모리 초과 또는 캐시 히트율 저하 |

### 주의사항
- `ADMIN_PROFILE_KEY` 미설정 → `analysisService.ts:210` 에서 `envAdminKey`가 `undefined`가 되어 **모든 프로파일링 요청 거부**. 의도된 동작이지만 배포 후 프로파일링 기능이 무음으로 비활성화됨.
- `KEEP_IMAGES=true`를 프로덕션 `.env`에 설정하면 이미지가 삭제되지 않아 디스크 고갈 위험. **프로덕션 기본값은 반드시 `false`**.

---

## 2. Frontend (`frontend/`)

소스: `frontend/src/api/uploadApi.ts`, `frontend/src/api/client.ts`
빌드 도구: Vite — `VITE_*` 변수는 **빌드 시점에 번들에 인라인**됨 (런타임 변경 불가)

| 키 | 타입 | 기본값 | 중요도 | 설명 | 위험 (미설정 시) |
|----|------|--------|--------|------|-----------------|
| `VITE_USE_MOCK` | boolean | `false` | 🔧 | `true` 시 Mock 데이터 사용, 실제 API 호출 안 함 | **프로덕션 빌드에 Mock이 포함되면 실제 AI 분석 무력화** |
| `VITE_API_URL` | string | `/api` (상대경로) | ⚠️ | Axios baseURL. Nginx 리버스 프록시 사용 시 기본값 유효 | 직접 접근 시 API 호출 실패 |

### 핵심 경고 — Vite 빌드 타임 임베딩
```
.env.local  ──(빌드)──→  번들 내 하드코딩된 상수
.dockerignore가 .env.local 제외 → Docker 빌드 시 VITE_USE_MOCK = undefined
undefined === 'true'  →  false  →  실제 API 사용  (안전)
```
`.dockerignore`가 `.env.local`을 제외하는 것은 **의도된 안전 장치**다. 변경 금지.

---

## 3. AI Server (`ai-server/`)

소스: `ai-server/src/config.py` (Pydantic BaseSettings)

| 키 | 타입 | 기본값 | 중요도 | 설명 | 위험 (미설정 시) |
|----|------|--------|--------|------|-----------------|
| `INFERENCE_BACKEND` | string | `pytorch` | ✅ | 추론 엔진 (`pytorch` \| `onnx` \| `tensorrt_native` \| `tensorrt_ort`) | 프로덕션에서 PyTorch raw 사용 → 성능 저하 |
| `DEVICE` | string | `cpu` | ⚠️ | 추론 디바이스 (`cpu` \| `cuda`) | GPU 서버에서도 CPU 사용 |

### 주의사항
- `ModelConfig`의 모든 필드가 기본값을 가지므로 **환경 변수 미설정 시 무음으로 기본값 사용**. 모델 경로가 실제 파일과 다를 경우 런타임 오류가 뒤늦게 발생.
- docker-compose.yml에서 `INFERENCE_BACKEND: "onnx"` 명시 중 — 로컬 개발 환경과의 불일치 주의.

---

## 4. Preprocess Server (`preprocess-server/`)

소스: `docker-compose.yml` environment 섹션

| 키 | 타입 | 기본값 | 중요도 | 설명 | 위험 (미설정 시) |
|----|------|--------|--------|------|-----------------|
| `PREPROCESS_WORKERS` | number | *(없음, 필수)* | ✅ | 전처리 워커 수 (ADR-028: c5.large vCPU 2) | 미설정 시 서버 기본값으로 동작 (성능 미최적화) |

---

## 5. 전체 서비스 요약 매트릭스

| 서비스 | 필수(✅) | 권장(⚠️) | 개발전용(🔧) | 시크릿(🔒) |
|--------|---------|---------|------------|-----------|
| api-gateway | NODE_ENV, PREPROCESS_SERVER_URL, AI_SERVER_URL | PORT, CACHE_TTL_SECONDS, CACHE_MAX_SIZE | KEEP_IMAGES | ADMIN_PROFILE_KEY |
| frontend | — | VITE_API_URL | VITE_USE_MOCK | — |
| ai-server | INFERENCE_BACKEND | DEVICE | — | — |
| preprocess-server | PREPROCESS_WORKERS | — | — | — |

---

## 6. 알려진 리스크 (발견된 취약점)

| # | 리스크 | 영향 | 완화 방안 |
|---|--------|------|-----------|
| R-01 | `VITE_USE_MOCK=true`가 프로덕션 빌드에 포함될 경우 | Mock 데이터가 실제 분석 결과로 전달됨 | Step 3: prebuild 검증 스크립트 + Step 5: CI 감사 |
| R-02 | `KEEP_IMAGES=true` 실수 설정 | 프로덕션 서버 디스크 고갈 | `.env.example`에 명시적 경고 주석 |
| R-03 | `ADMIN_PROFILE_KEY` 미설정 | 프로파일링 기능 무음 비활성화 | Step 6: `/health` 응답에 활성화 여부 노출 |
| R-04 | `INFERENCE_BACKEND` 미설정 | CPU-only PyTorch로 fallback, 성능 저하 | ai-server `.env.example` + 시작 검증 |
| R-05 | Docker 빌드 시 `.env.local` 누락 (의도된 동작) | VITE_* 변수 undefined → 기본값 적용 | `frontend/Dockerfile`에 ARG/ENV 추가 (Step 4) |

---

## 7. 참고 문서

- [ADR-033: Color Filter Bypass Fix](../troubleshooting/ADR033_ColorFilter_Bypass_Fix.md) — Fail-Open 패턴이 초래한 우회 경로 사례
- [ADR-035: Config Management System](../standards/ARCHITECTURE_DECISIONS.md) — 이 인벤토리를 만들게 된 아키텍처 결정 (Step 7에서 작성)
- [Deployment Checklist](DEPLOYMENT_CHECKLIST.md) — 배포 전 체크리스트 (Step 7에서 작성)
- [git-workflow-guide.md](git-workflow-guide.md) — 커밋 전 Self-Review 체크리스트
