const express = require('express');
const cors = require('cors');
const analyzeRouter = require('./src/routes/analyze');
const { UPLOAD_DIR, RESULT_DIR } = require('./src/utils/fileStorage');

const app = express();
const PORT = process.env.PORT || 3000;

// 미들웨어 설정
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// ----------------------------------------------------------------
// API 엔드포인트
// ----------------------------------------------------------------

// 기본 상태 확인
app.get('/', (req, res) => {
  res.send('Mind Palette API Gateway is running.');
});

// 분석 라우터 연결
app.use('/analyze', analyzeRouter);

// 서버 시작 (테스트 환경에서는 서버를 자동으로 시작하지 않음)
if (require.main === module) {
  app.listen(PORT, () => {
    console.log(`API Gateway running on http://localhost:${PORT}`);
    console.log(`- Uploads: ${UPLOAD_DIR}`);
    console.log(`- Results: ${RESULT_DIR}`);
  });
}

module.exports = app;

