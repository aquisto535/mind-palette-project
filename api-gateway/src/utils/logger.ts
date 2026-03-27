import winston from 'winston';
import path from 'node:path';

const { combine, timestamp, printf, colorize, json } = winston.format;

// 로그 포맷 정의
const logFormat = printf(({ level, message, timestamp, ...metadata }) => {
    // requestId가 존재하면 안전하게 문자열로 변환하여 출력 형식을 갖춘다.
    // 객체인 경우 [object Object] 대신 JSON 문자열로 출력되도록 처리 (L1)
    let requestIdStr = '';
    if (metadata.requestId) {
        requestIdStr = typeof metadata.requestId === 'object' 
            ? JSON.stringify(metadata.requestId) 
            : String(metadata.requestId);
    }
    
    const requestId = requestIdStr ? ` [${requestIdStr}]` : '';
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

// ─────────────────────────────────────────────
// PII 마스킹 유틸리티
// 로그 기록 전 개인 식별 가능 필드(파일명, 경로 내 파일명)를 마스킹한다.
// ─────────────────────────────────────────────
const PII_FIELDS = ['originalname'] as const;

export function maskPII(meta: Record<string, any>): Record<string, any> {
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

export default logger;
