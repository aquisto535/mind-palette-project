"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.upload = exports.RESULT_DIR = exports.UPLOAD_DIR = void 0;
exports.hasValidMagicBytes = hasValidMagicBytes;
exports.isSafeFilename = isSafeFilename;
const multer_1 = __importDefault(require("multer"));
const node_path_1 = __importDefault(require("node:path"));
const node_fs_1 = __importDefault(require("node:fs"));
// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
const SHARED_ROOT = node_path_1.default.join(__dirname, '../../../shared_volume');
exports.UPLOAD_DIR = node_path_1.default.join(SHARED_ROOT, 'uploads');
exports.RESULT_DIR = node_path_1.default.join(SHARED_ROOT, 'results');
// 폴더가 없으면 생성 (안전장치)
if (!node_fs_1.default.existsSync(exports.UPLOAD_DIR))
    node_fs_1.default.mkdirSync(exports.UPLOAD_DIR, { recursive: true });
if (!node_fs_1.default.existsSync(exports.RESULT_DIR))
    node_fs_1.default.mkdirSync(exports.RESULT_DIR, { recursive: true });
// ─────────────────────────────────────────────
// 매직 바이트 시그니처 정의
// context7 리서치 결론: PNG는 완전한 8바이트 시그니처(89 50 4E 47 0D 0A 1A 0A) 검증
// ─────────────────────────────────────────────
const MAGIC_SIGNATURES = [
    { bytes: [0xFF, 0xD8, 0xFF] }, // JPEG
    { bytes: [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A] }, // PNG (완전 8바이트)
    { bytes: [0x42, 0x4D] }, // BMP
    { bytes: [0x52, 0x49, 0x46, 0x46] }, // WebP (RIFF prefix)
];
const WEBP_MARKER = Buffer.from([0x57, 0x45, 0x42, 0x50]); // 'WEBP' at offset 8
function hasValidMagicBytes(buffer) {
    for (const sig of MAGIC_SIGNATURES) {
        const magic = Buffer.from(sig.bytes);
        if (buffer.length >= magic.length && buffer.subarray(0, magic.length).equals(magic)) {
            // WebP는 추가로 offset 8에 'WEBP' 확인
            if (sig.bytes[0] === 0x52 && sig.bytes[1] === 0x49) {
                return buffer.length >= 12 && buffer.subarray(8, 12).equals(WEBP_MARKER);
            }
            return true;
        }
    }
    return false;
}
// ─────────────────────────────────────────────
// Path Traversal 방어: 파일명 검증
// ─────────────────────────────────────────────
function isSafeFilename(filename) {
    if (filename.includes('\x00'))
        return false; // null byte
    if (/^[/\\]/.test(filename))
        return false; // 절대 경로
    if (filename.includes('..'))
        return false; // 상위 디렉토리 탐색
    return true;
}
// Multer 스토리지 설정
const storage = multer_1.default.diskStorage({
    destination: (req, file, cb) => {
        cb(null, exports.UPLOAD_DIR);
    },
    filename: (req, file, cb) => {
        const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
        // basename만 사용하여 경로 조작 방지
        const safeExt = node_path_1.default.extname(node_path_1.default.basename(file.originalname));
        cb(null, uniqueSuffix + safeExt);
    }
});
// 파일 필터 (파일명 안전성 + MIME 타입 검증)
// 매직 바이트 검증은 multer가 디스크에 저장 후 라우트 핸들러에서 수행
const fileFilter = (req, file, cb) => {
    const customReq = req;
    // 1. 파일명 Path Traversal 검증
    if (!isSafeFilename(file.originalname)) {
        customReq.fileValidationError = '유효하지 않은 파일명입니다.';
        cb(null, false);
        return;
    }
    // 2. MIME 타입 검증
    if (!file.mimetype.startsWith('image/')) {
        customReq.fileValidationError = 'Only image files are allowed';
        cb(null, false);
        return;
    }
    cb(null, true);
};
// Multer 설정 (파일 크기 제한 5MB)
exports.upload = (0, multer_1.default)({
    storage: storage,
    fileFilter: fileFilter,
    limits: {
        fileSize: 5 * 1024 * 1024 // 5MB
    }
});
