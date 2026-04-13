import express, { Request, Response, NextFunction } from 'express';
import cors from 'cors';
import morgan from 'morgan';
import rateLimit from 'express-rate-limit';
import analyzeRouter from './routes/analyze';
import healthRouter from './routes/health';
import { UPLOAD_DIR, RESULT_DIR } from './utils/fileStorage';
import { randomUUID } from 'node:crypto';
import logger from './utils/logger';

// ────────────────────────────────────────────────────────────────────────────
// 시작 시 환경 변수 검증 (Fail-Fast)
// 프로덕션에서 필수 설정이 누락된 채로 조용히 기동되는 것을 방지한다.
// ────────────────────────────────────────────────────────────────────────────
function validateEnv(): void {
  const isProduction = process.env.NODE_ENV === 'production';
  const warnings: string[] = [];

  if (isProduction) {
    // ADMIN_PROFILE_KEY 미설정 경고 (에러는 아니지만 운영자 인지 필요)
    if (!process.env.ADMIN_PROFILE_KEY) {
      warnings.push('ADMIN_PROFILE_KEY is not set — profiling endpoints will be disabled');
    }

    // PREPROCESS_SERVER_URL이 localhost를 가리키면 Docker 환경에서 동작 불가
    const preprocessUrl = process.env.PREPROCESS_SERVER_URL || '';
    if (preprocessUrl.includes('127.0.0.1') || preprocessUrl.includes('localhost')) {
      warnings.push(`PREPROCESS_SERVER_URL="${preprocessUrl}" points to localhost — Docker networking will fail`);
    }

    // AI_SERVER_URL 동일 검사
    const aiUrl = process.env.AI_SERVER_URL || '';
    if (aiUrl.includes('127.0.0.1') || aiUrl.includes('localhost')) {
      warnings.push(`AI_SERVER_URL="${aiUrl}" points to localhost — Docker networking will fail`);
    }

    // KEEP_IMAGES=true 프로덕션 경고
    if (process.env.KEEP_IMAGES === 'true') {
      warnings.push('KEEP_IMAGES=true in production — processed images will NOT be deleted (disk leak risk)');
    }
  }

  if (warnings.length > 0) {
    warnings.forEach(w => console.warn(`[env-check] ⚠️  ${w}`));
  }
}

validateEnv();

const app = express();
const PORT = process.env.PORT || 3000;

// 글로벌 Rate Limiting (기본 방어)
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15분
  max: 100, // IP당 최대 100회
  message: { error: 'Too many requests, please try again later.' },
  standardHeaders: true,
  legacyHeaders: false,
  skip: () => process.env.NODE_ENV === 'test',
});

// 미들웨어 설정
app.use(limiter);
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Request-ID 미들웨어
app.use((req: Request, res: Response, next: NextFunction) => {
  const requestId = req.header('X-Request-ID') || randomUUID();
  (req as any).requestId = requestId;
  res.setHeader('X-Request-ID', requestId);
  next();
});

// HTTP 요청 로깅 (morgan + winston)
const morganFormat = process.env.NODE_ENV === 'production' ? 'combined' : 'dev';
app.use(morgan(morganFormat, {
  stream: {
    write: (message: string) => {
      // morgan에서 로거로 전달할 때 requestId가 포함된 메타데이터를 함께 전달하는 것은 구조적으로 어려우나,
      // 일단 winston logger가 개별 호출에서 로그를 남기도록 유도
      logger.http(message.trim());
    }
  }
}));

// ----------------------------------------------------------------
// API 엔드포인트
// ----------------------------------------------------------------

// 기본 상태 확인
app.get('/', (req: Request, res: Response) => {
  res.send('Mind Palette API Gateway is running.');
});

// 헬스 체크 라우터 연결
app.use('/health', healthRouter);

// 분석 라우터 연결
app.use('/analyze', analyzeRouter);

// 글로벌 에러 핸들러
app.use((err: Error, req: Request, res: Response, next: NextFunction) => {
  logger.error(err.stack);
  res.status(500).json({ error: 'Internal Server Error' });
});

// 서버 시작 (테스트 환경에서는 서버를 자동으로 시작하지 않음)
if (require.main === module) {
  app.listen(PORT, () => {
    logger.info(`API Gateway running on http://localhost:${PORT}`);
    logger.info(`- Uploads: ${UPLOAD_DIR}`);
    logger.info(`- Results: ${RESULT_DIR}`);
  });
}

export default app;
