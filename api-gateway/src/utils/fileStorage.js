const multer = require('multer');
const path = require('path');
const fs = require('fs');

// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
const SHARED_ROOT = path.join(__dirname, '../../../shared_volume');
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

module.exports = {
  upload,
  UPLOAD_DIR,
  RESULT_DIR
};

