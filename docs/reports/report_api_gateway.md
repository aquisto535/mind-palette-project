# API Gateway 모듈 기술 보고서

## Node.js 기반 API 게이트웨이 — 설계 및 구현 상세 보고서

---

**프로젝트명:** Mind Palette  
**모듈명:** API Gateway  
**기술 스택:** Node.js · Express 4 · TypeScript · Multer · Winston · Jest  
**작성일:** 2026-02-14  
**문서 유형:** 기술 보고서 (논문 형태)

---

## 목차

1. [서론](#1-서론)
2. [시스템 개요 및 요구사항](#2-시스템-개요-및-요구사항)
3. [기술 스택 선정 근거](#3-기술-스택-선정-근거)
4. [프로젝트 구조 및 아키텍처](#4-프로젝트-구조-및-아키텍처)
5. [빌드 및 개발 환경 구성](#5-빌드-및-개발-환경-구성)
6. [서버 코어 (server.ts) 상세 분석](#6-서버-코어-serverts-상세-분석)
7. [라우트 계층 상세 분석](#7-라우트-계층-상세-분석)
8. [서비스 계층 (비즈니스 로직)](#8-서비스-계층-비즈니스-로직)
9. [유틸리티 모듈](#9-유틸리티-모듈)
10. [보안 및 안정성](#10-보안-및-안정성)
11. [테스트 전략](#11-테스트-전략)
12. [마이크로서비스 통신 아키텍처](#12-마이크로서비스-통신-아키텍처)
13. [확장성 및 향후 개선 방향](#13-확장성-및-향후-개선-방향)
14. [결론](#14-결론)

---

## 1. 서론

### 1.1 배경 및 목적

API Gateway는 Mind Palette 프로젝트의 **중앙 진입점(Single Entry Point)**으로, 프론트엔드 클라이언트로부터의 모든 HTTP 요청을 수신하고, 내부 마이크로서비스(Preprocess Server, 향후 AI Server)로 분배하는 역할을 담당한다.

**마이크로서비스 아키텍처에서의 위치:**

```
┌─────────┐     HTTP      ┌──────────────┐     HTTP     ┌──────────────────┐
│ Frontend │ ───────────→ │  API Gateway  │ ──────────→ │ Preprocess Server │
│ (React)  │     :3000    │  (Express)    │    :8081    │ (C++/Crow)        │
└─────────┘               │   Port 3000   │             └──────────────────┘
                          │               │     HTTP     ┌──────────────────┐
                          │               │ ──────────→ │ AI Server (예정)  │
                          └──────────────┘    :8082     │ (Python/FastAPI)  │
                                                        └──────────────────┘
```

### 1.2 핵심 책임

1. **요청 라우팅:** 프론트엔드 → 적절한 백엔드 서비스로 요청 전달
2. **파일 관리:** Multer를 통한 이미지 파일 업로드 처리 및 디스크 저장
3. **에러 핸들링:** 표준화된 에러 응답 형식으로 클라이언트에 전달
4. **보안:** 파일 크기 제한, MIME 타입 검증, Rate Limiting
5. **모니터링:** 헬스 체크 엔드포인트, 구조화된 로깅 (Winston)
6. **공유 저장소 관리:** `shared_volume/uploads`와 `shared_volume/results` 디렉토리 관리

### 1.3 설계 원칙

- **관심사 분리(Separation of Concerns):** 라우트, 서비스, 유틸리티로 명확한 계층 분리
- **실패 허용(Fault Tolerance):** Preprocess Server 장애 시에도 Mock 분석 결과 반환
- **테스트 가능성(Testability):** `require.main === module` 패턴으로 서버 인스턴스를 테스트에서 재사용
- **점진적 확장(Progressive Enhancement):** 현재 Mock 데이터 기반, 향후 실제 AI 서버 연동으로 점진적 전환

---

## 2. 시스템 개요 및 요구사항

### 2.1 기능적 요구사항

| ID | 설명 | 상태 |
|---|---|---|
| FR-01 | 이미지 파일 업로드 수신 (multipart/form-data) | ✅ 완료 |
| FR-02 | 이미지 MIME 타입 검증 (image/* 만 허용) | ✅ 완료 |
| FR-03 | 파일 크기 제한 (5MB) | ✅ 완료 |
| FR-04 | C++ Preprocess Server로 전처리 요청 전달 | ✅ 완료 |
| FR-05 | AI 분석 결과 생성 (현재 Mock) | ✅ 완료 |
| FR-06 | 분석 결과 JSON 파일 저장 | ✅ 완료 |
| FR-07 | 헬스 체크 (서버 상태, 메모리, 디스크) | ✅ 완료 |
| FR-08 | 구조화된 로깅 (Winston) | ✅ 완료 |

### 2.2 비기능적 요구사항

| ID | 설명 | 기준 |
|---|---|---|
| NFR-01 | API 응답 시간 (전처리 제외) | < 100ms |
| NFR-02 | 동시 접속 처리 | Node.js 이벤트 루프 기반 |
| NFR-03 | Rate Limiting | 헬스 체크: 60req/min/IP |
| NFR-04 | CORS 지원 | 모든 오리진 허용 (개발 단계) |

---

## 3. 기술 스택 선정 근거

### 3.1 Node.js + Express

**선정 이유:**

- **비동기 I/O:** 파일 업로드, Preprocess Server 호출 등 I/O 바운드 작업에 최적화된 이벤트 루프 모델
- **생태계:** npm의 방대한 미들웨어 생태계 (multer, cors, morgan, express-rate-limit 등)
- **JSON 네이티브:** JavaScript 객체와 JSON의 원활한 상호 변환
- **프론트엔드와의 기술 스택 통일:** TypeScript를 프론트엔드와 공유하여 타입 정의 재사용 가능성

### 3.2 TypeScript

**컴파일러 설정 분석:**

```json
{
  "compilerOptions": {
    "target": "ES2020",                      // 최신 JS 기능 사용
    "module": "commonjs",                    // Node.js 호환 모듈 시스템
    "outDir": "./dist",                      // 컴파일 출력 디렉토리
    "rootDir": "./src",                      // 소스 루트
    "strict": true,                          // 엄격한 타입 체크
    "esModuleInterop": true,                 // CommonJS/ESM 상호 운용
    "skipLibCheck": true,                    // @types 충돌 방지
    "forceConsistentCasingInFileNames": true  // 파일명 대소문자 일관성
  }
}
```

**Frontend와의 차이점:**

| 설정 | Frontend | API Gateway | 이유 |
|---|---|---|---|
| `module` | `ESNext` | `commonjs` | Node.js의 기본 모듈 시스템 |
| `jsx` | `react-jsx` | 없음 | 서버에서 JSX 불필요 |
| `noEmit` | `true` | 없음 (기본: `false`) | API Gateway는 `tsc`로 직접 컴파일 |
| `outDir` | 없음 | `./dist` | 컴파일 결과물 분리 |

### 3.3 주요 의존성 역할

| 패키지 | 역할 | 선정 이유 |
|---|---|---|
| `express` | HTTP 서버 프레임워크 | 사실상 Node.js 표준 |
| `multer` | 파일 업로드 처리 | `multipart/form-data` 전문 미들웨어 |
| `axios` | HTTP 클라이언트 | Preprocess Server 호출용 |
| `cors` | CORS 헤더 설정 | 프론트엔드와의 크로스 오리진 통신 |
| `morgan` | HTTP 요청 로깅 | 표준화된 접근 로그 |
| `winston` | 구조화된 로깅 | 파일 로테이팅, 로그 레벨 분리 |
| `express-rate-limit` | 요청 속도 제한 | DDoS 방지, 리소스 보호 |
| `dotenv` | 환경 변수 관리 | 설정 외부화 |

### 3.4 개발 의존성

| 패키지 | 역할 |
|---|---|
| `jest` + `ts-jest` | TypeScript 테스트 프레임워크 |
| `supertest` | Express 앱 HTTP 테스트 |
| `nock` | HTTP 요청 모킹 |
| `nodemon` | 파일 변경 감지 자동 재시작 |
| `rimraf` | 크로스 플랫폼 디렉토리 삭제 (테스트 정리) |

---

## 4. 프로젝트 구조 및 아키텍처

### 4.1 디렉토리 구조

```
api-gateway/
├── package.json              # 프로젝트 매니페스트
├── tsconfig.json             # TypeScript 컴파일러 설정
├── jest.config.js            # Jest 테스트 설정 (ts-jest)
├── .gitignore                # Git 제외 패턴
├── test-rate-limit.ps1       # Rate Limit 수동 테스트 스크립트
├── src/                      # TypeScript 소스
│   ├── server.ts             # Express 앱 설정 + 미들웨어 + 에러 핸들러
│   ├── routes/               # 라우트 정의
│   │   ├── analyze.ts        # POST /analyze — 이미지 분석
│   │   └── health.ts         # GET /health — 헬스 체크
│   ├── services/             # 비즈니스 로직
│   │   └── analysisService.ts # 분석 처리 (전처리 서버 호출 + Mock 결과)
│   └── utils/                # 유틸리티
│       ├── fileStorage.ts    # Multer 설정 + 저장소 관리
│       └── logger.ts         # Winston 로거
├── tests/                    # 테스트
│   ├── server.test.ts        # 서버 기본 기능 테스트
│   ├── health.test.ts        # 헬스 체크 테스트
│   ├── analysisService.test.ts  # 분석 서비스 로직 테스트
│   ├── integration.test.ts   # 통합 테스트
│   ├── security.test.ts      # 보안 테스트 (파일 타입, 크기 제한)
│   └── dummy.jpg             # 테스트용 더미 이미지
├── dist/                     # 컴파일된 JavaScript 출력
└── logs/                     # Winston 로그 파일 (런타임 생성)
```

### 4.2 계층 아키텍처

```
┌────────────────────────────────────────────────┐
│           HTTP Layer (Express Middleware)        │
│  CORS → JSON Parser → Morgan → Routes          │
├────────────────────────────────────────────────┤
│           Route Layer (요청/응답 처리)            │
│  analyze.ts: 파일 업로드 + 분석 요청              │
│  health.ts: 시스템 상태 조회                     │
├────────────────────────────────────────────────┤
│           Service Layer (비즈니스 로직)           │
│  analysisService.ts: Preprocess 호출 + Mock     │
├────────────────────────────────────────────────┤
│           Utility Layer                         │
│  fileStorage.ts: Multer + 디렉토리 관리          │
│  logger.ts: Winston 구조화 로깅                  │
├────────────────────────────────────────────────┤
│           External Services                     │
│  Preprocess Server (C++, :8081)                 │
│  AI Server (Python, :8082, 예정)                │
└────────────────────────────────────────────────┘
```

---

## 5. 빌드 및 개발 환경 구성

### 5.1 npm 스크립트

| 스크립트 | 명령어 | 용도 |
|---|---|---|
| `dev` | `nodemon src/server.ts` | 개발 서버 (핫 리로드) |
| `build` | `tsc` | TypeScript → JavaScript 컴파일 |
| `start` | `node dist/server.js` | 프로덕션 서버 실행 |
| `test` | `jest` | 테스트 실행 |

### 5.2 Jest 설정

```javascript
module.exports = {
    preset: 'ts-jest',          // TypeScript 파일 직접 테스트
    testEnvironment: 'node',    // Node.js 환경 (DOM 없음)
    transform: {
        '^.+\\.tsx?$': ['ts-jest', {}],
    },
};
```

---

## 6. 서버 코어 (server.ts) 상세 분석

### 6.1 미들웨어 스택

```typescript
const app = express();

// 1. CORS — 모든 오리진에서의 요청 허용 (개발 환경)
app.use(cors());

// 2. Body Parser — JSON 및 URL-encoded 요청 본문 파싱
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// 3. Morgan — HTTP 접근 로그를 Winston으로 스트리밍
const morganFormat = process.env.NODE_ENV === 'production' ? 'combined' : 'dev';
app.use(morgan(morganFormat, {
    stream: { write: (message: string) => logger.http(message.trim()) }
}));
```

**Morgan-Winston 통합 패턴:**

- **Morgan:** HTTP 요청/응답을 포맷팅 (`dev` = 간결, `combined` = Apache 표준)
- **Winston:** Morgan의 출력을 `stream` 인터페이스로 수신하여 파일 로그에 저장
- **로그 레벨:** `logger.http()` — Winston의 사용자 정의 레벨 활용

### 6.2 라우트 등록

```typescript
// 기본 상태 메시지
app.get('/', (req, res) => {
    res.send('Mind Palette API Gateway is running.');
});

// 헬스 체크 라우터
app.use('/health', healthRouter);

// 분석 라우터
app.use('/analyze', analyzeRouter);
```

### 6.3 글로벌 에러 핸들러

```typescript
app.use((err: any, req: Request, res: Response, next: NextFunction) => {
    logger.error(err.stack);
    res.status(500).json({ error: 'Internal Server Error' });
});
```

- Express의 4-매개변수 미들웨어 시그니처로 에러 핸들러 등록
- 스택 트레이스를 Winston 에러 로그에 기록
- 클라이언트에는 내부 구현 상세를 노출하지 않는 표준 에러 응답

### 6.4 테스트 가능한 서버 패턴

```typescript
// 테스트 환경에서는 서버를 자동으로 시작하지 않음
if (require.main === module) {
    app.listen(PORT, () => {
        logger.info(`API Gateway running on http://localhost:${PORT}`);
    });
}

export default app;  // Supertest에서 import하여 사용
```

**설계 의도:**

- `require.main === module`: 이 파일이 직접 실행된 경우에만 `listen()` 호출
- `export default app`: Supertest가 `request(app).get('/').expect(200)`처럼 서버 인스턴스에 직접 접근
- 포트 충돌 방지: 테스트 실행 시 실제 포트를 점유하지 않음

---

## 7. 라우트 계층 상세 분석

### 7.1 POST /analyze — 이미지 분석

**파일:** `src/routes/analyze.ts`

```typescript
router.post('/', (req, res, next) => {
    // Step 1: Multer 파일 업로드 처리
    upload.single('image')(req, res, (err: any) => {
        if (err) {
            if (err.code === 'LIMIT_FILE_SIZE') {
                return res.status(400).json({ error: 'File too large (limit: 5MB)' });
            }
            return res.status(400).json({ error: err.message });
        }
        next();
    });
}, async (req, res) => {
    // Step 2: 검증 및 분석 처리
    const customReq = req as CustomRequest;
    if (customReq.fileValidationError) {
        return res.status(400).json({ error: customReq.fileValidationError });
    }
    if (!req.file) {
        return res.status(400).json({ error: 'No image file uploaded' });
    }

    const result = await processAnalysis(req.file);
    res.json(result);
});
```

**요청 처리 플로우:**

```
POST /analyze (multipart/form-data, field: "image")
    │
    ▼ Multer Middleware
    ├── MIME 타입 검증 (image/* 만 허용)
    ├── 파일 크기 검증 (5MB 제한)
    ├── 파일명 생성 (timestamp_random.ext)
    └── shared_volume/uploads/ 에 저장
    │
    ▼ fileValidationError 확인
    ├── MIME 필터 거부 시 → 400 Only image files are allowed
    └── 파일 누락 시 → 400 No image file uploaded
    │
    ▼ processAnalysis(file)
    ├── Preprocess Server에 HTTP POST 요청
    │   ├── 성공: processedImagePath 업데이트
    │   └── 실패: 원본 이미지 경로 유지 (Graceful Degradation)
    ├── Mock 분석 결과 생성
    └── 결과 JSON 파일 저장 → shared_volume/results/
    │
    ▼ 응답 반환
    { score, percentile, date, interpretation, details }
```

### 7.2 GET /health — 헬스 체크

**파일:** `src/routes/health.ts`

**Rate Limiting 적용:**

```typescript
const healthRateLimiter = rateLimit({
    windowMs: 1 * 60 * 1000,  // 1분 윈도우
    max: 60,                   // IP당 1분에 최대 60요청
    message: { status: 'error', message: 'Too many health check requests...' },
    standardHeaders: true,     // RateLimit-* 표준 헤더
    legacyHeaders: false,      // X-RateLimit-* 비활성화
});
```

**시스템 메트릭 수집:**

```typescript
router.get('/', healthRateLimiter, (req, res) => {
    // 1. 서버 가동 시간
    const uptime = process.uptime();

    // 2. Node.js 힙 메모리 사용량
    const { heapUsed, heapTotal } = process.memoryUsage();

    // 3. 디스크 여유 공간 (크로스 플랫폼)
    let diskAvailableGB;
    if (os.platform() === 'win32') {
        // Windows: wmic 명령어
        const output = execSync('wmic logicaldisk where "DeviceID=\'C:\'" get FreeSpace');
    } else {
        // Linux/Mac: df 명령어
        const output = execSync("df -k / | tail -1 | awk '{print $4}'");
    }

    res.json({
        status: 'healthy',
        timestamp: new Date().toISOString(),
        uptime: `${minutes} minutes ${seconds} seconds`,
        memory: { used: `${heapUsedMB} MB`, total: `${heapTotalMB} MB` },
        disk: { available: diskAvailableGB }
    });
});
```

**Rate Limiting을 헬스 체크에 적용한 이유:**

- `execSync` 호출로 OS 명령을 동기 실행 → CPU 블로킹 발생
- 악의적 또는 과도한 모니터링 툴에 의한 서버 과부하 방지
- 헬스 체크 1분에 60번(1초에 1번)이면 대부분의 모니터링 시나리오에 충분

---

## 8. 서비스 계층 (비즈니스 로직)

### 8.1 analysisService.ts

**파일:** `src/services/analysisService.ts`

**인터페이스 정의:**

```typescript
interface AnalysisResult {
    score: number;               // 인지 발달 점수 (70~95)
    percentile: number;          // 또래 대비 백분위 (60~99)
    date: string;                // 분석 날짜 (로케일 문자열)
    interpretation: string;      // 종합 해석 텍스트
    details: {
        creativity: number;      // 창의성 점수
        expression: number;      // 표현력 점수
        observational: number;   // 관찰력 점수
    };
}
```

### 8.2 처리 플로우

```typescript
export const processAnalysis = async (file: Express.Multer.File): Promise<AnalysisResult> => {
    // Phase 1: 입력 검증
    if (!file) throw new Error('NO_FILE');

    // Phase 2: C++ Preprocess Server 호출 (Graceful Degradation)
    let processedImagePath = file.path;
    try {
        const preprocessRes = await axios.post(
            `${PREPROCESS_SERVER_URL}/preprocess`,
            { imagePath: file.path }
        );
        if (preprocessRes.data?.processedPath) {
            processedImagePath = preprocessRes.data.processedPath;
        }
    } catch (error) {
        console.warn('Preprocessing failed, using original image:', error.message);
        // 전처리 실패 시에도 원본으로 계속 진행
    }

    // Phase 3: AI 분석 (현재 Mock, 향후 Python 서버 호출)
    const resultData = generateDummyResult();

    // Phase 4: 결과 저장
    const resultPath = path.join(RESULT_DIR, `${Date.now()}_result.json`);
    fs.writeFileSync(resultPath, JSON.stringify(resultData, null, 2));

    return resultData;
};
```

**Graceful Degradation 전략:**

- Preprocess Server 장애 시에도 `catch` 블록에서 에러를 삼키고 원본 이미지 경로를 유지
- 전체 서비스가 중단되지 않고 분석 결과(현재 Mock)를 반환
- 프로덕션에서는 에러 발생 시 사용자에게 알림 또는 재시도 로직 추가 필요

### 8.3 Mock 데이터 생성

```typescript
function generateDummyResult(): AnalysisResult {
    return {
        score: Math.floor(Math.random() * (95 - 70) + 70),
        percentile: Math.floor(Math.random() * (99 - 60) + 60),
        date: new Date().toLocaleDateString(),
        interpretation: "AI 분석 결과가 여기에 표시됩니다. (현재는 Mock 데이터입니다)",
        details: { creativity: 85, expression: 90, observational: 88 }
    };
}
```

**향후 Phase 4 구현 시 교체 지점:** `generateDummyResult()` → Python AI Server HTTP 호출

---

## 9. 유틸리티 모듈

### 9.1 fileStorage.ts — 파일 업로드 관리

**Multer 스토리지 설정:**

```typescript
const SHARED_ROOT = path.join(__dirname, '../../../shared_volume');
export const UPLOAD_DIR = path.join(SHARED_ROOT, 'uploads');
export const RESULT_DIR = path.join(SHARED_ROOT, 'results');

// 자동 디렉토리 생성
if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
if (!fs.existsSync(RESULT_DIR)) fs.mkdirSync(RESULT_DIR, { recursive: true });
```

**공유 볼륨 아키텍처:**

```
mind-palette-project/
  ├── api-gateway/          → UPLOAD_DIR에 이미지 저장
  ├── preprocess-server/    → UPLOAD_DIR에서 이미지 읽기, RESULT_DIR에 저장
  └── shared_volume/        ← 공유 파일 시스템
      ├── uploads/          ← 원본 이미지
      └── results/          ← 전처리 결과 + 분석 결과 JSON
```

**이 구조의 의미:**

- Docker 환경에서는 `shared_volume`이 바인드 마운트로 공유됨
- 개발 환경에서는 상대 경로로 같은 디렉토리를 참조
- API Gateway와 Preprocess Server 간 파일 시스템 기반 통신

**Multer diskStorage 세부 설정:**

```typescript
const storage = multer.diskStorage({
    destination: (req, file, cb) => cb(null, UPLOAD_DIR),
    filename: (req, file, cb) => {
        const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
        cb(null, uniqueSuffix + path.extname(file.originalname));
    }
});
```

- **유니크 파일명:** `Date.now()` + 난수로 파일명 충돌 방지
- **원본 확장자 유지:** `path.extname(file.originalname)`

**MIME 타입 필터 (비-에러 방식):**

```typescript
const fileFilter = (req: Request, file: Express.Multer.File, cb: FileFilterCallback) => {
    const customReq = req as CustomRequest;
    if (file.mimetype.startsWith('image/')) {
        cb(null, true);
    } else {
        customReq.fileValidationError = 'Only image files are allowed';
        cb(null, false);  // 에러를 던지지 않고 플래그 설정
    }
};
```

**비-에러 방식 선택 이유:**

- Multer의 `fileFilter`에서 에러를 `throw`하면 `ECONNRESET` 오류가 발생할 수 있음
- `cb(null, false)`로 파일 거부 후, 후속 미들웨어에서 `fileValidationError` 플래그를 확인하여 응답
- 이 패턴은 Multer의 공식 문서에서도 권장하는 안전한 검증 방식

**CustomRequest 인터페이스:**

```typescript
export interface CustomRequest extends Request {
    fileValidationError?: string;
}
```

### 9.2 logger.ts — Winston 로깅 시스템

**로거 아키텍처:**

```typescript
const logger = winston.createLogger({
    level: process.env.NODE_ENV === 'production' ? 'info' : 'debug',
    format: combine(timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }), json()),
    transports: [
        // 에러 전용 파일 (error.log)
        new winston.transports.File({ filename: 'logs/error.log', level: 'error' }),
        // 전체 로그 파일 (combined.log)
        new winston.transports.File({ filename: 'logs/combined.log' }),
    ],
});

// 개발 환경에서만 콘솔 출력 추가
if (process.env.NODE_ENV !== 'production') {
    logger.add(new winston.transports.Console({
        format: combine(colorize(), timestamp(), logFormat),
    }));
}
```

**로그 레벨 계층:**

| 레벨 | 숫자 | 용도 |
|---|---|---|
| `error` | 0 | 오류 (error.log에 별도 저장) |
| `warn` | 1 | 경고 (전처리 실패 등) |
| `info` | 2 | 일반 정보 (서버 시작, 파일 저장) |
| `http` | 3 | HTTP 접근 로그 (Morgan 연동) |
| `debug` | 4 | 디버그 정보 (개발 환경에서만) |

**환경별 동작:**

| 환경 | 로그 레벨 | 콘솔 출력 | 파일 저장 |
|---|---|---|---|
| 개발 (기본) | `debug` | ✅ (색상) | ✅ |
| 프로덕션 | `info` | ❌ | ✅ |

---

## 10. 보안 및 안정성

### 10.1 입력 검증 계층

```
클라이언트 요청
    │
    ▼ Multer fileFilter
    ├── MIME 타입 검증 (image/* 만)
    └── 파일 크기 제한 (5MB)
    │
    ▼ Route Handler
    ├── fileValidationError 플래그 확인
    └── req.file 존재 확인
    │
    ▼ 처리 진행
```

### 10.2 Rate Limiting

- 헬스 체크 엔드포인트: 1분당 60회 (IP 기준)
- `standardHeaders: true`: 표준 `RateLimit-*` 헤더 포함으로 클라이언트 측 대응 가능
- 분석 엔드포인트: 현재 미적용 (향후 추가 권장)

### 10.3 CORS 설정

```typescript
app.use(cors());  // 모든 오리진 허용 (개발 단계)
```

**프로덕션 권장 설정:**

```typescript
app.use(cors({
    origin: ['https://mind-palette.netlify.app'],
    methods: ['GET', 'POST'],
    credentials: true,
}));
```

### 10.4 에러 정보 은닉

글로벌 에러 핸들러에서 내부 스택 트레이스를 클라이언트에 노출하지 않고, 로그 파일에만 기록:

```typescript
app.use((err, req, res, next) => {
    logger.error(err.stack);                               // 내부 로그
    res.status(500).json({ error: 'Internal Server Error' }); // 외부 응답
});
```

---

## 11. 테스트 전략

### 11.1 테스트 분류

| 파일 | 테스트 유형 | 대상 |
|---|---|---|
| `server.test.ts` | 단위 | 기본 라우트, 미들웨어 |
| `health.test.ts` | 단위 | 헬스 체크 응답 형식 |
| `analysisService.test.ts` | 단위 | 분석 서비스 로직 |
| `integration.test.ts` | 통합 | 전체 분석 플로우 |
| `security.test.ts` | 보안 | 파일 타입/크기 제한 |

### 11.2 테스트 도구 조합

- **Supertest:** Express 앱에 직접 HTTP 요청 전송 (실제 포트 바인딩 불필요)
- **Nock:** Preprocess Server HTTP 호출을 모킹
- **Jest:** 테스트 러너 + 어서션 + 모킹

### 11.3 수동 테스트 스크립트

**Rate Limit 테스트 (`test-rate-limit.ps1`):**

```powershell
# 1초 간격으로 65번 헬스 체크 요청 → 61번째부터 429 응답 확인
for ($i = 1; $i -le 65; $i++) {
    $response = Invoke-WebRequest -Uri "http://localhost:3000/health" -Method GET
    Write-Host "Request $i : Status $($response.StatusCode)"
    Start-Sleep -Seconds 1
}
```

---

## 12. 마이크로서비스 통신 아키텍처

### 12.1 서비스 간 통신 패턴

```
Frontend → API Gateway: RESTful HTTP (multipart/form-data)
API Gateway → Preprocess Server: RESTful HTTP (JSON)
API Gateway → AI Server (향후): RESTful HTTP (JSON)
API Gateway ↔ 공유 볼륨: 파일 시스템 (shared_volume/)
```

### 12.2 서비스 디스커버리

현재 **정적 구성** 방식:

```typescript
const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
```

- 환경 변수 `PREPROCESS_SERVER_URL`로 외부 주입 가능
- Docker Compose 환경에서는 서비스명으로 해석 (예: `http://preprocess-server:8081`)
- 기본값 `localhost:8081`로 로컬 개발 시 즉시 작동

### 12.3 향후 확장: AI Server 연동

```
processAnalysis() 내부:

Phase 3 (현재): generateDummyResult()
    ↓ 교체
Phase 4 (향후): axios.post(AI_SERVER_URL + '/predict', {
    imagePath: processedImagePath,
    childAge: childAge,
    childGender: childGender
})
```

---

## 13. 확장성 및 향후 개선 방향

### 13.1 아키텍처 개선

- **API 버전닝:** `/v1/analyze`, `/v2/analyze` 경로 분리
- **요청 큐잉:** Bull/BullMQ를 활용한 분석 요청 큐 도입 (대량 트래픽 대응)
- **서킷 브레이커:** opossum 라이브러리로 Preprocess Server 장애 시 빠른 실패(Fast Fail)
- **결과 캐싱:** Redis를 활용한 동일 이미지 분석 결과 캐싱

### 13.2 보안 강화

- **Helmet.js:** HTTP 보안 헤더 자동 설정
- **CORS 화이트리스트:** 프로덕션 도메인만 허용
- **요청 본문 크기 제한:** `express.json({ limit: '1mb' })`
- **분석 엔드포인트 Rate Limiting:** IP당 1분에 10회 등

### 13.3 모니터링 확장

- **Prometheus 메트릭:** 요청 수, 처리 시간, 에러율 등
- **분산 추적:** OpenTelemetry로 서비스 간 요청 추적
- **로그 집중화:** ELK 스택 또는 Datadog 로그 관리

### 13.4 인프라

- **Docker Compose:** 전체 서비스 스택 원클릭 실행
- **CI/CD:** GitHub Actions에서 테스트 → 빌드 → Docker 이미지 푸시 → 배포
- **환경 분리:** `.env.development`, `.env.production` 환경별 설정

---

## 14. 결론

API Gateway는 **Express.js + TypeScript** 기반의 경량하면서도 견고한 중앙 진입점으로, Mind Palette 프로젝트의 마이크로서비스 아키텍처에서 프론트엔드와 백엔드 서비스를 연결하는 핵심 역할을 수행한다.

**Multer 기반 파일 업로드**, **Winston 구조화 로깅**, **express-rate-limit 속도 제한**, **Graceful Degradation 패턴**(Preprocess Server 장애 허용) 등 실무에서 중요한 프로덕션 레벨의 패턴들이 적용되어 있다.

특히 **`require.main === module` 패턴**을 통한 테스트 가능한 서버 설계와, **CustomRequest 확장을 통한 비-에러 방식 파일 검증**은 Express.js 애플리케이션의 모범 사례(Best Practice)를 잘 반영하고 있다.

현재는 Mock 데이터 기반의 MVP 단계이나, 서비스 계층의 `processAnalysis()` 함수 내 명확한 Phase 구분(Phase 2: 전처리, Phase 3: Mock, Phase 4: AI)으로 향후 실제 AI 서버 연동 시 최소한의 코드 변경으로 전환이 가능하도록 설계되었다.

---

**부록: 파일 목록 및 코드 라인 수**

| 파일 | 라인 수 | 주요 역할 |
|---|---|---|
| `server.ts` | 54 | Express 앱 설정, 미들웨어, 에러 핸들러 |
| `routes/analyze.ts` | 39 | 이미지 분석 라우트 |
| `routes/health.ts` | 83 | 헬스 체크 + Rate Limiting |
| `services/analysisService.ts` | 81 | 분석 비즈니스 로직 + Mock |
| `utils/fileStorage.ts` | 52 | Multer 설정 + 저장소 관리 |
| `utils/logger.ts` | 51 | Winston 로깅 시스템 |
| **소스 합계** | **360** | |
| `tests/server.test.ts` | 80 | 서버 기본 테스트 |
| `tests/health.test.ts` | 62 | 헬스 체크 테스트 |
| `tests/analysisService.test.ts` | 56 | 분석 서비스 테스트 |
| `tests/integration.test.ts` | 70 | 통합 테스트 |
| `tests/security.test.ts` | 72 | 보안 테스트 |
| **테스트 합계** | **340** | |
| **전체 합계** | **700** | |
