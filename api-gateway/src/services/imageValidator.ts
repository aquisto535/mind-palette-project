import fs from 'node:fs/promises';
import { getSafePath } from '../utils/pathValidator';
import logger from '../utils/logger';
import { hasValidMagicBytes, checkImageDimensions, UPLOAD_DIR } from '../utils/fileStorage';

export interface ValidationResult {
  valid: boolean;
  error?: string;
}

export class ImageValidator {
  /**
   * Performs all security and integrity checks for an uploaded image.
   * L2: Magic Byte (Header-only)
   * L5: Resolution Limit (Pixel Flood protection)
   * Path Injection protection
   */
  static async validate(file: Express.Multer.File): Promise<ValidationResult> {
    try {
      const filePath = getSafePath(file.path, UPLOAD_DIR);

      // 2. L2: Magic Byte Validation (Header-only 12 bytes)
      const fd = await fs.open(filePath, 'r');
      const headerBuffer = Buffer.allocUnsafe(12);
      try {
        await fd.read(headerBuffer, 0, 12, 0);
      } finally {
        await fd.close();
      }

      if (!hasValidMagicBytes(headerBuffer)) {
        await fs.unlink(filePath).catch(() => undefined);
        return { valid: false, error: '파일 내용이 올바른 이미지 형식이 아닙니다.' };
      }

      // 3. L5: Resolution Limit (Pixel Flood protection)
      const dimCheck = await checkImageDimensions(filePath);
      if (!dimCheck.valid) {
        await fs.unlink(filePath).catch(() => undefined);
        return { valid: false, error: dimCheck.error };
      }

      return { valid: true };
    } catch (error) {
      logger.error('Validator Error:', { error: error instanceof Error ? error.message : String(error) });
      return { valid: false, error: 'Internal validation error' };
    }
  }
}
