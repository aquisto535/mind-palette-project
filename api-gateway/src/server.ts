import express, { Request, Response, NextFunction } from 'express';
import cors from 'cors';
import morgan from 'morgan';
import analyzeRouter from './routes/analyze';
import healthRouter from './routes/health';
import { UPLOAD_DIR, RESULT_DIR } from './utils/fileStorage';
import { v4 as uuidv4 } from 'uuid';
import logger from './utils/logger';

const app = express();
const PORT = process.env.PORT || 3000;

// 미들웨어 설정
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Request-ID 미들웨어
app.use((req: Request, res: Response, next: NextFunction) => {
  const requestId = req.header('X-Request-ID') || uuidv4();
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
