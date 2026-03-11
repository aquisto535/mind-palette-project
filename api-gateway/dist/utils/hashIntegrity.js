"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.computeHash = computeHash;
exports.saveWithHash = saveWithHash;
exports.verifyHash = verifyHash;
/**
 * SHA-256 해시 무결성 유틸리티
 *
 * plan.md Security > 해시 무결성 (Tier 1 필수):
 * 결과 파일 저장 시 SHA-256 해시를 함께 저장하고,
 * 변조 시 캐시 매칭 실패를 감지한다.
 */
const node_crypto_1 = __importDefault(require("node:crypto"));
const promises_1 = __importDefault(require("node:fs/promises"));
/**
 * 문자열 내용의 SHA-256 해시를 반환한다.
 */
function computeHash(content) {
    return node_crypto_1.default.createHash('sha256').update(content, 'utf-8').digest('hex');
}
/**
 * 결과 파일을 저장하고, 동일 경로에 `.sha256` 해시 파일을 함께 저장한다.
 *
 * @param content - 저장할 JSON 문자열
 * @param filePath - 결과 파일 경로
 */
async function saveWithHash(content, filePath) {
    const hash = computeHash(content);
    await promises_1.default.writeFile(filePath, content, 'utf-8');
    await promises_1.default.writeFile(filePath + '.sha256', hash, 'utf-8');
}
/**
 * 결과 파일의 무결성을 검증한다.
 * `.sha256` 파일이 없거나, 해시가 불일치하면 false를 반환한다.
 *
 * @param filePath - 검증할 결과 파일 경로
 * @returns 무결성 검증 통과 여부
 */
async function verifyHash(filePath) {
    const hashPath = filePath + '.sha256';
    const hashExists = await promises_1.default.access(hashPath).then(() => true).catch(() => false);
    if (!hashExists)
        return false;
    const [content, savedHash] = await Promise.all([
        promises_1.default.readFile(filePath, 'utf-8'),
        promises_1.default.readFile(hashPath, 'utf-8'),
    ]);
    return computeHash(content) === savedHash.trim();
}
