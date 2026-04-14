# Phase 5: 통합 및 고도화 (배포 전략) 계획안

## Context

Phase 1~4에서 4개 마이크로서비스(API Gateway, C++ Preprocess Server, Python AI Server, React Frontend)의 코드 구현과 테스트를 완료했다. 118/118 테스트 통과, ONNX/TensorRT 최적화, 구조화 로깅, X-Request-ID 추적 등 생산 준비 코드는 갖춰졌으나 배포 인프라(Docker, Nginx, HTTPS 설정)가 전무하다.

ADR-027에서 EC2 인스턴스 타입을 **c5.large (vCPU 2 고정, RAM 4GB, ~$62/월)** 로 결정(Accepted)했고, 전제 조건인 ONNX Runtime 전환은 Phase 4에서 완료되었다. Phase 5는 Docker 컨테이너화 → Nginx/HTTPS 설정 → EC2 c5.large 실배포 → 성능·부하 검증까지 포함한 전체 배포 파이프라인을 구축하는 것이 목표다.

---

## 작업 범위 (5개 단계)

### Step 1: Docker 컨테이너화 (Dockerfiles + docker-compose)

**목표**: 각 서비스를 독립 컨테이너로 패키징하고 로컬에서 `docker compose up`으로 전체 스택 실행 검증

#### 1-1. 각 서비스 Dockerfile 작성

| 서비스 | 파일 위치 | 베이스 이미지 | 전략 |
|--------|-----------|--------------|------|
| preprocess-server | `preprocess-server/Dockerfile` | `ubuntu:22.04` | Multi-stage: CMake 빌드 → 런타임 복사 |
| api-gateway | `api-gateway/Dockerfile` | `node:20-alpine` | Multi-stage: npm ci → tsc 빌드 → 최소 런타임 |
| ai-server | `ai-server/Dockerfile` | `python:3.10-slim` | pip install + ONNX 모델 파일 복사 |
| frontend | `frontend/Dockerfile` | `node:20-alpine` → `nginx:alpine` | Vite build → Nginx 정적 서빙 |

**C++ 빌드 주의사항**: Docker 내에서 Linux용 CMake 빌드 수행. vcpkg `x64-linux` triplet 사용 (Windows `x64-windows` 아님). `PREPROCESS_WORKERS` 환경변수로 ADR-028 WorkerPool 튜닝 가능.

**AI 서버 주의사항**: `ai-server/models/*.onnx` 파일을 이미지에 포함하거나 Docker 볼륨으로 마운트 (파일 크기 고려해 결정).

#### 1-2. docker-compose.yml 작성 (프로젝트 루트)

```yaml
서비스 구성:
  nginx:             포트 443:443, 80:80 (외부 진입점)
  api-gateway:       내부 3000 (오케스트레이터)
  preprocess-server: 내부 8081 (C++ 이미지 처리)
  ai-server:         내부 8082 (Python ONNX 추론)

볼륨:
  shared_volume: api-gateway, preprocess-server, ai-server 공유

네트워크:
  external-net: nginx ↔ api-gateway
  internal-net: api-gateway ↔ preprocess-server, ai-server (외부 미노출)

헬스체크 체인:
  preprocess-server: curl /health → healthy
  ai-server:         curl /health → healthy
  api-gateway:       curl /health → healthy (depends_on: preprocess + ai)
  nginx:             depends_on: api-gateway healthy

재시작 정책: restart: unless-stopped
```

**로컬 개발용**: `docker-compose.override.yml` (포트 노출, 볼륨 바인드 마운트)

---

### Step 2: Nginx 리버스 프록시 설정

**목표**: 외부 HTTPS(443) → 내부 HTTP 라우팅, Mixed Content 방지

#### 2-1. nginx.conf 작성 (`nginx/nginx.conf`)

```nginx
라우팅 규칙:
  /api/* → api-gateway:3000 (proxy_pass)
  /      → frontend 정적 파일 (root /usr/share/nginx/html)

보안:
  HTTP → HTTPS 리다이렉트 (301)
  SSL/TLS 종단 처리 (인증서: /etc/nginx/ssl/)
  HSTS 헤더

성능:
  gzip 압축 활성화
  client_max_body_size 10m (이미지 업로드 대응)
  proxy_read_timeout 60s (AI 추론 지연 대응)
```

#### 2-2. nginx/Dockerfile 작성

```dockerfile
FROM nginx:alpine
COPY nginx.conf /etc/nginx/nginx.conf
# 인증서는 볼륨으로 마운트 (이미지에 포함 안 함)
```

---

### Step 3: EC2 c5.large 배포

**목표**: AWS EC2 c5.large에서 전체 스택 운영 (ADR-027 참고)

#### 3-1. EC2 인스턴스 프로비저닝

```
인스턴스 타입: c5.large (ADR-027)
  - vCPU: 2 (고정, 버스터블 아님)
  - RAM: 4GB
  - OS: Ubuntu 22.04 LTS
  - 스토리지: 20GB gp3 (OS + Docker 이미지 + 모델 파일)

보안 그룹 (Inbound):
  - 22  (SSH)   → 개인 IP만 허용
  - 80  (HTTP)  → 0.0.0.0/0
  - 443 (HTTPS) → 0.0.0.0/0
```

#### 3-2. EC2 서버 초기 설정 (수동 1회)

```bash
# Docker 설치
sudo apt update && sudo apt install -y docker.io docker-compose-plugin
sudo usermod -aG docker ubuntu

# 프로젝트 클론
git clone https://github.com/<org>/mind-palette-project.git
cd mind-palette-project
```

#### 3-3. 환경변수 설정

**파일**: `.env` (EC2 서버에서 직접 작성, git에 올리지 않음)

```env
NODE_ENV=production
PREPROCESS_SERVER_URL=http://preprocess-server:8081
AI_SERVER_URL=http://ai-server:8082
KEEP_IMAGES=false
CACHE_TTL_SECONDS=3600
CACHE_MAX_SIZE=100
PREPROCESS_WORKERS=2        # c5.large vCPU 2 기준 (ADR-028)
```

#### 3-4. Let's Encrypt SSL 인증서 발급

```bash
# Certbot 설치 및 발급 (--standalone, 80 포트 일시 사용)
sudo apt install -y certbot
sudo certbot certonly --standalone -d <도메인>

# 인증서를 Docker 볼륨 위치로 복사
sudo cp /etc/letsencrypt/live/<도메인>/fullchain.pem nginx/ssl/
sudo cp /etc/letsencrypt/live/<도메인>/privkey.pem nginx/ssl/

# 자동 갱신 (cron, 월 1회)
echo "0 0 1 * * root certbot renew --quiet && docker compose restart nginx" \
  | sudo tee /etc/cron.d/certbot-renew
```

#### 3-5. 배포 실행

```bash
# 첫 배포
docker compose up -d --build

# 이후 배포 (코드 업데이트 시)
git pull && docker compose up -d --build --no-deps <서비스명>
```

#### 3-6. 배포 후 스모크 테스트

```bash
curl https://<도메인>/api/health | jq .
```

---

### Step 4: 해시 기반 캐싱 구현 (API Gateway, TDD)

**목표**: 동일 이미지 재업로드 시 < 10ms 캐시 히트 응답

#### 4-1. 캐시 레이어 구현

**신규 파일**: `api-gateway/src/services/cacheService.ts`

```typescript
// 구현 전략
- 이미지 업로드 시 SHA-256 해시 계산
- Map<hash, {result, timestamp}> 인메모리 LRU 캐시 (최대 100건)
- TTL: CACHE_TTL_SECONDS 환경변수 (기본 3600초)
- 캐시 히트: 전처리/추론 건너뛰고 저장 결과 즉시 반환
```

**재사용할 기존 유틸리티**:
- `api-gateway/src/utils/hashIntegrity.ts` — SHA-256 파일 해시 계산 로직
- `api-gateway/src/services/analysisService.ts` — 캐시 조회/저장 삽입 위치

**TDD 순서**:
1. RED: `api-gateway/tests/cacheService.test.ts` (캐시 히트/미스/만료/LRU 케이스)
2. GREEN: `cacheService.ts` 구현
3. REFACTOR: `analysisService.ts`에 캐시 통합

---

### Step 5: 부하 테스트 (k6 + Traffic Bot)

**목표**: c5.large 기준 100 동시 사용자 P95 레이턴시 검증

#### 5-1. Node.js 트래픽 봇

**신규 파일**: `scripts/traffic-bot.ts`

```typescript
// Axios 기반 트래픽 생성기
- 테스트 이미지 디렉토리에서 랜덤 이미지 선택
- 동시 요청 수(concurrency), 총 요청 수 설정 가능
- 응답 시간, 성공률, P50/P95 통계 출력
```

#### 5-2. k6 시나리오

**신규 파일**: `scripts/load-test.js`

```javascript
시나리오 1 (Smoke):  1 VU,   1분  — 기본 동작 확인
시나리오 2 (Load):   100 VU, 5분  — c5.large 정상 부하
시나리오 3 (Stress): 200 VU, 10분 — 한계 탐색 (점진적 증가)

임계값:
  http_req_duration p(95) < 500ms  (캐시 미스, 추론 포함)
  http_req_duration p(95) < 10ms   (캐시 히트 시나리오)
  http_req_failed rate < 0.01      (실패율 1% 미만)
```

---

## 실행 순서

```
Step 1: Docker 컨테이너화
  ├── preprocess-server/Dockerfile
  ├── api-gateway/Dockerfile
  ├── ai-server/Dockerfile
  ├── frontend/Dockerfile
  ├── docker-compose.yml
  └── 검증: docker compose up --build (로컬)

Step 2: Nginx 설정
  ├── nginx/nginx.conf
  ├── nginx/Dockerfile
  ├── docker-compose.yml에 nginx 서비스 추가
  └── 검증: localhost 라우팅, HTTPS self-signed 동작

Step 3: EC2 c5.large 배포
  ├── 인스턴스 프로비저닝 (c5.large, Ubuntu 22.04)
  ├── 보안 그룹 설정 (22, 80, 443)
  ├── Docker 설치 + 프로젝트 클론
  ├── .env 설정 (PREPROCESS_WORKERS=2)
  ├── Let's Encrypt 인증서 발급
  ├── docker compose up -d --build
  └── 검증: curl https://<도메인>/api/health | jq .

Step 4: 해시 캐싱 (TDD)
  ├── RED: cacheService.test.ts
  ├── GREEN: cacheService.ts
  ├── REFACTOR: analysisService.ts 통합
  └── 검증: 동일 이미지 2회 → 2번째 < 10ms

Step 5: 부하 테스트
  ├── scripts/traffic-bot.ts
  ├── scripts/load-test.js (k6)
  └── 검증: 100 VU, P95 < 500ms (EC2에서 실행)
```

---

## 수정/생성할 파일 목록

### 신규 생성
| 파일 | 단계 |
|------|------|
| `preprocess-server/Dockerfile` | Step 1 |
| `api-gateway/Dockerfile` | Step 1 |
| `ai-server/Dockerfile` | Step 1 |
| `frontend/Dockerfile` | Step 1 |
| `docker-compose.yml` | Step 1 |
| `docker-compose.override.yml` | Step 1 (로컬 개발용) |
| `nginx/nginx.conf` | Step 2 |
| `nginx/Dockerfile` | Step 2 |
| `api-gateway/src/services/cacheService.ts` | Step 4 |
| `api-gateway/tests/cacheService.test.ts` | Step 4 |
| `scripts/traffic-bot.ts` | Step 5 |
| `scripts/load-test.js` | Step 5 |

### 수정 대상
| 파일 | 수정 내용 |
|------|----------|
| `api-gateway/src/services/analysisService.ts` | 캐시 조회/저장 로직 삽입 |
| `.env.example` | CACHE_TTL_SECONDS, CACHE_MAX_SIZE, PREPROCESS_WORKERS 추가 |
| `plan.md` | Step별 완료 체크박스 업데이트 |

### EC2 서버에만 존재 (git 비포함)
| 파일 | 내용 |
|------|------|
| `.env` | 프로덕션 환경변수 |
| `nginx/ssl/fullchain.pem` | Let's Encrypt 인증서 |
| `nginx/ssl/privkey.pem` | 개인키 |

---

## 검증 방법

### 1. 로컬 Docker 스택
```bash
docker compose up --build
curl http://localhost/api/health
```

### 2. EC2 배포 후 스모크 테스트
```bash
curl https://<도메인>/api/health | jq .
```

### 3. 캐시 성능 (EC2)
```bash
# 캐시 미스 (첫 요청)
time curl -F "image=@test.jpg" -F 'childInfo={}' https://<도메인>/api/analyze
# 캐시 히트 (동일 이미지 재요청, < 10ms 목표)
time curl -F "image=@test.jpg" -F 'childInfo={}' https://<도메인>/api/analyze
```

### 4. Jest 테스트 (캐싱)
```bash
cd api-gateway && npm test -- --testPathPattern=cacheService
```

### 5. k6 부하 테스트 (EC2)
```bash
k6 run scripts/load-test.js
# 확인: p(95) < 500ms, 실패율 < 1%
```

---

## 제약사항 및 주의사항

| 항목 | 내용 |
|------|------|
| C++ Docker 빌드 | vcpkg `x64-linux` triplet 사용 (Windows `x64-windows` 아님) |
| ONNX 모델 크기 | `ai-server/models/*.onnx` 크기 확인 후 이미지 포함 or 볼륨 결정 |
| EC2 인스턴스 타입 | c5.large (ADR-027) — t3.medium은 버스터블 CPU로 부적합 |
| PREPROCESS_WORKERS | `2` 설정 (c5.large vCPU 2, ADR-028) |
| SSL 인증서 | 로컬: self-signed, EC2: Let's Encrypt + 월 1회 자동 갱신 |
| 비용 | c5.large 약 $62/월 (온디맨드 기준) |
