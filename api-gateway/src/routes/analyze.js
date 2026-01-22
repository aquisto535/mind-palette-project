const express = require('express');
const router = express.Router();
const { upload } = require('../utils/fileStorage');
const analysisService = require('../services/analysisService');

// POST /analyze
router.post('/', (req, res, next) => {
  upload.single('image')(req, res, (err) => {
    if (err) {
      if (err.code === 'LIMIT_FILE_SIZE') {
        return res.status(400).json({ error: 'File too large (limit: 5MB)' });
      }
      return res.status(400).json({ error: err.message });
    }
    next();
  });
}, async (req, res) => {
  try {
    // Multer fileFilter에서 거부된 경우 체크
    if (req.fileValidationError) {
      return res.status(400).json({ error: req.fileValidationError });
    }
    
    if (!req.file) {
      return res.status(400).json({ error: 'No image file uploaded' });
    }

    const result = await analysisService.processAnalysis(req.file);
    res.json(result);
  } catch (error) {
    console.error('Analysis Error:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

module.exports = router;

