import multer, { FileFilterCallback } from 'multer';
import path from 'node:path';
import fs from 'node:fs';
import { Request } from 'express';
import { imageSize } from 'image-size';

// Extend Request to include fileValidationError
export interface CustomRequest extends Request {
  fileValidationError?: string;
}

// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
const SHARED_ROOT = path.join(__dirname, '../../../shared_volume');
export const UPLOAD_DIR = path.join(SHARED_ROOT, 'uploads');
export const PROCESSED_DIR = path.join(SHARED_ROOT, 'processed');
export const RESULT_DIR = path.join(SHARED_ROOT, 'results');

// 폴더가 없으면 생성 (안전장치)
if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
if (!fs.existsSync(PROCESSED_DIR)) fs.mkdirSync(PROCESSED_DIR, { recursive: true });
if (!fs.existsSync(RESULT_DIR)) fs.mkdirSync(RESULT_DIR, { recursive: true });

interface MagicSignature {
  name: string;
  bytes: number[];
  offset?: number;
  extraCheck?: (buffer: Buffer) => boolean;
}

const MAGIC_SIGNATURES: MagicSignature[] = [
  { name: 'JPEG', bytes: [0xFF, 0xD8, 0xFF] },
  { name: 'PNG',  bytes: [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A] },
  { name: 'BMP',  bytes: [0x42, 0x4D] },
  { 
    name: 'WebP', 
    bytes: [0x52, 0x49, 0x46, 0x46], // 'RIFF'
    extraCheck: (buffer) => buffer.length >= 12 && buffer.subarray(8, 12).equals(Buffer.from([0x57, 0x45, 0x42, 0x50])) // 'WEBP'
  },
];

export function hasValidMagicBytes(buffer: Buffer): boolean {
  return MAGIC_SIGNATURES.some(sig => {
    const magic = Buffer.from(sig.bytes);
    const offset = sig.offset || 0;
    
    // 기본 시그니처 매칭
    const matches = buffer.length >= offset + magic.length && 
                   buffer.subarray(offset, offset + magic.length).equals(magic);
    
    if (!matches) return false;
    
    // 추가 검증 로직이 있으면 실행
    return sig.extraCheck ? sig.extraCheck(buffer) : true;
  });
}

// ─────────────────────────────────────────────
// Path Traversal 방어: 파일명 검증
// ─────────────────────────────────────────────
export function isSafeFilename(filename: string): boolean {
  if (filename.includes('\x00')) return false;       // null byte
  if (/^[/\\]/.test(filename)) return false;         // 절대 경로
  if (filename.includes('..')) return false;          // 상위 디렉토리 탐색
  return true;
}

// ─────────────────────────────────────────────
// L5 강화: 이미지 해상도 제한 (Pixel Flood 방어)
// ─────────────────────────────────────────────
const MAX_IMAGE_DIMENSION = 4096;

export interface DimensionCheckResult {
  valid: boolean;
  width?: number;
  height?: number;
  error?: string;
}

// 헬퍼: 이미지 크기 확인
export async function checkImageDimensions(filePath: string): Promise<DimensionCheckResult> {
  try {
    // Neutralize path for CodeQL: only use the filename joined with trusted UPLOAD_DIR
    const fileName = path.basename(filePath);
    const safePath = path.resolve(UPLOAD_DIR, fileName);
    const fileBuffer = await fs.promises.readFile(safePath);
    const dimensions = imageSize(fileBuffer);
    const width = dimensions.width ?? 0;
    const height = dimensions.height ?? 0;

    if (width > MAX_IMAGE_DIMENSION || height > MAX_IMAGE_DIMENSION) {
      return {
        valid: false,
        width,
        height,
        error: `Image dimensions exceed limit (${width}x${height}, max ${MAX_IMAGE_DIMENSION}x${MAX_IMAGE_DIMENSION})`,
      };
    }

    return { valid: true, width, height };
  } catch {
    // 메타데이터 파싱 실패 시 통과 (Magic Byte 검증을 이미 통과한 상태)
    return { valid: true };
  }
}

// Multer 스토리지 설정
const storage = multer.diskStorage({
  destination: (req: Request, file: Express.Multer.File, cb: (error: Error | null, destination: string) => void) => {
    cb(null, UPLOAD_DIR);
  },
  filename: (req: Request, file: Express.Multer.File, cb: (error: Error | null, filename: string) => void) => {
    const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
    // basename만 사용하여 경로 조작 방지
    const safeExt = path.extname(path.basename(file.originalname));
    cb(null, uniqueSuffix + safeExt);
  }
});

// 파일 필터 (파일명 안전성 + MIME 타입 검증)
// 매직 바이트 검증은 multer가 디스크에 저장 후 라우트 핸들러에서 수행
const fileFilter = (req: Request, file: Express.Multer.File, cb: FileFilterCallback) => {
  const customReq = req as CustomRequest;

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
export const upload = multer({
  storage: storage,
  fileFilter: fileFilter,
  limits: {
    fileSize: 5 * 1024 * 1024 // 5MB
  }
});
