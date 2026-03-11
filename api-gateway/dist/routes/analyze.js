"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const promises_1 = __importDefault(require("node:fs/promises"));
const multer_1 = __importDefault(require("multer"));
const fileStorage_1 = require("../utils/fileStorage");
const analysisService_1 = require("../services/analysisService");
const logger_1 = __importDefault(require("../utils/logger"));
const router = express_1.default.Router();
// POST /analyze
router.post('/', (req, res, next) => {
    fileStorage_1.upload.single('image')(req, res, (err) => {
        if (err) {
            if (err instanceof multer_1.default.MulterError && err.code === 'LIMIT_FILE_SIZE') {
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
        // 매직 바이트 기반 물리적 파일 형식 검증 (MIME 위조 대응)
        const fileBuffer = await promises_1.default.readFile(req.file.path);
        if (!(0, fileStorage_1.hasValidMagicBytes)(fileBuffer)) {
            await promises_1.default.unlink(req.file.path).catch(() => undefined);
            return res.status(400).json({ error: '파일 내용이 올바른 이미지 형식이 아닙니다.' });
        }
        const requestId = req.requestId;
        const result = await (0, analysisService_1.processAnalysis)(req.file, requestId);
        res.json(result);
    }
    catch (error) {
        logger_1.default.error('Analysis Error:', { error: error instanceof Error ? error.message : String(error) });
        res.status(500).json({ error: 'Internal Server Error' });
    }
});
exports.default = router;
