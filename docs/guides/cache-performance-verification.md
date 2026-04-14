# 캐시 성능 검증 (EC2 실환경) — 실행 계획

## Context

plan.md의 `EC2 c5.large 실배포` 섹션 중 마지막 미완료 항목:

```
- [ ] 캐시 성능 검증 (EC2 실환경):
  - [ ] 동일 이미지 2회 요청 → 2번째 응답 < 10ms 확인
  - [ ] k6 부하 테스트 EC2에서 실행: `k6 run scripts/load-test.js`
  - [ ] 검증 기준: 100 VU 기준 P95 < 500ms, 실패율 < 1%
```

**목표**: 이미 프로덕션 배포된 EC2 환경에서 사용자가 브라우저로 이미지를 업로드하는 실제 흐름을 기준으로, 모듈별(Gateway / Preprocess / AI) 성능 수치를 측정하고 캐시 효과를 검증한다.

**핵심 메커니즘 (코드 기반)**:
- `analysisService.ts:210-237`: `X-Admin-Profile-Key` 헤더가 `.env`의 `ADMIN_PROFILE_KEY`와 일치할 때만 `Server-Timing` 응답 헤더를 반환
  - 형식: `gateway;dur=45.2, preprocess;dur=123.4, ai;dur=567.8`
- `analysisService.ts:194-199`: SHA-256 해시 기반 캐시 히트 시 `< 10ms` early return
- `cacheService.ts`: LRU + TTL 인메모리 캐시 (`CACHE_TTL_SECONDS=3600`)

---

## 단계별 실행 계획

### 사전 준비 — EC2 상태 확인

**목적**: 서비스 전체 정상 여부 확인

```bash
# 헬스 체크 (터미널에서 실행)
curl https://<도메인>/api/health | jq .
```

예상 응답: `{ "status": "ok", "mode": "production", "uptime": ..., "memory": ... }`

---

### Step 1 — 브라우저 DevTools로 E2E 응답 시간 측정

**목적**: 실제 사용자 경험 기준의 총 응답 시간 측정

1. Chrome에서 브라우저 접속 → **DevTools 열기** (F12 → Network 탭)
2. 이미지 업로드 후 `/api/analyze` 요청 선택
3. `Timing` 탭에서 `TTFB(Time To First Byte)` 확인
4. `Headers` 탭에서 `Server-Timing` 헤더 존재 여부 확인

> **주의**: `Server-Timing`은 `X-Admin-Profile-Key` 헤더가 포함된 요청에서만 반환됨.
> 기본 브라우저 요청에서는 나타나지 않음 — Step 2에서 curl로 처리.

---

### Step 2 — Admin Profile Key로 모듈별 성능 분리 측정 (curl)

**목적**: gateway / preprocess / ai-server 각 구간 소요 시간 측정

**파일**: `api-gateway/src/services/analysisService.ts:231-237`

```bash
# 1회차 요청 (캐시 미스 — 전체 파이프라인 실행)
curl -s -w "\n총 시간: %{time_total}s\n" \
  -H "X-Admin-Profile-Key: <ADMIN_PROFILE_KEY>" \
  -F "file=@테스트이미지.png" \
  https://<도메인>/api/analyze \
  -D - | grep -E "(Server-Timing|총 시간)"
```

**예상 출력**:
```
Server-Timing: gateway;dur=45.2, preprocess;dur=97.3, ai;dur=24.4
총 시간: 0.185s
```

**측정 대상 지표**:

| 모듈 | 목표 | 측정 항목 |
|------|------|-----------|
| gateway (전체) | 참고값 | 캐시 조회 + 오케스트레이션 오버헤드 |
| preprocess (C++) | < 100ms | Crow + OpenCV FilterPipeline (ADR 기준: 97ms) |
| ai (Python ONNX) | < 30ms | ONNX Runtime EfficientNet-B2 추론 |

---

### Step 3 — 캐시 히트 검증 (동일 이미지 2회 요청)

**목적**: SHA-256 해시 기반 캐시가 프로덕션에서 동작하는지 확인

**파일**: `api-gateway/src/services/analysisService.ts:194-199`

```bash
# 2회차 요청 — 동일한 이미지 파일 재사용
curl -s -w "\n총 시간: %{time_total}s\n" \
  -H "X-Admin-Profile-Key: <ADMIN_PROFILE_KEY>" \
  -F "file=@테스트이미지.png" \
  https://<도메인>/api/analyze \
  -D - | grep -E "(Server-Timing|총 시간)"
```

**합격 기준**:
- `time_total` < 0.010s (10ms)
- `Server-Timing` 헤더 없음 (캐시 히트 early return 시 프로파일링 코드 미실행)
- 응답 body의 분석 결과가 1회차와 동일

---

### Step 4 — k6 부하 테스트 (로컬 → EC2 대상)

**목적**: 100 VU 동시 사용자 기준 P95 응답 시간 및 에러율 검증

**파일**: `scripts/load-test.js`

```bash
# 로컬 머신에서 EC2 도메인을 대상으로 실행
# (k6 미설치 시: https://k6.io/docs/getting-started/installation/ )
k6 run \
  -e BASE_URL=https://<도메인> \
  scripts/load-test.js
```

**합격 기준** (plan.md 명시):

| 지표 | 목표 |
|------|------|
| P95 응답 시간 | < 500ms |
| 에러율 | < 1% |
| VU | 100 |

> k6 스크립트 내 시나리오:
> - Smoke: 5 VU, 5s
> - Load: 100 VU, 30s ← **이것이 검증 기준**
> - Stress: 200 VU, 1m

---

### Step 5 — 결과 기록 및 plan.md 체크

**측정값 기록 예시** (plan.md 또는 별도 측정 메모):
```
캐시 미스 응답: xxx ms (gateway: x, preprocess: x, ai: x)
캐시 히트 응답: x ms
k6 P95: xxx ms, 에러율: x%
```

**plan.md에서 체크할 항목**:
```
- [x] 동일 이미지 2회 요청 → 2번째 응답 < 10ms 확인
- [x] k6 부하 테스트 EC2에서 실행: `k6 run scripts/load-test.js`
- [x] 검증 기준: 100 VU 기준 P95 < 500ms, 실패율 < 1%
```

---

## 핵심 파일 참조

| 파일 | 관련 기능 |
|------|-----------|
| `api-gateway/src/services/analysisService.ts` | Server-Timing 생성, 캐시 히트 early return |
| `api-gateway/src/services/cacheService.ts` | LRU+TTL 캐시 구현 |
| `scripts/load-test.js` | k6 시나리오 정의 |
| `.env.example` | ADMIN_PROFILE_KEY, CACHE_TTL_SECONDS |

---

## 구현 작업 없음 — 실행만 필요

이 계획은 **코드 변경 없이** 기존 인프라를 활용한 측정 절차입니다.
모든 측정 메커니즘(Server-Timing, 캐시, k6)이 이미 구현되어 있습니다.
