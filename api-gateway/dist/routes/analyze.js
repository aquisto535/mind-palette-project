"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const fileStorage_1 = require("../utils/fileStorage");
const analysisService_1 = require("../services/analysisService");
const router = express_1.default.Router();
// POST /analyze
router.post('/', (req, res, next) => {
    fileStorage_1.upload.single('image')(req, res, (err) => {
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
        const customReq = req;
        // Multer fileFilter에서 거부된 경우 체크
        if (customReq.fileValidationError) {
            return res.status(400).json({ error: customReq.fileValidationError });
        }
        if (!req.file) {
            return res.status(400).json({ error: 'No image file uploaded' });
        }
        const result = await (0, analysisService_1.processAnalysis)(req.file);
        res.json(result);
    }
    catch (error) {
        console.error('Analysis Error:', error);
        res.status(500).json({ error: 'Internal Server Error' });
    }
});
exports.default = router;
