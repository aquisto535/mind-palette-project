import winston from 'winston';
import path from 'node:path';

const { combine, timestamp, printf, colorize, json } = winston.format;

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
const LOG_DIR = path.join(__dirname, '../../logs');

const logger = winston.createLogger({
    level: process.env.NODE_ENV === 'production' ? 'info' : 'debug',
    format: combine(
        timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
        json()
    ),
    transports: [
        // 에러 로그는 별도 파일에 저장
        new winston.transports.File({
            filename: path.join(LOG_DIR, 'error.log'),
            level: 'error',
        }),
        // 모든 로그를 통합 파일에 저장
        new winston.transports.File({
            filename: path.join(LOG_DIR, 'combined.log'),
        }),
    ],
});

// 개발 환경에서는 콘솔에도 출력
if (process.env.NODE_ENV !== 'production') {
    logger.add(
        new winston.transports.Console({
            format: combine(
                colorize(),
                timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
                logFormat
            ),
        })
    );
}

export default logger;
