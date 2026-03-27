/**
 * SHA-256 해시 무결성 유틸리티
 *
 * plan.md Security > 해시 무결성 (Tier 1 필수):
 * 결과 파일 저장 시 SHA-256 해시를 함께 저장하고,
 * 변조 시 캐시 매칭 실패를 감지한다.
 */
import crypto from 'node:crypto';
import fs from 'node:fs/promises';

import path from 'node:path';
import { RESULT_DIR } from '../utils/fileStorage';

/**
 * 문자열 내용의 SHA-256 해시를 반환한다.
 */
export function computeHash(content: string): string {
  return crypto.createHash('sha256').update(content, 'utf-8').digest('hex');
}

/**
 * 결과 파일을 저장하고, 동일 경로에 `.sha256` 해시 파일을 함께 저장한다.
 *
 * @param content - 저장할 JSON 문자열
 * @param filePath - 결과 파일 경로 (filename 추출용)
 * @param allowedDir - 허용된 저장 디렉토리 (기본값: RESULT_DIR)
 */
export async function saveWithHash(content: string, filePath: string, allowedDir: string = RESULT_DIR): Promise<void> {
  const hash = computeHash(content);
  
  // Neutralize path for CodeQL: only use the filename joined with trusted directory
  const fileName = path.basename(filePath);
  const safePath = path.resolve(allowedDir, fileName);

  await fs.writeFile(safePath, content, 'utf-8');
  await fs.writeFile(safePath + '.sha256', hash, 'utf-8');
}

/**
 * 결과 파일의 무결성을 검증한다.
 * `.sha256` 파일이 없거나, 해시가 불일치하면 false를 반환한다.
 *
 * @param filePath - 검증할 결과 파일 경로
 * @param allowedDir - 허용된 저장 디렉토리 (기본값: RESULT_DIR)
 * @returns 무결성 검증 통과 여부
 */
export async function verifyHash(filePath: string, allowedDir: string = RESULT_DIR): Promise<boolean> {
  // Neutralize path for CodeQL
  const fileName = path.basename(filePath);
  const safePath = path.resolve(allowedDir, fileName);
  const hashPath = safePath + '.sha256';

  const hashExists = await fs.access(hashPath).then(() => true).catch(() => false);
  if (!hashExists) return false;

  const [content, savedHash] = await Promise.all([
    fs.readFile(safePath, 'utf-8'),
    fs.readFile(hashPath, 'utf-8'),
  ]);

  return computeHash(content) === savedHash.trim();
}
