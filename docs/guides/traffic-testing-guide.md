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

---

### 5.7 Server-Timing 통합 (ADR-026 연동)

> **목표**: TrafficBot이 부하를 발생시키는 동시에, ADR-026의 W3C Server-Timing 인프라를 활용하여
> 각 마이크로서비스(Gateway / C++ Preprocess / Python AI) 구간별 평균 응답시간을 한 번에 리포트한다.

#### 배경

ADR-026에 따라 Node.js·C++·Python 3개 서버 모두 `Server-Timing` 헤더를 이미 구현하고 있다.
그러나 기존 TrafficBot(5.3)은 응답 헤더를 수집하지 않아 "몇 건 처리됐나?"만 알 수 있고
"어느 서버가 병목인가?"는 알 수 없다. 이 섹션은 그 공백을 채운다.

#### 구조

```
TrafficBot (--profile-key 옵션 활성화 시)
    │  X-Admin-Profile-Key 헤더 포함 요청 발사
    ▼
API Gateway  →  C++ Preprocess  →  Python AI
    │            server-timing       server-timing
    │            헤더 릴레이          헤더 릴레이
    ▼
Server-Timing: gateway;dur=12.3, preprocess;dur=18.7, ai_inference;dur=8.5
    │
    ▼
TrafficBot 집계: N건 평균값 출력
```

#### 변경 파일 (3개)

| 파일 | 변경 내용 |
|------|---------|
| `api-gateway/tests/trafficBot.test.ts` | 프로파일 모드 테스트 2종 추가 |
| `api-gateway/src/tools/trafficBot.ts` | 인터페이스 확장 + 헤더 수집·집계 구현 |
| `api-gateway/src/tools/runTrafficBot.ts` | `--profile-key` CLI 옵션 + 타이밍 출력 추가 |

#### 인터페이스 변경

**TrafficBotConfig** 추가 필드:
```typescript
profileKey?: string;  // X-Admin-Profile-Key 값. 설정 시 Server-Timing 수집 활성화
```

**TrafficBotResult** 추가 필드:
```typescript
avgTimings?: {
  gateway?: number;       // API Gateway 처리 시간 평균 (ms)
  preprocess?: number;    // C++ 전처리 서버 처리 시간 평균 (ms)
  aiInference?: number;   // Python AI 추론 시간 평균 (ms)
};
```

#### CLI 실행 예시

```bash
# 환경변수 설정 (서버와 동일한 키 사용)
export ADMIN_PROFILE_KEY=my-secret-key

# 프로파일 모드로 TrafficBot 실행
cd api-gateway
npx ts-node src/tools/runTrafficBot.ts \
  --requests 10 \
  --interval 7000 \
  --profile-key my-secret-key
```

> **주의**: rate limit(10 req/min) 때문에 `--interval 7000` 이상 권장. `--profile-key`를 생략하면 기존 모드(타이밍 미수집)로 동작한다.

#### 기대 출력

서버 3개가 모두 실행 중이고 `--profile-key`가 올바르게 설정된 경우:

```
TrafficBot 시작
  대상: http://localhost:3000/analyze
  총 요청: 10
  동시 요청: 1
  요청 간격: 7000ms

[TrafficBot] 완료: 10/10 성공, avgResponseMs=45ms, P95=68ms
서비스별 평균 응답시간 (Server-Timing):
  gateway:      12.3ms
  preprocess:   18.7ms
  ai_inference: 8.5ms
```

`--profile-key` 없이 실행한 경우 기존 출력만 표시되고 타이밍 테이블은 생략된다.

#### TDD 테스트 전략

| 테스트 | 검증 내용 |
|--------|---------|
| `should_collect_server_timing_when_profileKey_is_set` | nock으로 `Server-Timing` 헤더 포함 응답 모킹 → `result.avgTimings` 값 검증 |
| `should_not_collect_server_timing_when_profileKey_is_not_set` | profileKey 없음 → `result.avgTimings === undefined` 검증 |

#### 커밋 전략

기능적 변경만이므로 단일 커밋:
```
feat(api-gateway): integrate Server-Timing collection into TrafficBot
```

---

## 6. E2E 스모크 테스트 실행 (`scripts/e2e_smoke_test.ps1`)

### 6.1 개요

`scripts/e2e_smoke_test.ps1`은 서버 3개 기동 → TrafficBot N발 분산사격 → 결과 출력 → 클린 종료를
한 번에 수행하는 E2E 스모크 테스트 스크립트다.

```powershell
# 기본 실행 (10발, 7초 간격, 동시 1)
.\scripts\e2e_smoke_test.ps1

# 파라미터 지정
.\scripts\e2e_smoke_test.ps1 -Requests 5 -IntervalMs 7000 -Concurrency 1

# Server-Timing 프로파일링 모드 (ADMIN_PROFILE_KEY 환경변수 필요)
$env:ADMIN_PROFILE_KEY = "my-secret-key"
.\scripts\e2e_smoke_test.ps1 -ProfileMode -Requests 5
```

| 파라미터 | 기본값 | 설명 |
|----------|--------|------|
| `-Requests` | `10` | TrafficBot이 전송할 총 요청 수 |
| `-IntervalMs` | `7000` | 요청 간격(ms). rate limit(10 req/min) 때문에 6000 이상 권장 |
| `-Concurrency` | `1` | 동시 요청 수 |
| `-ProfileMode` | off | `ADMIN_PROFILE_KEY` 환경변수와 함께 사용 시 Server-Timing 수집 |

---

### 6.2 트러블슈팅 기록

실제 실행 중 발생한 문제와 해결법을 기록한다.

---

#### 문제 1: `WorkingDirectory has a value that is not valid`

**시나리오**: `cd scripts && .\e2e_smoke_test.ps1` 으로 실행했을 때 발생.

```
Start-Process: WorkingDirectory has a value that is not valid.
```

**원인**: 스크립트 내부에서 `$PROJECT_ROOT = (Get-Item .).FullName`을 사용했기 때문.
CWD가 `scripts/`이면 `preprocess-server/build/bin/preprocess_server.exe` 경로가 `scripts/preprocess-server/...`로 잘못 조립된다.

**해결**: `$PSScriptRoot`(스크립트 자신의 디렉토리)를 기준으로 루트를 계산.

```powershell
# 수정 전 (CWD 기준 — 실행 위치에 따라 달라짐)
$PROJECT_ROOT = (Get-Item .).FullName

# 수정 후 (스크립트 위치 기준 — 항상 안정적)
$PROJECT_ROOT = Split-Path -Parent $PSScriptRoot
```

---

#### 문제 2: `Cannot find module './analyze'` — npx 경유 시 인자 파싱 오류

**시나리오**: PowerShell에서 `& npx ts-node $botArgs`로 호출했을 때 발생.

```
node:internal/modules/cjs/loader:1404
  throw err;
Error: Cannot find module './analyze'
Require stack:
- C:\...\runTrafficBot.ts --requests 10 --interval 7000 ... --endpoint \imaginaryUncacheableRequireResolveScript
```

**원인**: `npx`가 PowerShell 배열 `$botArgs`를 단일 문자열로 합쳐서 전달.
결과적으로 `ts-node "runTrafficBot.ts --requests 10 ..."` 처럼 모든 인자가 파일명에 포함된다.

**해결**: `npx` 경유를 제거하고 `.cmd` 파일을 직접 호출.

```powershell
# 수정 전 — PowerShell 배열이 npx 내부에서 단일 문자열로 flatten됨
& npx ts-node $botArgs

# 수정 후 — .cmd 직접 호출 시 PowerShell 배열이 개별 인자로 정상 전달됨
$tsNode = ".\node_modules\.bin\ts-node.cmd"
& $tsNode $botArgs
```

---

#### 문제 3: `Premature end of JPEG file` — OpenCV가 더미 이미지 거부

**시나리오**: TrafficBot이 10발을 쐈는데 전부 HTTP 500으로 실패.

```
[TrafficBot] 완료: 0/10 성공, avgResponseMs=214ms, P95=841ms
에러 분포:
  HTTP 500: 10회
```

로그를 보면 C++ 전처리 서버에서 이미지 로드 자체가 실패:

```
Premature end of JPEG file
Failed to load image: SYSTEM
L6 Sanitization skipped: preprocessing failed, using original image
INFO: "POST /analyze HTTP/1.1" 400 Bad Request
Analysis Error: {"error":"Request failed with status code 400"}
POST /analyze 500
```

**원인**: `createDummyJpeg()`이 반환하던 70바이트 버퍼는 JPEG 매직 바이트(`FF D8 FF E0`)만 있고
DQT/SOF0/DHT/SOS 마커가 없는 불완전한 파일이었다.
OpenCV `cv::imread`는 완전한 디코딩 가능한 JPEG만 허용한다.

**해결**: `trafficBot.ts`의 `createDummyJpeg()`를 표준 Annex K Huffman 테이블을 포함한
완전한 JFIF baseline JPEG(1×1 픽셀, 그레이스케일, ~331바이트)로 교체.

```
구조: SOI → APP0(JFIF) → DQT(양자화 테이블) → SOF0(1×1 그레이스케일)
      → DHT DC(Annex K) → DHT AC(Annex K) → SOS → 스캔데이터(0x2B) → EOI

스캔데이터 계산:
  DC coefficient = 0 → DC category 0 → Huffman code: 00 (2비트)
  AC = EOB           → Huffman code: 1010 (4비트)
  합계 6비트 → 패딩 후 1바이트: 00101011 = 0x2B
```

---

#### 문제 4: PowerShell 출력 인코딩 깨짐

**시나리오**: 스크립트가 성공적으로 실행됐지만 한글 출력이 깨져서 표시됨.

```
[00:27:46] TrafficBot ?쒖옉
[00:27:46]   ??? http://127.0.0.1:3000/analyze
[00:27:46] [TrafficBot] ?꾨즺: 10/10 ?깃났, avgResponseMs=214ms, P95=841ms
```

**원인**: PowerShell이 외부 프로세스 출력을 캡처할 때 `[Console]::OutputEncoding`(기본값: 시스템 코드 페이지 CP949)으로
디코딩한다. ts-node는 UTF-8로 출력하므로 인코딩 불일치가 발생한다.

**해결**: ts-node 호출 직전에 콘솔 인코딩을 UTF-8로 변경하고 호출 후 복원.

```powershell
$prevEncoding = [Console]::OutputEncoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$botOutput = & $tsNode $botArgs 2>&1
[Console]::OutputEncoding = $prevEncoding
$botOutput | ForEach-Object { Write-LogInfo $_ }
$botOutput | Out-File $LOG_FILE -Append -Encoding utf8
```
