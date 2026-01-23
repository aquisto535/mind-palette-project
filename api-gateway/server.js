const express = require('express');
const cors = require('cors');
const morgan = require('morgan');
const analyzeRouter = require('./src/routes/analyze');
const healthRouter = require('./src/routes/health');
const { UPLOAD_DIR, RESULT_DIR } = require('./src/utils/fileStorage');
const logger = require('./src/utils/logger');

const app = express();
const PORT = process.env.PORT || 3000;

// 미들웨어 설정
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// HTTP 요청 로깅 (morgan + winston)
const morganFormat = process.env.NODE_ENV === 'production' ? 'combined' : 'dev';
app.use(morgan(morganFormat, {
  stream: { write: (message) => logger.http(message.trim()) }
}));

// ----------------------------------------------------------------
// API 엔드포인트
// ----------------------------------------------------------------

// 기본 상태 확인
app.get('/', (req, res) => {
  res.send('Mind Palette API Gateway is running.');
});

// 헬스 체크 라우터 연결
app.use('/health', healthRouter);

// 분석 라우터 연결
app.use('/analyze', analyzeRouter);

// 글로벌 에러 핸들러
app.use((err, req, res, next) => {
  logger.error(err.stack);
  res.status(500).json({ error: 'Internal Server Error' });
});

// 서버 시작 (테스트 환경에서는 서버를 자동으로 시작하지 않음)
if (require.main === module) {
  app.listen(PORT, () => {
    logger.info(`API Gateway running on http://localhost:${PORT}`);
    logger.info(`- Uploads: ${UPLOAD_DIR}`);
    logger.info(`- Results: ${RESULT_DIR}`);
  });
}

module.exports = app;

