const express = require('express');
const cors = require('cors'); // CORS 미들웨어
const multer = require('multer'); // Multer는 파일 업로드를 처리하는 미들웨어
const path = require('path'); // Node.js의 path 모듈
const fs = require('fs'); // Node.js의 fs 모듈

const app = express();
const PORT = process.env.PORT || 3000;

// 미들웨어 설정
app.use(cors()); // CORS 미들웨어 설정 -> 다른 도메인의 요청을 허용하는 미들웨어
app.use(express.json()); // JSON 파싱 미들웨어 설정 -> JSON 형식의 요청을 파싱하는 미들웨어
app.use(express.urlencoded({ extended: true })); // URL 인코딩 파싱 미들웨어 설정 -> URL 인코딩 형식의 요청을 파싱하는 미들웨어

// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
// Docker 환경과 로컬 환경을 모두 고려하여 경로 설정
const SHARED_ROOT = path.join(__dirname, '../shared_volume');
const UPLOAD_DIR = path.join(SHARED_ROOT, 'uploads');
const RESULT_DIR = path.join(SHARED_ROOT, 'results');

// 폴더가 없으면 생성 (안전장치)
if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
if (!fs.existsSync(RESULT_DIR)) fs.mkdirSync(RESULT_DIR, { recursive: true });

// Multer 스토리지 설정
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, UPLOAD_DIR);
  },
  filename: (req, file, cb) => {
    // 파일명: timestamp_originalname
    const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
    cb(null, uniqueSuffix + path.extname(file.originalname));
  }
});

const upload = multer({ storage: storage });

// ----------------------------------------------------------------
// API 엔드포인트
// ----------------------------------------------------------------

// 기본 상태 확인
app.get('/', (req, res) => {
  res.send('Mind Palette API Gateway is running.');
});

// POST /api/analyze
// 이미지 업로드 및 분석 요청 처리
app.post('/analyze', upload.single('image'), async (req, res) => {
  try {
    if (!req.file) {
      return res.status(400).json({ error: 'No image file uploaded' });
    }

    console.log('Image uploaded:', req.file.path);

    const imagePath = req.file.path;
    const timestamp = Date.now();
    const resultPath = path.join(RESULT_DIR, `${timestamp}_result.json`);

    // TODO: Phase 3 - C++ 전처리 서버 호출
    // TODO: Phase 4 - Python AI 서버 호출
    
    // [Phase 2] 임시 더미 데이터 생성 (계획서 참조)
    const dummyResult = generateDummyResult();

    // 결과 JSON 파일 저장
    fs.writeFileSync(resultPath, JSON.stringify(dummyResult, null, 2));
    console.log('Result saved:', resultPath);

    // 클라이언트에 결과 반환
    res.json(dummyResult);

  } catch (error) {
    console.error('Analysis Error:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// ----------------------------------------------------------------
// Helper Functions (Refactoring for TDD/Clean Code)
// ----------------------------------------------------------------

/**
 * 더미 분석 결과를 생성합니다.
 * TODO: 실제 AI 모델 연동 시 제거 또는 Mocking 용도로 사용
 */
function generateDummyResult() {
  return {
    score: Math.floor(Math.random() * (95 - 70) + 70), // 70~95점 랜덤
    percentile: Math.floor(Math.random() * (99 - 60) + 60),
    date: new Date().toLocaleDateString(),
    interpretation: "AI 분석 결과가 여기에 표시됩니다. (현재는 Mock 데이터입니다)",
    details: {
      creativity: 85,
      expression: 90,
      observational: 88
    }
  };
}

// 서버 시작 (테스트 환경에서는 서버를 자동으로 시작하지 않음)
if (require.main === module) {
  app.listen(PORT, () => {
    console.log(`API Gateway running on http://localhost:${PORT}`);
    console.log(`- Uploads: ${UPLOAD_DIR}`);
    console.log(`- Results: ${RESULT_DIR}`);
  });
}

module.exports = app;

