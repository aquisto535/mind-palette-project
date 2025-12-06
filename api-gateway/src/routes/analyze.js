const express = require('express');
const router = express.Router();
const { upload } = require('../utils/fileStorage');
const analysisService = require('../services/analysisService');

// POST /analyze
router.post('/', upload.single('image'), async (req, res) => {
  try {
    const result = await analysisService.processAnalysis(req.file);
    res.json(result);
  } catch (error) {
    if (error.message === 'NO_FILE') {
      return res.status(400).json({ error: 'No image file uploaded' });
    }
    
    console.error('Analysis Error:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

module.exports = router;

