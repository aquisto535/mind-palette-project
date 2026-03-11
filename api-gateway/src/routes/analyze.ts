import express, { Request, Response, NextFunction } from 'express';
import fs from 'node:fs/promises';
import multer from 'multer';
import { upload, CustomRequest, hasValidMagicBytes } from '../utils/fileStorage';
import { processAnalysis } from '../services/analysisService';
import logger from '../utils/logger';

const router = express.Router();

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
    const fileBuffer = await fs.readFile(req.file.path);
    if (!hasValidMagicBytes(fileBuffer)) {
      await fs.unlink(req.file.path).catch(() => undefined);
      return res.status(400).json({ error: '파일 내용이 올바른 이미지 형식이 아닙니다.' });
    }
    next();
  } catch (error) {
    next(error);
  }
};

// POST /analyze
router.post('/', 
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
