import express, { Request, Response, NextFunction } from 'express';
import multer from 'multer';
import rateLimit from 'express-rate-limit';
import axios from 'axios';
import { upload, CustomRequest } from '../utils/fileStorage';
import { processAnalysis } from '../services/analysisService';
import { ImageValidator } from '../services/imageValidator';
import logger from '../utils/logger';

const router = express.Router();

// ─────────────────────────────────────────────────────────
// 미들웨어: /analyze 전용 Rate Limiting (DoS 방어)
// 분석 요청은 리소스 소모가 크므로 글로벌보다 엄격하게 제한
// ─────────────────────────────────────────────────────────
const analyzeLimiter = rateLimit({
  windowMs: 60 * 1000, // 1분
  max: 10, // IP당 분당 10회
  message: { error: 'Too many analysis requests, please try again in a minute.' },
  standardHeaders: true,
  legacyHeaders: false,
  skip: () => process.env.NODE_ENV === 'test',
});

// ─────────────────────────────────────────────────────────
// 미들웨어: Multer 업로드 처리 및 에러 핸들링
// ─────────────────────────────────────────────────────────
const handleMulterUpload = (req: Request, res: Response, next: NextFunction) => {
  upload.single('image')(req, res, (err) => {
    if (err) {
      if (err instanceof multer.MulterError && err.code === 'LIMIT_FILE_SIZE') {
        return res.status(400).json({ error: 'File too large (limit: 5MB)' });
      }
      return res.status(400).json({ error: err.message });
    }
    next();
  });
};

// ─────────────────────────────────────────────────────────
// 미들웨어: 이미지 콘텐츠 무결성 검증 (Magic Bytes & Resolution)
// ─────────────────────────────────────────────────────────
const validateImageContent = async (req: Request, res: Response, next: NextFunction) => {
  const customReq = req as CustomRequest;
  
  if (customReq.fileValidationError) {
    return res.status(400).json({ error: customReq.fileValidationError });
  }

  if (!req.file) {
    return res.status(400).json({ error: 'No image file uploaded' });
  }

  const result = await ImageValidator.validate(req.file);
  if (!result.valid) {
    if (result.error === 'Internal validation error') {
      return res.status(500).json({ error: result.error });
    }
    return res.status(result.error === 'Access denied' ? 403 : 400).json({ error: result.error });
  }

  next();
};

// POST /analyze
router.post('/', 
  analyzeLimiter,
  handleMulterUpload, 
  validateImageContent, 
  async (req: Request, res: Response) => {
    try {
      const requestId = (req as any).requestId;
      const adminProfileKey = req.header('x-admin-profile-key');

      const result = await processAnalysis(req.file!, requestId, adminProfileKey);
      
      res.set('X-Sanitization-Status', result.sanitized ? 'applied' : 'skipped');
      if (result.serverTiming) {
        res.set('Server-Timing', result.serverTiming);
      }

      const { sanitized: _sanitized, serverTiming: _serverTiming, ...responseData } = result;
      res.json(responseData);
    } catch (error: unknown) {
      logger.error('Analysis Error:', { error: error instanceof Error ? error.message : String(error) });
      // ADR-033: 하위 서비스(preprocess/ai)의 422는 클라이언트에 그대로 전달
      if (axios.isAxiosError(error) && error.response?.status === 422) {
        return res.status(422).json(error.response.data);
      }
      // ADR-033 Fail-Closed: preprocess-server 장애 시 503 반환 (원본 이미지 우회 방지)
      if (error instanceof Error && error.name === 'PreprocessServiceError') {
        return res.status(503).json({ error: 'PREPROCESS_SERVICE_UNAVAILABLE', message: '전처리 서버에 일시적인 문제가 발생했습니다. 잠시 후 다시 시도해주세요.' });
      }
      res.status(500).json({ error: 'Internal Server Error' });
    }
  }
);

export default router;
