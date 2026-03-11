"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.maskPII = maskPII;
const winston_1 = __importDefault(require("winston"));
const node_path_1 = __importDefault(require("node:path"));
const { combine, timestamp, printf, colorize, json } = winston_1.default.format;
// 로그 포맷 정의
const logFormat = printf(({ level, message, timestamp, ...metadata }) => {
    const requestId = metadata.requestId ? ` [${metadata.requestId}]` : '';
    let msg = `${timestamp}${requestId} [${level}] : ${message}`;
    // requestId는 이미 출력했으므로 메타데이터 복사본에서 삭제하여 중복 출력 방지
    const meta = { ...metadata };
    delete meta.requestId;
    if (Object.keys(meta).length > 0) {
        msg += ` ${JSON.stringify(meta)}`;
    }
    return msg;
});
// 로그 파일 저장 디렉토리
const LOG_DIR = node_path_1.default.join(__dirname, '../../logs');
const logger = winston_1.default.createLogger({
    level: process.env.NODE_ENV === 'production' ? 'info' : 'debug',
    format: combine(timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }), json()),
    transports: [
        // 에러 로그는 별도 파일에 저장
        new winston_1.default.transports.File({
            filename: node_path_1.default.join(LOG_DIR, 'error.log'),
            level: 'error',
        }),
        // 모든 로그를 통합 파일에 저장
        new winston_1.default.transports.File({
            filename: node_path_1.default.join(LOG_DIR, 'combined.log'),
        }),
    ],
});
// 개발 환경에서는 콘솔에도 출력
if (process.env.NODE_ENV !== 'production') {
    logger.add(new winston_1.default.transports.Console({
        format: combine(colorize(), timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }), logFormat),
    }));
}
// ─────────────────────────────────────────────
// PII 마스킹 유틸리티
// 로그 기록 전 개인 식별 가능 필드(파일명, 경로 내 파일명)를 마스킹한다.
// ─────────────────────────────────────────────
const PII_FIELDS = ['originalname'];
function maskPII(meta) {
    const result = { ...meta };
    // 원본 파일명 마스킹
    for (const field of PII_FIELDS) {
        if (typeof result[field] === 'string') {
            result[field] = '***';
        }
    }
    // 경로에서 파일명 부분 마스킹 (경로 구조는 유지)
    if (typeof result['path'] === 'string') {
        const dir = result['path'].replace(/[/\\][^/\\]+$/, '');
        result['path'] = dir + '/***';
    }
    return result;
}
exports.default = logger;
