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
