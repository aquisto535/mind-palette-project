"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const winston_1 = __importDefault(require("winston"));
const path_1 = __importDefault(require("path"));
const { combine, timestamp, printf, colorize, json } = winston_1.default.format;
// 로그 포맷 정의
const logFormat = printf(({ level, message, timestamp, ...metadata }) => {
    let msg = `${timestamp} [${level}] : ${message}`;
    if (Object.keys(metadata).length > 0) {
        msg += ` ${JSON.stringify(metadata)}`;
    }
    return msg;
});
// 로그 파일 저장 디렉토리
const LOG_DIR = path_1.default.join(__dirname, '../../logs');
const logger = winston_1.default.createLogger({
    level: process.env.NODE_ENV === 'production' ? 'info' : 'debug',
    format: combine(timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }), json()),
    transports: [
        // 에러 로그는 별도 파일에 저장
        new winston_1.default.transports.File({
            filename: path_1.default.join(LOG_DIR, 'error.log'),
            level: 'error',
        }),
        // 모든 로그를 통합 파일에 저장
        new winston_1.default.transports.File({
            filename: path_1.default.join(LOG_DIR, 'combined.log'),
        }),
    ],
});
// 개발 환경에서는 콘솔에도 출력
if (process.env.NODE_ENV !== 'production') {
    logger.add(new winston_1.default.transports.Console({
        format: combine(colorize(), timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }), logFormat),
    }));
}
exports.default = logger;
