// ─────────────────────────────────────────────────────────────
// [TDD][RED] 저장/무결성: SHA-256 해시 무결성 검증
// plan.md Security > 해시 무결성 (Tier 1 필수)
// "결과 파일 변조 시 캐시 매칭 실패 및 재분석 트리거 테스트"
// ─────────────────────────────────────────────────────────────
import path from 'node:path';
import fs from 'node:fs/promises';
import os from 'node:os';
import { computeHash, saveWithHash, verifyHash } from '../src/utils/hashIntegrity';

describe('Hash Integrity', () => {
  let tmpDir: string;

  beforeAll(async () => {
    tmpDir = await fs.mkdtemp(path.join(os.tmpdir(), 'hash-test-'));
  });

  afterAll(async () => {
    await fs.rm(tmpDir, { recursive: true, force: true });
  });

  // ─────────────────────────────────────────────
  // computeHash
  // ─────────────────────────────────────────────
  it('[RED] 동일한 내용은 항상 동일한 SHA-256 해시를 반환한다', () => {
    const content = JSON.stringify({ score: 85, percentile: 90 });
    const hash1 = computeHash(content);
    const hash2 = computeHash(content);

    expect(hash1).toBe(hash2);
    expect(hash1).toMatch(/^[a-f0-9]{64}$/); // SHA-256: 64자 hex
  });

  it('[RED] 내용이 1바이트라도 달라지면 해시가 달라진다', () => {
    const original = JSON.stringify({ score: 85 });
    const tampered = JSON.stringify({ score: 86 });

    expect(computeHash(original)).not.toBe(computeHash(tampered));
  });

  // ─────────────────────────────────────────────
  // saveWithHash
  // ─────────────────────────────────────────────
  it('[RED] saveWithHash는 결과 파일과 함께 .sha256 해시 파일을 생성한다', async () => {
    const resultPath = path.join(tmpDir, 'result.json');
    const content = JSON.stringify({ score: 85, percentile: 90 });

    await saveWithHash(content, resultPath, tmpDir);

    const hashPath = resultPath + '.sha256';
    const hashExists = await fs.access(hashPath).then(() => true).catch(() => false);
    expect(hashExists).toBe(true);

    const savedHash = await fs.readFile(hashPath, 'utf-8');
    expect(savedHash.trim()).toMatch(/^[a-f0-9]{64}$/);
  });

  it('[RED] saveWithHash가 저장한 .sha256 해시는 내용의 SHA-256과 일치한다', async () => {
    const resultPath = path.join(tmpDir, 'result2.json');
    const content = JSON.stringify({ score: 75 });

    await saveWithHash(content, resultPath, tmpDir);

    const hashPath = resultPath + '.sha256';
    const savedHash = (await fs.readFile(hashPath, 'utf-8')).trim();
    expect(savedHash).toBe(computeHash(content));
  });

  // ─────────────────────────────────────────────
  // verifyHash (핵심: 변조 탐지)
  // ─────────────────────────────────────────────
  it('[RED] 변조되지 않은 파일은 verifyHash가 true를 반환한다', async () => {
    const resultPath = path.join(tmpDir, 'valid.json');
    const content = JSON.stringify({ score: 88 });
    await saveWithHash(content, resultPath, tmpDir);

    const isValid = await verifyHash(resultPath, tmpDir);
    expect(isValid).toBe(true);
  });

  it('[RED] 결과 파일 변조 시 verifyHash가 false를 반환한다 (캐시 매칭 실패)', async () => {
    const resultPath = path.join(tmpDir, 'tampered.json');
    const original = JSON.stringify({ score: 88 });
    await saveWithHash(original, resultPath, tmpDir);

    // 파일 변조 — 해시 파일은 그대로, 결과 파일만 수정
    await fs.writeFile(resultPath, JSON.stringify({ score: 99 }));

    const isValid = await verifyHash(resultPath, tmpDir);
    expect(isValid).toBe(false);
  });

  it('[RED] .sha256 해시 파일이 없으면 verifyHash가 false를 반환한다', async () => {
    const resultPath = path.join(tmpDir, 'no-hash.json');
    await fs.writeFile(resultPath, JSON.stringify({ score: 70 }));
    // .sha256 파일 없음

    const isValid = await verifyHash(resultPath, tmpDir);
    expect(isValid).toBe(false);
  });
});
