# Phase 6: System Reliability & Chaos Engineering

## Context

Phase 5까지 모든 서비스(C++, Python, Gateway, Frontend, Nginx)가 완성됐지만 **장애 탄력성은 아직 검증되지 않았다**:

- `invokeAiServer`는 axios 타임아웃 없음 → ai-server 지연 시 요청이 오래 pending 되며 연결/메모리 리소스 잠식
- Circuit Breaker 없음 → 사망한 서버에 계속 TCP 연결 시도
- Retry 없음 → 일시적 네트워크 오류에 취약
- Chaos Testing 없음 → 컨테이너 재시작 자동 복구 미검증
- Phase 5의 `scripts/load-test.js`는 구현됨 → Phase 6은 해당 k6 기준선을 바탕으로 장애 주입 시 회복성을 검증

목표: TDD로 Circuit Breaker + Retry 구현 → Docker Chaos 스크립트로 E2E 복구 검증.

---

## 현황 분석 (SPOF)

| 서비스 장애 | 현재 동작 | 변경 후 |
|---|---|---|
| preprocess-server 사망 | Fail-Closed: `PreprocessServiceError` → 503. 단, 사망 서버에 계속 TCP 연결 | OPEN 즉시 503. 원본 이미지 우회 금지 유지 |
| **ai-server 지연** | **무한 대기 → 요청 pending/리소스 잠식** | **30s timeout + OPEN 즉시 503** |
| ai-server 사망 | throw → 500. 타임아웃 없음 | OPEN 즉시 503 |
| api-gateway 사망 | Docker 자동 재시작 (미검증) | chaos-test.sh로 복구 시간 측정 |

---

## 구현 계획

### 1단계: TDD Red — 테스트 먼저 작성

**신규: `api-gateway/tests/circuitBreaker.test.ts`**
```
[CB-01] CLOSED: 성공 요청은 결과를 반환한다
[CB-02] CLOSED: timeout 초과 시 에러를 throw한다
[CB-03] volumeThreshold 충족 + 에러율 >50% → OPEN 전이
[CB-04] OPEN: fn 호출 없이 즉시 fallback 반환
[CB-05] OPEN: fn을 호출하지 않는다 (spy 검증)
[CB-06] resetTimeout 후 HALF_OPEN 전이, 시험 요청 허용
[CB-07] HALF_OPEN: 성공 → CLOSED 복귀
[CB-08] HALF_OPEN: 실패 → OPEN 재진입
[CB-09] 상태 전이 시 logger가 호출된다
```

**신규: `api-gateway/tests/failover.test.ts`**
```
# Failover 시나리오 (nock으로 HTTP 스터빙)
[FO-01] C++ 서버 500 × 5회 → preprocessBreaker OPEN → PreprocessServiceError 반환
[FO-02] preprocessBreaker OPEN 중 요청 → AI 서버 미호출, 즉시 503 반환
[FO-03] AI 서버 timeout × 5회 → aiBreaker OPEN → ServiceUnavailableError throw
[FO-04] aiBreaker OPEN 중 요청 → fn 미호출, 즉시 에러

# Chaos 시나리오
[CH-01] 처리 중 ai-server 연결 끊김 → processAnalysis reject 후 finally 블록 실행
[CH-02] reject 후 uploads/ 고아 파일 없음
[CH-03] CB OPEN 중 10개 동시 요청 → 모두 즉시 응답 (블로킹 없음)
[CH-04] HALF_OPEN 성공 후 이후 요청이 정상 처리됨
```

---

### 2단계: Tidy First (Structural 커밋)

**신규 파일: `api-gateway/src/services/circuitBreakerService.ts`**

```typescript
// 브레이커 설정
const PREPROCESS_BREAKER_OPTS = {
  timeout: 10_000,              // C++ P95 97ms의 100배
  errorThresholdPercentage: 50,
  resetTimeout: 30_000,         // Docker restart 감안
  volumeThreshold: 5,
  name: 'preprocess-server',
};

const AI_BREAKER_OPTS = {
  timeout: 30_000,              // ONNX 콜드스타트 고려
  errorThresholdPercentage: 50,
  resetTimeout: 60_000,         // Python 재시작 더 오래 걸림
  volumeThreshold: 5,
  name: 'ai-server',
};

// 에러 타입
export class ServiceUnavailableError extends Error { ... }

// 팩토리: 모듈 수준 싱글턴으로 export
export const preprocessBreaker: CircuitBreakerWrapper;
export const aiBreaker: CircuitBreakerWrapper;
```

opossum 라이브러리 사용. `opossum` 이벤트(`open`, `halfOpen`, `close`)에 Winston logger 연결.

이 커밋에서 `analysisService.ts`는 미수정.

---

### 3단계: Behavioral 커밋 — 브레이커 통합

**수정: `api-gateway/src/services/analysisService.ts`**

```typescript
// invokePreprocessServer 변경 (L78)
// 변경 전: axios.post(PREPROCESS_SERVER_URL/preprocess, ...)
// 변경 후: preprocessBreaker.fire(filePath, requestId, passKey)
// fallback: throws PreprocessServiceError('PREPROCESS_SERVICE_UNAVAILABLE')
// 보안 정책: ADR-033 Fail-Closed 유지. 전처리 장애 시 원본 이미지 우회 금지.

// invokeAiServer 변경 (L110)
// 변경 전: axios.post(AI_SERVER_URL/analyze, formData, { headers })
// 변경 후: aiBreaker.fire(processedImagePath, sanitized, requestId, passKey)
// fallback: throws ServiceUnavailableError('AI server unavailable')

// axios 명시적 타임아웃 추가
// preprocess: { timeout: 10_000 }
// ai: { timeout: 30_000 }
```

**수정: `api-gateway/src/routes/analyze.ts`** (또는 해당 라우터 파일)
- `ServiceUnavailableError` 타입 가드로 503 반환
- 기존 `PreprocessServiceError`는 계속 503으로 반환

---

### 4단계: Chaos Testing 스크립트

**신규: `scripts/chaos-test.sh`**
- 안전장치: `CHAOS_ENV=local` 또는 `CONFIRM_CHAOS_TEST=true` 없으면 즉시 종료
- 대상 확인: Docker Compose project name이 `mind-palette`인지 확인하고, 실행 전 대상 컨테이너 목록 출력
- Docker Compose SIGKILL 기반 4개 시나리오:
  1. preprocess-server SIGKILL
  2. ai-server SIGKILL
  3. api-gateway SIGKILL
  4. preprocess + ai 동시 종료
- 각 시나리오: SIGKILL → chaos-verify.ts 폴링 → 복구 확인 → chaos_results.json 기록
- `scripts/load-test.js`의 Phase 5 k6 결과를 정상 기준선으로 보관하고, Chaos 실행 후 회복 시간/SLO 위반 여부를 별도 기록

**신규: `scripts/chaos-verify.ts`**
```typescript
async function pollUntilHealthy(serviceUrl, maxWaitMs): Promise<RecoveryResult>
// 1단계: unhealthy 확인 (최대 10초)
// 2단계: healthy 복귀까지 폴링 (POLL_INTERVAL=3s, MAX_WAIT=120s)
// 3단계: 실제 분석 요청으로 데이터 무결성 검증
```

**복구 SLO**:
- preprocess-server: 최대 60초 (30s start_period + 3×10s)
- ai-server: 최대 50초 (20s start_period + 3×10s)
- api-gateway: 최대 45초 (15s start_period + 3×10s)

---

## 커밋 전략

```
커밋 1: chore(deps): add opossum circuit breaker library
커밋 2: test(reliability): RED - circuit breaker state machine tests
커밋 3: test(chaos): RED - failover and data integrity tests
커밋 4: feat(structural): add CircuitBreakerService factory [Tidy First]
커밋 5: feat(behavioral): integrate circuit breakers into analysisService
커밋 6: refactor: extract ServiceUnavailableError and breaker constants
커밋 7: feat(chaos): add chaos-test.sh and chaos-verify.ts scripts
```

---

## 수정/생성 파일 목록

| 파일 | 변경 | 비고 |
|---|---|---|
| `api-gateway/package.json` | 수정 | opossum, @types/opossum 추가 |
| `api-gateway/src/services/circuitBreakerService.ts` | **신규** | 브레이커 팩토리 + 에러 타입 |
| `api-gateway/src/services/analysisService.ts` | 수정 | L78, L110 — 브레이커 통합, axios timeout 추가 |
| `api-gateway/src/routes/analyze.ts` | 수정 | ServiceUnavailableError → 503 |
| `api-gateway/tests/circuitBreaker.test.ts` | **신규** | CB 단위 테스트 9개 |
| `api-gateway/tests/failover.test.ts` | **신규** | Failover + Chaos 통합 테스트 8개, 전처리 Fail-Closed 검증 |
| `scripts/chaos-test.sh` | **신규** | 안전장치 포함 SIGKILL 시나리오 자동화 |
| `scripts/chaos-verify.ts` | **신규** | 헬스체크 폴링 + 복구 검증 |

---

## 검증 방법

### 단위/통합 테스트
```bash
cd api-gateway
npm test -- --testPathPattern="(circuitBreaker|failover)"
# 예상: 17개 테스트 통과
```

### E2E Chaos 테스트 (Docker 환경)
```bash
docker compose up -d --build
CHAOS_ENV=local bash scripts/chaos-test.sh
# chaos_results.json에 복구 시간 기록
# 모든 시나리오의 복구 시간이 SLO 이내인지 확인
```

### 수동 검증 포인트
1. `docker compose kill -s SIGKILL preprocess-server` → Gateway 로그에 "circuit open" 메시지
2. 60초 후 → 로그에 "circuit half-open", "circuit closed" 순서로 출력
3. `curl -X POST http://localhost:3000/api/analyze -F image=@test.jpg` → preprocess OPEN 중 즉시 503, AI 서버 미호출
