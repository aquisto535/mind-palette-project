import fs from 'node:fs/promises';
import path from 'node:path';
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
      // Neutralize path for CodeQL: only use the filename joined with trusted UPLOAD_DIR
      const fileName = path.basename(file.path);
      const filePath = path.resolve(UPLOAD_DIR, fileName);

      // 2. L2: Magic Byte Validation (Header-only 12 bytes)
      const fd = await fs.open(filePath, 'r');
      const headerBuffer = Buffer.alloc(12); // Use zero-initialized buffer
      try {
        const { bytesRead } = await fd.read(headerBuffer, 0, 12, 0);
        const actualHeader = headerBuffer.subarray(0, bytesRead);
        
        if (!hasValidMagicBytes(actualHeader)) {
          await fd.close(); // Close before early return
          await fs.unlink(filePath).catch(() => undefined);
          return { valid: false, error: '파일 내용이 올바른 이미지 형식이 아닙니다.' };
        }
      } finally {
        await fd.close();
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
