# 🚦 트래픽 생성 및 부하 테스트 가이드 (Traffic & Load Testing Guide)

이 문서는 Mind Palette 프로젝트에서 시스템의 안정성을 검증하고 성능 목표(전처리 < 100ms)를 달성하기 위해 사용할 수 있는 다양한 트래픽 생성 및 부하 테스트 방법론을 정리합니다.

---

## 1. 단계별 권장 방법 (Mind Palette 추천)

### 단계 1: 초간단 쉘 봇 (현 단계 로깅 확인용)
별도 설치 없이 터미널에서 즉시 실행하여 서버 활성화 상태를 유지하고 기본 로그를 생성합니다.
*   **PowerShell**: `while($true) { Invoke-RestMethod -Uri "http://localhost:3000/health" -Method Get; Start-Sleep -Milliseconds 500 }`
*   **Bash**: `while true; do curl -i http://localhost:3000/health; sleep 0.5; done`

### 단계 2: Node.js 기반 트래픽 봇 (Phase 2~3)
프로젝트 내 `api-gateway` 환경을 활용하여 실제 이미지 업로드 흐름을 모사합니다.
*   **장점**: 추가 도구 설치 불필요, `axios` 등 익숙한 라이브러리 사용.
*   **용도**: `spdlog` 파일 회전 테스트, 서비스 간 Request ID 전파 확인.

### 단계 3: k6 부하 테스트 (Phase 4~5)
현대적인 JavaScript 기반 부하 테스트 도구로 전처리 서버의 성능 한계를 측정합니다.
*   **장점**: 매우 높은 동시성(VU) 처리, p95/p99 응답 시간 자동 분석.
*   **용도**: 전처리 속도 100ms 달성 여부 증명, 멀티스레딩 성능 벤치마크.

---

## 2. 도구별 분류 및 특징

### 🚀 고성능 RPS 측정 (단순 벤치마킹)
*   **Apache Benchmark (ab)**: 고전적이고 심플한 HTTP 테스트 도구.
*   **wrk / wrk2**: C 기반의 최강 성능. 단순 RPS 측정의 표준.
*   **Vegeta**: 일정한 요청 비율(Constant Rate) 유지에 특화.

### 🎭 시나리오 기반 테스트 (사용자 패턴 모사)
*   **Locust (Python)**: 파이썬 코드로 유연한 시나리오 작성 가능. 대시보드가 직관적임.
*   **JMeter (Java)**: 업계 표준. 강력한 기능과 복잡한 프로토콜 지원. GUI 중심.
*   **Artillery (Node.js)**: YAML 기반 설정. 클라우드(Lambda) 연동 부하 생성에 강점.

### 🔄 실트래픽 재현 및 기타
*   **GoReplay**: 실제 운영 트래픽을 가로채서 테스트 서버에 재현.
*   **Playwright / Puppeteer**: 실제 브라우저를 띄워 렌더링 포함 E2E 부하 측정.
*   **Newman (Postman)**: 작성된 Postman 컬렉션을 CLI에서 반복 실행.

---

## 3. 상황별 도구 선택 가이드

| 테스트 목적 | 추천 도구 | 이유 |
| :--- | :--- | :--- |
| **빠른 로깅 확인** | Shell Script | 즉시 실행 가능, 오버헤드 없음 |
| **이미지 분석 흐름 검증** | Node.js Bot | 기존 API 로직 및 라이브러리 재활용 |
| **극도로 높은 동시성** | `wrk` | 시스템 리소스를 적게 먹고 대량 요청 가능 |
| **성능 최적화 근거 마련** | `k6` | 가독성 좋은 보고서와 높은 신뢰도 |
| **복잡한 비즈니스 로직** | `Locust` | 파이썬 코드로 정교한 사용자 행동 설계 |

---

## 4. Mind Palette 적용 전략
1.  **개발 단계 (현 지점)**: Node.js 봇을 활용해 `spdlog` 및 `Winston` 연동 무결성 확인.
2.  **최적화 단계 (Phase 4)**: k6를 도입하여 C++ 서버의 100ms 목표 달성 공식 리포트 작성.
3.  **통합 단계 (Phase 5)**: 필요시 Locust를 사용하여 전체 파이프라인(Node-C++-Python) 부하 테스트 수행.

---

## 5. Traffic Generation Phase 3~4 구현 계획 (TDD)

> **목표**: 대량 로그 발생 시 파일 I/O 병목 및 시스템 영향도를 검증하고, 로그 로테이션 동작을 TDD로 검증한다.

### 5.1 사전 작업: Winston 로그 로테이션 추가

**현재 상태**: `api-gateway/src/utils/logger.ts`에 `winston.transports.File`만 사용 중 → 로테이션 없음.

**필요 작업**:
- `winston-daily-rotate-file` 패키지 추가 (`npm install winston-daily-rotate-file`)
- `logger.ts`의 파일 트랜스포트를 `DailyRotateFile`로 교체

**로테이션 설정**:
```typescript
new DailyRotateFile({
  filename: path.join(LOG_DIR, 'combined-%DATE%.log'),
  datePattern: 'YYYY-MM-DD',
  maxSize: '10m',       // 10MB 초과 시 로테이션
  maxFiles: '7d',       // 7일치 보관
  zippedArchive: true,  // 압축 보관
})
```

**수정 파일**: `api-gateway/package.json`, `api-gateway/src/utils/logger.ts`

---

### 5.2 TDD Red — 테스트 작성

**신규 파일**: `api-gateway/tests/trafficBot.test.ts`

| 테스트 | 검증 내용 |
|--------|---------|
| 로그 크기 증가 | Traffic Bot N건 요청 후 `combined.log` 크기 증가 확인 |
| 로테이션 발동 | `maxSize` 초과 시 새 로그 파일 생성 확인 |
| 봇 기본 기능 | 지정 횟수만큼 요청 발송 및 성공/실패 카운트 집계 |
| 주기적 요청 | `intervalMs` 설정에 따라 반복 요청 수행 확인 |

**테스트 전략**:
- `nock`으로 C++ 서버(8081)·Python AI 서버(8082) 응답 모킹
- `supertest`로 Express 앱에 직접 요청 → 실제 Winston 로깅 발생
- 테스트용 로그 디렉토리를 임시 경로로 격리 (기존 로그와 충돌 방지)
- `NODE_ENV=test` 환경에서 rate limiter 자동 skip (기존 `skip: () => process.env.NODE_ENV === 'test'` 로직 활용)

---

### 5.3 TDD Green — TrafficBot 구현

**신규 파일**: `api-gateway/src/tools/trafficBot.ts`

```typescript
interface TrafficBotConfig {
  targetUrl: string;      // 대상 서버 (default: http://localhost:3000)
  endpoint: string;       // 엔드포인트 (default: /analyze)
  intervalMs: number;     // 요청 간격 ms (default: 1000)
  totalRequests: number;  // 총 요청 수 (default: 100)
  concurrency: number;    // 동시 요청 수 (default: 1)
}

interface TrafficBotResult {
  totalSent: number;
  successCount: number;
  failureCount: number;
  avgResponseMs: number;
  p95ResponseMs: number;
  errors: { status: number; count: number }[];
}
```

**핵심 구현 포인트**:
- `axios` + `FormData`로 `multipart/form-data` 요청 (실제 `/analyze` 형식)
- 더미 JPEG 이미지 인메모리 생성 (유효한 매직 바이트 `FF D8 FF` 포함, 검증 통과)
- `X-Request-ID` 헤더 자동 부여 (분산 추적용)
- P95 응답시간 계산 및 결과 집계

**CLI 실행 스크립트**: `api-gateway/src/tools/runTrafficBot.ts`

```bash
# 실행 예시
npx ts-node src/tools/runTrafficBot.ts --requests 100 --interval 6000
```

> **주의**: CLI 실행 시 실제 rate limit(10 req/min)이 적용되므로 `--interval 6000` 이상 권장.

---

### 5.4 파일 변경 요약

| 액션 | 파일 | 설명 |
|------|------|------|
| 수정 | `api-gateway/package.json` | `winston-daily-rotate-file` 의존성 추가 |
| 수정 | `api-gateway/src/utils/logger.ts` | DailyRotateFile 트랜스포트 적용 |
| 생성 | `api-gateway/tests/trafficBot.test.ts` | TDD Red 테스트 |
| 생성 | `api-gateway/src/tools/trafficBot.ts` | TrafficBot 클래스 구현 |
| 생성 | `api-gateway/src/tools/runTrafficBot.ts` | CLI 실행 스크립트 |

---

### 5.5 커밋 전략 (Tidy First)

```
feat(api-gateway): add Winston log rotation with DailyRotateFile
feat(api-gateway): add axios-based traffic bot with TDD
```

> 구조적 변경(logger.ts 수정)과 기능적 변경(trafficBot 추가)은 별도 커밋으로 분리.

---

### 5.6 검증 방법

1. `cd api-gateway && npm test` — 기존 테스트 + 새 트래픽 봇 테스트 전체 통과
2. `logs/` 디렉토리에서 로테이션된 파일 확인 (예: `combined-2026-03-23.log`)
3. CLI 수동 실행 후 결과 리포트 출력 확인:
   ```
   [TrafficBot] 완료: 100/100 성공, avgResponseMs=38ms, P95=62ms
   ```
