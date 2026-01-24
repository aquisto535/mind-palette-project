import express, { Request, Response, NextFunction } from 'express';
import { upload, CustomRequest } from '../utils/fileStorage';
import { processAnalysis } from '../services/analysisService';

const router = express.Router();

// POST /analyze
router.post('/', (req: Request, res: Response, next: NextFunction) => {
  upload.single('image')(req, res, (err: any) => {
    if (err) {
      if (err.code === 'LIMIT_FILE_SIZE') {
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

    const result = await processAnalysis(req.file);
    res.json(result);
  } catch (error: any) {
    console.error('Analysis Error:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

export default router;
