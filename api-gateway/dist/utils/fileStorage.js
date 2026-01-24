"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.upload = exports.RESULT_DIR = exports.UPLOAD_DIR = void 0;
const multer_1 = __importDefault(require("multer"));
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
const SHARED_ROOT = path_1.default.join(__dirname, '../../../shared_volume');
exports.UPLOAD_DIR = path_1.default.join(SHARED_ROOT, 'uploads');
exports.RESULT_DIR = path_1.default.join(SHARED_ROOT, 'results');
// 폴더가 없으면 생성 (안전장치)
if (!fs_1.default.existsSync(exports.UPLOAD_DIR))
    fs_1.default.mkdirSync(exports.UPLOAD_DIR, { recursive: true });
if (!fs_1.default.existsSync(exports.RESULT_DIR))
    fs_1.default.mkdirSync(exports.RESULT_DIR, { recursive: true });
// Multer 스토리지 설정
const storage = multer_1.default.diskStorage({
    destination: (req, file, cb) => {
        cb(null, exports.UPLOAD_DIR);
    },
    filename: (req, file, cb) => {
        // 파일명: timestamp_originalname
        const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
        cb(null, uniqueSuffix + path_1.default.extname(file.originalname));
    }
});
// 파일 필터 (이미지만 허용)
const fileFilter = (req, file, cb) => {
    const customReq = req;
    if (file.mimetype.startsWith('image/')) {
        cb(null, true);
    }
    else {
        // 에러를 던지면 ECONNRESET이 발생할 수 있으므로, 플래그를 설정하고 false 반환
        customReq.fileValidationError = 'Only image files are allowed';
        cb(null, false);
    }
};
// Multer 설정 (파일 크기 제한 5MB)
exports.upload = (0, multer_1.default)({
    storage: storage,
    fileFilter: fileFilter,
    limits: {
        fileSize: 5 * 1024 * 1024 // 5MB
    }
});
