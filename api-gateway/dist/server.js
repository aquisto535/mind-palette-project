"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const cors_1 = __importDefault(require("cors"));
const morgan_1 = __importDefault(require("morgan"));
const analyze_1 = __importDefault(require("./routes/analyze"));
const health_1 = __importDefault(require("./routes/health"));
const fileStorage_1 = require("./utils/fileStorage");
const node_crypto_1 = require("node:crypto");
const logger_1 = __importDefault(require("./utils/logger"));
const app = (0, express_1.default)();
const PORT = process.env.PORT || 3000;
// 미들웨어 설정
app.use((0, cors_1.default)());
app.use(express_1.default.json());
app.use(express_1.default.urlencoded({ extended: true }));
// Request-ID 미들웨어
app.use((req, res, next) => {
    const requestId = req.header('X-Request-ID') || (0, node_crypto_1.randomUUID)();
    req.requestId = requestId;
    res.setHeader('X-Request-ID', requestId);
    next();
});
// HTTP 요청 로깅 (morgan + winston)
const morganFormat = process.env.NODE_ENV === 'production' ? 'combined' : 'dev';
app.use((0, morgan_1.default)(morganFormat, {
    stream: {
        write: (message) => {
            // morgan에서 로거로 전달할 때 requestId가 포함된 메타데이터를 함께 전달하는 것은 구조적으로 어려우나,
            // 일단 winston logger가 개별 호출에서 로그를 남기도록 유도
            logger_1.default.http(message.trim());
        }
    }
}));
// ----------------------------------------------------------------
// API 엔드포인트
// ----------------------------------------------------------------
// 기본 상태 확인
app.get('/', (req, res) => {
    res.send('Mind Palette API Gateway is running.');
});
// 헬스 체크 라우터 연결
app.use('/health', health_1.default);
// 분석 라우터 연결
app.use('/analyze', analyze_1.default);
// 글로벌 에러 핸들러
app.use((err, req, res, next) => {
    logger_1.default.error(err.stack);
    res.status(500).json({ error: 'Internal Server Error' });
});
// 서버 시작 (테스트 환경에서는 서버를 자동으로 시작하지 않음)
if (require.main === module) {
    app.listen(PORT, () => {
        logger_1.default.info(`API Gateway running on http://localhost:${PORT}`);
        logger_1.default.info(`- Uploads: ${fileStorage_1.UPLOAD_DIR}`);
        logger_1.default.info(`- Results: ${fileStorage_1.RESULT_DIR}`);
    });
}
exports.default = app;
