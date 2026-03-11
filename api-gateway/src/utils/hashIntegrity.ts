/**
 * SHA-256 해시 무결성 유틸리티
 *
 * plan.md Security > 해시 무결성 (Tier 1 필수):
 * 결과 파일 저장 시 SHA-256 해시를 함께 저장하고,
 * 변조 시 캐시 매칭 실패를 감지한다.
 */
import crypto from 'node:crypto';
import fs from 'node:fs/promises';

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
 * @param filePath - 결과 파일 경로
 */
export async function saveWithHash(content: string, filePath: string): Promise<void> {
  const hash = computeHash(content);
  await fs.writeFile(filePath, content, 'utf-8');
  await fs.writeFile(filePath + '.sha256', hash, 'utf-8');
}

/**
 * 결과 파일의 무결성을 검증한다.
 * `.sha256` 파일이 없거나, 해시가 불일치하면 false를 반환한다.
 *
 * @param filePath - 검증할 결과 파일 경로
 * @returns 무결성 검증 통과 여부
 */
export async function verifyHash(filePath: string): Promise<boolean> {
  const hashPath = filePath + '.sha256';

  const hashExists = await fs.access(hashPath).then(() => true).catch(() => false);
  if (!hashExists) return false;

  const [content, savedHash] = await Promise.all([
    fs.readFile(filePath, 'utf-8'),
    fs.readFile(hashPath, 'utf-8'),
  ]);

  return computeHash(content) === savedHash.trim();
}
