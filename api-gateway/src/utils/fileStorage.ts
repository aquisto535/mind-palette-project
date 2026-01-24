import multer, { FileFilterCallback } from 'multer';
import path from 'path';
import fs from 'fs';
import { Request } from 'express';

// Extend Request to include fileValidationError
export interface CustomRequest extends Request {
  fileValidationError?: string;
}

// 저장소 경로 설정 (프로젝트 루트의 shared_volume 사용)
const SHARED_ROOT = path.join(__dirname, '../../../shared_volume');
export const UPLOAD_DIR = path.join(SHARED_ROOT, 'uploads');
export const RESULT_DIR = path.join(SHARED_ROOT, 'results');

// 폴더가 없으면 생성 (안전장치)
if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
if (!fs.existsSync(RESULT_DIR)) fs.mkdirSync(RESULT_DIR, { recursive: true });

// Multer 스토리지 설정
const storage = multer.diskStorage({
  destination: (req: Request, file: Express.Multer.File, cb: (error: Error | null, destination: string) => void) => {
    cb(null, UPLOAD_DIR);
  },
  filename: (req: Request, file: Express.Multer.File, cb: (error: Error | null, filename: string) => void) => {
    // 파일명: timestamp_originalname
    const uniqueSuffix = Date.now() + '_' + Math.round(Math.random() * 1E9);
    cb(null, uniqueSuffix + path.extname(file.originalname));
  }
});

// 파일 필터 (이미지만 허용)
const fileFilter = (req: Request, file: Express.Multer.File, cb: FileFilterCallback) => {
  const customReq = req as CustomRequest;
  if (file.mimetype.startsWith('image/')) {
    cb(null, true);
  } else {
    // 에러를 던지면 ECONNRESET이 발생할 수 있으므로, 플래그를 설정하고 false 반환
    customReq.fileValidationError = 'Only image files are allowed';
    cb(null, false);
  }
};

// Multer 설정 (파일 크기 제한 5MB)
export const upload = multer({
  storage: storage,
  fileFilter: fileFilter,
  limits: {
    fileSize: 5 * 1024 * 1024 // 5MB
  }
});
