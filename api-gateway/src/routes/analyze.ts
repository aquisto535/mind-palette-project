import express, { Request, Response, NextFunction } from 'express';
import multer from 'multer';
import { upload, CustomRequest } from '../utils/fileStorage';
import { processAnalysis } from '../services/analysisService';
import logger from '../utils/logger';

const router = express.Router();

// POST /analyze
router.post('/', (req: Request, res: Response, next: NextFunction) => {
  upload.single('image')(req, res, (err) => {
    if (err) {
      if (err instanceof multer.MulterError && err.code === 'LIMIT_FILE_SIZE') {
        return res.status(400).json({ error: 'File too large (limit: 5MB)' });
      }
      return res.status(400).json({ error: err.message });
    }
    next();
  });
}, async (req: Request, res: Response) => {
  try {
    const customReq = req as CustomRequest;
    // Multer fileFilter에서 거부된 경우 체크
    if (customReq.fileValidationError) {
      return res.status(400).json({ error: customReq.fileValidationError });
    }

    if (!req.file) {
      return res.status(400).json({ error: 'No image file uploaded' });
    }

    const requestId = (req as any).requestId;
    const result = await processAnalysis(req.file, requestId);
    res.json(result);
  } catch (error: unknown) {
    logger.error('Analysis Error:', { error: error instanceof Error ? error.message : String(error) });
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

export default router;
