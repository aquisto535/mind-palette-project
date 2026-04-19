# Mind Palette

**아동 인물화(HFD) 기반 지능 측정 AI 시스템**

아동이 그린 인물화(Human Figure Drawing) 이미지를 AI로 분석하여 발달 지능(IQ)과 백분위를 산출하는 마이크로서비스 시스템입니다.

---

## 목차

- [프로젝트 개요](#프로젝트-개요)
- [시스템 아키텍처](#시스템-아키텍처)
- [서비스 구성](#서비스-구성)
- [데이터 흐름](#데이터-흐름)
- [API 명세](#api-명세)
- [시작하기](#시작하기)
- [테스트](#테스트)
- [개발 현황](#개발-현황)

---

## 프로젝트 개요

HFD(Human Figure Drawing) 검사는 아동의 손 그림을 통해 발달 지능을 측정하는 심리 검사입니다. Mind Palette는 이 검사를 자동화하여, 이미지를 업로드하면 AI가 60개 세부 문항을 채점하고 연령별 표준화된 IQ 점수와 백분위를 반환합니다.

**핵심 특징**

- 고성능 C++ 이미지 전처리 → ONNX/TensorRT 가속 AI 추론 파이프라인
- 3채널 하이브리드 입력 (Grayscale + Binary + Distance Transform)
- 연령·성별별 표준화 IQ/백분위 산출 (EfficientNet-B2 + 4-Head 분류)
- 2-Tier Fail-Fast 입력 검증 (C++ HSV 채도 분석 + Python Confidence Score)
- SHA-256 해시 기반 결과 캐싱 (동일 이미지 재요청 시 30~40ms 응답)
- Docker Compose + Nginx SSL 종단 기반 프로덕션 배포 (AWS EC2 c5.large)
- TDD 기반 개발, 통합 테스트 206개 (C++ 59 + Python 147)

---

## 시스템 아키텍처

```
               ┌──────────────────────────────────┐
               │  Nginx Reverse Proxy (443/HTTPS) │
               │  · Let's Encrypt SSL 종단        │
               │  · HTTP → HTTPS 리다이렉트       │
               └──────────────┬───────────────────┘
                              │ (Public TLS)
┌─────────────────────────────▼───────────────────────┐
│              React Frontend  (Port 5173 / Netlify)   │
│   Hero → InfoForm → Guide → Upload → Result / Error │
└────────────────────┬────────────────────────────────┘
                     │ POST multipart/form-data
                     ▼
┌─────────────────────────────────────────────────────┐
│           Node.js API Gateway  (Port 3000)           │
│   Rate Limit · Magic Byte 검증 · SHA-256 Cache      │
│   · Path Traversal 차단 · Orchestration             │
└────────┬───────────────────────────┬────────────────┘
         │ POST /preprocess           │ POST /analyze
         ▼ (internal-net HTTP)        ▼ (internal-net HTTP)
┌─────────────────┐       ┌──────────────────────────┐
│  C++ Preprocess │       │    Python AI Server       │
│  Server         │       │    (Port 8082)            │
│  (Port 8081)    │       │                          │
│  ∙ Color Valid. │       │  EfficientNet-B2          │
│  ∙ Resize       │       │  + 4 Multi-head (60문항)  │
│  ∙ Hybrid 3ch   │       │  + ONNX / TensorRT        │
│  ∙ Pressure     │       │  + Confidence Guard       │
│  ∙ Tremor       │       │  → IQ / Percentile        │
│  ∙ ...11 filters│       │                          │
└─────────────────┘       └──────────────────────────┘
```

**포트 맵**

| 서비스 | 포트 | 언어/프레임워크 |
| --- | --- | --- |
| Nginx (SSL 종단) | 80, 443 | Nginx + Let's Encrypt |
| Frontend | 5173 (로컬) / Netlify (프로덕션) | React 18 + Vite |
| API Gateway | 3000 (내부망) | Node.js / Express |
| Preprocess Server | 8081 (내부망) | C++17 / Crow |
| AI Server | 8082 (내부망) | Python / FastAPI |

> **배포 토폴로지**: 외부 트래픽은 Nginx(HTTPS)만 수신하고, 백엔드 서비스는 Docker `internal-net` 격리망에서 HTTP로 통신하여 SSL 오버헤드를 제거합니다.

---

## 서비스 구성

### 1. Preprocess Server (`preprocess-server/`)

C++17로 구현한 고성능 이미지 전처리 서버입니다.

**기술 스택**

| 항목 | 내용 |
|------|------|
| 언어 | C++17 |
| 빌드 | CMake 3.15+, vcpkg (`x64-windows`) |
| REST | Crow |
| 이미지 | OpenCV 4.x |
| 로깅 | spdlog |
| 테스트 | Google Test |

**구현된 필터 (Strategy Pattern)**

| 필터 | 역할 |
| --- | --- |
| `color_validation_filter` | **ADR-034** HSV 채도 분석으로 컬러 이미지 조기 차단 (HTTP 422) |
| `resize_filter` | 512×512 Letterbox 리사이징 (INTER_AREA/INTER_CUBIC 자동 선택) |
| `grayscale_filter` | RGB → Grayscale |
| `binarize_filter` | Adaptive Threshold (blockSize=7, C=3) |
| `morphology_filter` | 침식/팽창 (선 연결성 강화) |
| `denoise_filter` | 가우시안 필터 (5×5) |
| `nlmeans_denoise_filter` | NL-Means 엣지 보존 노이즈 제거 (h=5) |
| `clahe_filter` | CLAHE 명암비 향상 (clipLimit=1.0) |
| `otsu_canny_filter` | Otsu 기반 자동 Threshold Canny 엣지 감지 (sigma=0.5) |
| `invert_filter` | 이미지 반전 (White Background 보정) |
| `rgb_convert_filter` | RGB 채널 변환 |
| `hybrid_preprocess_filter` | 3채널 종합 전처리 출력 (R=Gray, G=InvBinary, B=Distance) |

**분석 모듈 (AI 보조 기하 특징)**

| 모듈 | 엔드포인트 | 역할 |
| --- | --- | --- |
| `pressure_analyzer` | `POST /analyze-pressure` | Grayscale 픽셀 분포 히스토그램 기반 필압 점수 |
| `tremor_analyzer` | `POST /analyze-tremor` | Hu Moments 기반 선 떨림(Tremor) 스코어 |

**인프라**

- `ThreadPool`: CPU 바운드 작업용 워커 스레드 풀 (기본 4개, `PREPROCESS_WORKERS` 환경변수로 조정 — 프로덕션 EC2 c5.large: 2)
- `AtomicWriter`: `.tmp` → `rename` 패턴으로 저장/삭제 원자성 보장 (동시 요청 시 파일 쓰기 경합 방지)
- `FilterPipeline`: Composite 패턴으로 필터 체인 구성, OCP 준수 (신규 필터 무수정 확장)
- **Early Resize 최적화**: 이진화·컨투어 추출 전 768px로 선제 축소 (Latency 183ms → 97ms)

---

### 2. AI Server (`ai-server/`)

EfficientNet-B2 기반 HFD 분류 추론 서버입니다.

**기술 스택**

| 항목 | 내용 |
|------|------|
| 언어 | Python 3.x |
| 프레임워크 | FastAPI |
| 딥러닝 | PyTorch, torchvision |
| 추론 최적화 | ONNX Runtime, TensorRT |
| 이미지 | OpenCV |
| 로깅 | structlog |
| 테스트 | pytest |

**모델 아키텍처 (ADR-019)**

```
Input: (1, 3, 260, 260)   ← 3채널 이미지
  ↓
EfficientNet-B2 (Frozen Backbone)
  ↓ Feature Vector: (1, 1408)
  ↓
┌──────────────────────────────────────────┐
│  Head A: 19문항  (머리/얼굴)             │
│  Head B: 14문항  (몸통/연결/비례)        │
│  Head C: 16문항  (사지/말단)             │
│  Head D: 11문항  (의복/질적)             │
└──────────────────────────────────────────┘
  Total: 60문항 → 원점수 → IQ/백분위 변환
```

**3채널 입력 의미**

| 채널 | 내용 | 역할 |
|------|------|------|
| R (CH0) | Grayscale | 명도 정보 |
| G (CH1) | Binary | 구조 정보 |
| B (CH2) | Distance Transform | 거리/두께 정보 |

**추론 백엔드 (자동 폴백)**

```
TensorRT Native (~10ms) → ONNX Runtime (~20ms) → PyTorch (~40ms)
```

---

### 3. API Gateway (`api-gateway/`)

마이크로서비스 오케스트레이터이자 클라이언트 진입점입니다.

**기술 스택**

| 항목 | 내용 |
|------|------|
| 언어 | TypeScript |
| 프레임워크 | Express.js |
| 파일 업로드 | Multer |
| HTTP 클라이언트 | Axios |
| 로깅 | Winston + Morgan |
| 테스트 | Jest + Supertest |

**처리 파이프라인**

```
1. 클라이언트 요청 수신 (X-Request-ID 생성/전파)
2. Rate Limiting (전역 15분/100회, /analyze 1분/10회)
3. 매직 바이트 검증 (JPEG/PNG/BMP/WebP) + 경로 트래버설 차단
4. SHA-256 해시 산출 → Cache Hit 시 즉시 반환 (~30ms)
5. shared_volume/uploads/ 에 파일 저장
6. → Preprocess Server 호출 (C++) — ValidationException(422) 전파 지원
7. → AI Server 호출 (Python) + processed 이미지 전달
8. AiServerResponse → AnalysisResult 매핑
9. SHA-256 무결성 해시 포함 결과 저장 (results/)
10. 임시 파일 정리 (KEEP_IMAGES=false 시 uploads/processed 자동 삭제)
11. 클라이언트 응답 반환 (Server-Timing 헤더 포함)
```

**보안**

- Rate Limiting: 전역 (15분/100회), `/analyze` (1분/10회)
- 매직 바이트 기반 파일 형식 검증 (6-Layer Defense)
- 경로 트래버설 방지 (CodeQL 대응, null byte/절대경로/`..` 차단)
- 로그 PII 마스킹 (파일 경로·이름·생년월일 숨김)
- SHA-256 결과 무결성 검증 + Atomic Delete

**성능/관측성**

- **Hash-based Caching (ADR-032)**: LRU + TTL 기반 결과 캐시, 동일 이미지 재요청 시 30~40ms 응답
- **Winston 구조화 로깅**: JSON 포맷, DailyRotateFile (10MB, 7일 보관, gzip)
- **Server-Timing 헤더**: `gateway`, `preprocess`, `ai_inference` 구간별 지연시간 계측
- **OpenAPI 3.0 스펙**: `/api-docs` 경로에서 Swagger UI 제공

---

### 4. Frontend (`frontend/`)

아동 정보 입력 → 이미지 업로드 → 결과 시각화 흐름의 React 앱입니다.

**기술 스택**

| 항목 | 내용 |
|------|------|
| 프레임워크 | React 18 + TypeScript |
| 빌드 | Vite |
| 스타일 | Tailwind CSS |
| 애니메이션 | Framer Motion |
| PDF | html2canvas + jsPDF |
| 테스트 | Vitest |

**7단계 사용자 흐름**

```
Hero → InfoForm → Guide → Upload → Loading → Result
                                          └──→ Error (422/429/503)
```

| 단계 | 내용 |
| --- | --- |
| Hero | 시작 화면 |
| InfoForm | 자녀 이름·성별·생년월일 입력 (인라인 유효성 검사) |
| Guide | 인물화 그리기 안내 |
| Upload | 이미지 드래그&드롭 업로드 (10MB 제한) |
| Loading | AI 분석 대기 |
| Result | IQ·백분위·발달 트리 시각화, PDF 저장 |
| Error | 422(컬러/저신뢰도) / 429(Rate Limit) / 503 전용 안내 + 재시도 |

**입력 유효성 검사**

- 이름: 2~20자, 한글/영문/공백만 허용
- 생년월일: 만 5~13세 범위, `min`/`max` picker 제한
- 이미지: 10MB 이하, JPEG/PNG/BMP/WebP
- `AnalysisError` 클래스 + Axios 인터셉터로 상태 코드별 분기 처리

---

## 데이터 흐름

### 전체 시퀀스 다이어그램

```mermaid
sequenceDiagram
    autonumber
    participant U as 🖥️ React Frontend<br/>(Port 5173)
    participant G as 🔀 API Gateway<br/>(Port 3000)
    participant FS as 📁 shared_volume/
    participant C as ⚡ C++ Preprocess<br/>(Port 8081)
    participant A as 🧠 AI Server<br/>(Port 8082)

    Note over U,A: 📤 Phase 1 — 이미지 업로드 및 검증

    U->>G: POST /analyze<br/>multipart/form-data<br/>{image: File, childInfo: JSON}
    activate G

    Note right of G: 🛡️ 보안 미들웨어 체인<br/>① Rate Limit (1분/10회)<br/>② Multer 파일 크기 검사 (≤5MB)<br/>③ Magic Bytes 검증 (JPEG/PNG/BMP/WebP)<br/>④ 해상도 검사 (≤4096x4096)

    G->>FS: 💾 uploads/{timestamp}.jpg

    Note over G,C: 🔧 Phase 2 — C++ 이미지 전처리

    G->>C: POST /preprocess<br/>{"imagePath": "shared_volume/uploads/xxx.jpg"}
    activate C

    Note right of C: 🧵 ThreadPool 워커 스레드 할당

    C->>FS: 📖 uploads/xxx.jpg 읽기
    FS-->>C: Raw Image (JPEG/PNG)

    Note right of C: 🔬 ImageProcessor 파이프라인<br/>① Resize (260×260 Letterbox)<br/>② Grayscale 정규화<br/>③ Adaptive Binarization (Otsu)<br/>④ Morphology (침식/팽창)<br/>⑤ Distance Transform 생성<br/>⑥ 3채널 합성<br/>   R=Grayscale, G=Binary, B=Distance

    C->>FS: 💾 processed/xxx_clean.jpg<br/>(AtomicWriter: .tmp→rename)
    C-->>G: {"processedPath": "shared_volume/processed/xxx_clean.jpg"}
    deactivate C

    Note over G,A: 🧠 Phase 3 — AI 추론

    G->>FS: 📖 processed/xxx_clean.jpg 읽기
    FS-->>G: Processed 3-Channel Image

    G->>A: POST /analyze<br/>multipart/form-data<br/>{file: 3ch_image, age, child_gender, figure_gender}
    activate A

    Note right of A: 🔄 전처리 파이프라인<br/>① PIL.Image.open → RGB 변환<br/>② val_transform (260×260 리사이즈)<br/>③ Normalize<br/>   mean=(0.972, 0.031, 0.012)<br/>   std=(0.156, 0.174, 0.074)<br/>④ Tensor → numpy (1,3,260,260)

    Note right of A: ⚙️ 추론 엔진 (자동 폴백)<br/>TensorRT → ONNX Runtime → PyTorch

    Note right of A: 🏗️ EfficientNet-B2 추론<br/>Backbone(frozen) → Feature(1,1408)<br/>→ 4 Linear Heads<br/>   Head A: 19문항 (머리/얼굴)<br/>   Head B: 14문항 (몸통/비례)<br/>   Head C: 16문항 (사지/말단)<br/>   Head D: 11문항 (의복/질적)

    Note right of A: 📊 후처리<br/>① Sigmoid → 확률값 변환<br/>② Threshold(0.5) → 0/1 이진 판정<br/>③ 60문항 합산 → raw_score<br/>④ IQ = 100 + 15×(raw−M)/SD<br/>   (연령·성별 규준표 기반)<br/>⑤ IQ → 백분위 매핑

    A-->>G: {"iq": 105, "percentile": 63,<br/>"raw_score": 51, "items": {60문항},<br/>"head_scores": {4개 Head}}
    deactivate A

    Note over G,U: 📦 Phase 4 — 응답 조립 및 정리

    Note right of G: 🔄 응답 매핑<br/>mapAiResponseToResult()<br/>iq → score<br/>head_a/19×100 → creativity<br/>head_b/14×100 → expression<br/>head_c/16×100 → observational

    G->>FS: 💾 results/{timestamp}_result.json<br/>(SHA256 무결성 해시 포함)

    Note right of G: 🧹 임시 파일 정리<br/>uploads/xxx.jpg 삭제<br/>processed/xxx_clean.jpg 삭제

    G-->>U: {"score": 105, "percentile": 63,<br/>"details": {creativity, expression, observational},<br/>"interpretation": "HFD 검사 결과..."}
    deactivate G

    Note over U: 🎨 Result 화면 렌더링<br/>IQ 점수 표시 + 발달 트리 애니메이션<br/>+ PDF 저장 (html2canvas + jsPDF)
```

### 서비스 간 공유 파일 시스템

```mermaid
graph LR
    subgraph shared_volume
        UP[📂 uploads/]
        PR[📂 processed/]
        RS[📂 results/]
    end

    GW[API Gateway] -->|"① 원본 저장"| UP
    CPP[C++ Server] -->|"② 읽기"| UP
    CPP -->|"③ 전처리 결과 저장"| PR
    GW -->|"④ 전처리 이미지 읽기"| PR
    GW -->|"⑤ 분석 결과 저장"| RS
    GW -.->|"⑥ 정리: 임시 파일 삭제"| UP
    GW -.->|"⑥ 정리: 임시 파일 삭제"| PR

    style UP fill:#fef3c7,stroke:#f59e0b
    style PR fill:#dbeafe,stroke:#3b82f6
    style RS fill:#d1fae5,stroke:#10b981
```

---

## API 명세

### API Gateway (Port 3000)

#### `GET /health`

서버 상태를 반환합니다.

```json
{
  "status": "healthy",
  "timestamp": "2026-03-30T12:00:00.000Z",
  "uptime": "45 minutes 30 seconds",
  "memory": { "used": "250.50 MB", "total": "512.00 MB" },
  "disk": { "available": "450.75 GB" }
}
```

#### `POST /analyze`

이미지를 업로드하고 HFD 분석 결과를 반환합니다.

**요청**

```
Content-Type: multipart/form-data
Body: file (JPEG/PNG/BMP/WebP)
Headers:
  X-Request-ID: UUID (선택, 미제공 시 자동 생성)
```

**응답**

```json
{
  "score": 105,
  "percentile": 63,
  "date": "03/30/2026",
  "interpretation": "HFD 검사 결과 IQ 105점 (백분위 63%)",
  "details": {
    "creativity": 79,
    "expression": 86,
    "observational": 88
  }
}
```

**응답 헤더**

```
X-Request-ID: <UUID>
X-Sanitization-Status: applied | skipped
Server-Timing: gateway;dur=250.5, preprocess;dur=45.2, ai_inference;dur=19.8
```

---

### Preprocess Server (Port 8081)

#### `POST /preprocess`

```json
// 요청
{ "imagePath": "/shared_volume/uploads/image.jpg" }

// 응답
{ "processedPath": "/shared_volume/processed/image_clean.jpg" }
```

---

### AI Server (Port 8082)

#### `GET /health`

```json
{
  "status": "ok",
  "uptime_seconds": 3600,
  "system": {
    "cpu_usage_percent": 15.2,
    "memory_usage_bytes": 524288000,
    "memory_usage_mb": 500.0
  },
  "models": { "male": true, "female": true },
  "engine_type": "onnx"
}
```

#### `POST /analyze`

```
Content-Type: multipart/form-data
Body:
  file          이미지 파일
  age           아동 나이 (기본 10)
  child_gender  male | female
  figure_gender male | female
```

```json
{
  "items": { "1": 1, "2": 0, "...": "...", "60": 1 },
  "head_scores": {
    "head_a": 15, "head_b": 12, "head_c": 14, "head_d": 10
  },
  "raw_score": 51,
  "iq": 105,
  "percentile": 63,
  "child_info": { "age": 10, "child_gender": "male", "figure_gender": "male" },
  "date": "2026-03-30"
}
```

---

## 시작하기

### 사전 요구사항

| 항목 | 버전 |
| --- | --- |
| Node.js | 18+ |
| Python | 3.10+ |
| CMake | 3.15+ |
| vcpkg | 최신 (Windows: `x64-windows` triplet 필수) |
| CUDA | 12.x (TensorRT 사용 시, 선택) |
| Docker | 24+ & Compose v2 (통합 실행 시) |

### Quick Start — Docker Compose (권장)

전체 스택을 한 번에 띄우려면:

```bash
# 로컬 개발 모드 (override 자동 적용, 포트 노출)
docker compose up --build

# 프로덕션 모드 (internal-net 격리, Nginx SSL 종단)
docker compose -f docker-compose.yml up -d --build
```

`.env` (루트)의 주요 환경변수:

```bash
PREPROCESS_WORKERS=2
CACHE_TTL_SECONDS=3600
KEEP_IMAGES=false
AI_MODEL_DIR=./ai-server/models
```

> **프로덕션 배포**: AWS EC2 c5.large (Ubuntu 22.04) + Let's Encrypt 인증서. 상세는 [`docs/guides/aws-deployment-guide.md`](docs/guides/aws-deployment-guide.md) 참조.

### 개별 서비스 실행 (개발용)

#### 1. Preprocess Server

```bash
cd preprocess-server
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
./build/Release/preprocess_server
```

> **Windows 주의:** vcpkg triplet은 반드시 `x64-windows` (동적 링크)를 사용하세요.
> `x64-windows-static` 사용 시 CRT Mismatch로 Heap Assertion 오류가 발생합니다.

#### 2. AI Server

```bash
cd ai-server
pip install -r requirements.txt
uvicorn src.main:app --host 0.0.0.0 --port 8082
```

모델 파일을 `models/` 디렉토리에 배치 후 `.env` 설정:

```bash
MALE_MODEL_PATH=models/mind_palette_male.onnx
FEMALE_MODEL_PATH=models/mind_palette_female.onnx
INFERENCE_BACKEND=onnx   # pytorch | onnx | tensorrt_native | tensorrt_ort
DEVICE=cuda              # cuda | cpu
```

#### 3. API Gateway

```bash
cd api-gateway
npm install
cp .env.example .env     # 환경변수 설정
npm run build
npm start
```

`.env` 설정:

```bash
PORT=3000
PREPROCESS_SERVER_URL=http://127.0.0.1:8081
AI_SERVER_URL=http://127.0.0.1:8082
ADMIN_PROFILE_KEY=your_secret_key
KEEP_IMAGES=false
NODE_ENV=production
```

#### 4. Frontend

```bash
cd frontend
npm install
cp .env.local.example .env.local
npm run dev
```

`.env.local` 설정:

```bash
VITE_API_URL=http://localhost:3000
VITE_USE_MOCK=false   # true: Mock 응답 사용 (서버 없이 UI 확인)
```

---

## 테스트

### Preprocess Server (Google Test)

```bash
cd preprocess-server/build
ctest --output-on-failure
```

> Windows 환경에서 DLL 경로 문제가 있을 경우 Visual Studio 테스트 탐색기를 사용하세요.

### AI Server (pytest)

```bash
cd ai-server
pytest tests/ -v
```

| 테스트 파일 | 검증 내용 |
| --- | --- |
| `test_model_architecture.py` | 모델 구조 (입력/출력 shape) |
| `test_preprocessing.py` | 전처리 파이프라인 |
| `test_augmentation.py` | 스케치 특화 데이터 증강 (ChannelDropout 등) |
| `test_dataset.py` | HFDDataset 실제/합성 로더 |
| `test_inference.py` | PyTorch 추론 결과 |
| `test_onnx.py` | ONNX 변환 동등성 (max_abs_err < 1e-4) |
| `test_tensorrt.py` | TensorRT FP16 엔진 (3채널, Optimization Profile) |
| `test_iq_scorer.py` | IQ/백분위 변환 로직 (연령·성별 규준표) |
| `test_item_mapping.py` | 60문항 → 4헤드 매핑 |
| `test_analyze.py` | `/analyze` E2E + Confidence Guard (422) |
| `test_hybrid_combiner.py` | C++ 기하 특징 + AI 결과 결합 |
| `test_e2e_real_image.py` | 실제 스케치 이미지 End-to-End |
| `test_health.py` | `/health` 응답 |

### API Gateway (Jest)

```bash
cd api-gateway
npm test
```

### Frontend (Vitest)

```bash
cd frontend
npm test
```

### 부하 테스트 (k6)

```bash
# Smoke / Load(100 VU) / Stress(200 VU) 시나리오 제공
k6 run scripts/load-test.js
```

---

## 개발 현황

### 완료

| Phase | 내용 |
| --- | --- |
| Phase 1 | 프로젝트 설계, 아키텍처 결정 (ADR-001~036) |
| Phase 2 | React Frontend (7단계 흐름, 에러 화면, 유효성 검사, PDF) |
| Phase 3 | C++ Preprocess Server (11 필터 + 2 분석 모듈, ThreadPool, GTest 59) |
| Phase 4 | Python AI Server (EfficientNet-B2, ONNX/TensorRT FP16, pytest 147) |
| Phase 5 | API Gateway + 배포 (Hash Cache, Nginx SSL, Docker Compose, EC2) |
| QA/CI | GitHub Actions, CodeQL (JS/TS·C++), 단위·통합·k6 부하 테스트 |

### 최근 주요 마일스톤

- **2026-04-12**: 프론트엔드 에러 화면 및 입력 유효성 검사 완료 (22개 테스트)
- **2026-04-05**: ADR-034 컬러 이미지 2-Tier Fail-Fast 필터 완료, Docker Build 최적화 (6GB → 1MB)
- **2026-03-30**: EC2 c5.large 프로덕션 배포 + Let's Encrypt HTTPS 전환
- **2026-03-20**: Phase 4 AI 서버 완료 (TensorRT P95 14.1ms, 325 QPS)
- **2026-03-18**: 3-Engine 벤치마크 (PyTorch/ONNX/TensorRT)

### 진행 예정

- [ ] k6 EC2 실환경 부하 테스트 (100 VU P95 < 500ms 목표)
- [ ] Phase 6 Chaos Engineering (Circuit Breaker, Failover)
- [ ] 추가 데이터 수집(50+) 후 모델 재학습 (C++ 전처리 통합 학습)
- [ ] CRITICAL 로그 Slack/Email 알림

---

## 프로젝트 구조

```
mind-palette-project/
├── preprocess-server/          C++17 이미지 전처리 서버
│   ├── src/
│   │   ├── core/               server.h, image_processor, validation_exception
│   │   ├── filters/            11개 필터 (color_validation 포함)
│   │   ├── analysis/           pressure_analyzer, tremor_analyzer
│   │   ├── infra/              thread_pool, atomic_writer
│   │   └── utils/              Logger
│   └── tests/                  Google Test 수트 (59개)
│
├── ai-server/                  Python FastAPI 추론 서버
│   ├── src/
│   │   ├── core/               model, preprocessing, iq_scorer, augmentation, dataset
│   │   ├── infra/              model_loader, onnx/tensorrt engine
│   │   └── routes/             health, analyze (Confidence Guard)
│   ├── scripts/                train, export_model, build_tensorrt_engine
│   └── tests/                  pytest 수트 (147개)
│
├── api-gateway/                Node.js 오케스트레이터
│   ├── src/
│   │   ├── routes/             health, analyze
│   │   ├── services/           analysisService, cacheService, imageValidator
│   │   └── utils/              logger (Winston), hashIntegrity, pathValidator, fileStorage
│   └── tests/                  Jest 수트
│
├── frontend/                   React 웹 클라이언트
│   └── src/
│       ├── components/         Hero, InfoForm, Upload, Result, Error, ...
│       ├── types/              errors.ts (AnalysisError)
│       ├── utils/              validation.ts
│       └── api/                client (Axios 인터셉터), uploadApi
│
├── nginx/                      Reverse Proxy + SSL 종단
│   ├── nginx.conf              HTTPS 종단, HTTP→HTTPS 리다이렉트
│   ├── Dockerfile
│   └── ssl-local/              로컬 개발용 자체 서명 인증서
│
├── scripts/                    운영 스크립트
│   ├── load-test.js            k6 부하 테스트 시나리오
│   ├── traffic-bot.ts          주기적 요청 자동화
│   └── ci/                     CI 보조 스크립트
│
├── docker-compose.yml          프로덕션 구성 (internal-net 격리)
├── docker-compose.override.yml 로컬 개발 구성 (포트 노출)
├── docker-compose.dev.yml      개발 편의 구성
└── docs/                       설계 문서 및 ADR
    ├── standards/              CODING_STANDARDS, ADR 모음
    ├── architecture/           ARCHITECTURE_DECISIONS, ADR-parameter-rationale
    ├── guides/                 git-workflow, aws-deployment, CI/CD, MCP 등
    ├── reports/                서비스별 아키텍처 리포트
    ├── status/                 BENCHMARKS, PROGRESS, CODE_REVIEW_HISTORY
    └── troubleshooting/        Windows 빌드, CRT Mismatch 등
```

---

## 설계 문서

주요 아키텍처 결정 및 가이드는 [`docs/`](docs/) 디렉토리에 있습니다.

- [standards/CODING_STANDARDS.md](docs/standards/CODING_STANDARDS.md) — TDD, 커밋 규칙, 코드 품질 기준
- [architecture/ARCHITECTURE_DECISIONS.md](docs/architecture/ARCHITECTURE_DECISIONS.md) — ADR-001~036 전체 목록
- [guides/git-workflow-guide.md](docs/guides/git-workflow-guide.md) — Feature Branch + Conventional Commits
- [guides/aws-deployment-guide.md](docs/guides/aws-deployment-guide.md) — EC2 c5.large 배포 절차
- [guides/AI_SERVER_MODEL_OPERATIONS_GUIDE.md](docs/guides/AI_SERVER_MODEL_OPERATIONS_GUIDE.md) — 모델 학습·변환·운영
- [guides/cache-performance-verification.md](docs/guides/cache-performance-verification.md) — Hash Cache 성능 검증
- [troubleshooting/](docs/troubleshooting/) — Windows 빌드 이슈, CRT Mismatch 해결 등
