import express, { Request, Response, NextFunction } from 'express';
import fs from 'node:fs/promises';
import path from 'node:path';
import multer from 'multer';
import rateLimit from 'express-rate-limit';
import { upload, CustomRequest, hasValidMagicBytes, UPLOAD_DIR } from '../utils/fileStorage';
import { processAnalysis } from '../services/analysisService';
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
// 미들웨어: 이미지 콘텐츠 무결성 검증 (Magic Bytes)
// ─────────────────────────────────────────────────────────
const validateImageContent = async (req: Request, res: Response, next: NextFunction) => {
  const customReq = req as CustomRequest;
  
  if (customReq.fileValidationError) {
    return res.status(400).json({ error: customReq.fileValidationError });
  }

  if (!req.file) {
    return res.status(400).json({ error: 'No image file uploaded' });
  }

  try {
    // ─────────────────────────────────────────────────────────
    // Path Injection 방어: 파일 경로가 기획된 업로드 폴더 내에 있는지 검증
    // ─────────────────────────────────────────────────────────
    const filePath = path.resolve(req.file.path);
    // ─────────────────────────────────────────────────────────
    // P2 Fix: 트레일링 path.sep을 추가하여 sibling prefix bypass 방어
    // ─────────────────────────────────────────────────────────
    const resolvedUploadDir = path.resolve(UPLOAD_DIR) + path.sep;
    
    if (!filePath.startsWith(resolvedUploadDir)) {
      logger.error('Security Alert: Path traversal attempt blocked', { path: req.file.path });
      return res.status(403).json({ error: 'Access denied' });
    }

    const fileBuffer = await fs.readFile(filePath);
    if (!hasValidMagicBytes(fileBuffer)) {
      await fs.unlink(filePath).catch(() => undefined);
      return res.status(400).json({ error: '파일 내용이 올바른 이미지 형식이 아닙니다.' });
    }
    next();
  } catch (error) {
    next(error);
  }
};

// POST /analyze
router.post('/', 
  analyzeLimiter,
  handleMulterUpload, 
  validateImageContent, 
  async (req: Request, res: Response) => {
    try {
      const requestId = (req as any).requestId;
      const result = await processAnalysis(req.file!, requestId); // Non-null assertion safe due to middleware
      res.json(result);
    } catch (error: unknown) {
      logger.error('Analysis Error:', { error: error instanceof Error ? error.message : String(error) });
      res.status(500).json({ error: 'Internal Server Error' });
    }
  }
);

export default router;
