/**
 * Mind Palette 트래픽 봇 (Axios 기반)
 *
 * 사용법:
 *   npx ts-node scripts/traffic-bot.ts [options]
 *
 * 옵션:
 *   --url        타겟 URL (기본: http://localhost:3000/api/analyze)
 *   --images     테스트 이미지 디렉토리 (기본: ./scripts/test-images)
 *   --concurrency 동시 요청 수 (기본: 10)
 *   --total      총 요청 수 (기본: 100)
 */

import axios from 'axios';
import fs from 'node:fs';
import path from 'node:path';
import FormData from 'form-data';

// ─────────────────────────────────────────────────────────────────────────────
// CLI 인자 파싱
// ─────────────────────────────────────────────────────────────────────────────
function getArg(flag: string, defaultValue: string): string {
  const idx = process.argv.indexOf(flag);
  return idx !== -1 && process.argv[idx + 1] ? process.argv[idx + 1] : defaultValue;
}

const TARGET_URL    = getArg('--url',         'http://localhost:3000/api/analyze');
const IMAGES_DIR    = getArg('--images',      path.join(__dirname, 'test-images'));
const CONCURRENCY   = parseInt(getArg('--concurrency', '10'), 10);
const TOTAL         = parseInt(getArg('--total',       '100'), 10);

// ─────────────────────────────────────────────────────────────────────────────
// 이미지 파일 목록
// ─────────────────────────────────────────────────────────────────────────────
function loadImagePaths(): string[] {
  if (!fs.existsSync(IMAGES_DIR)) {
    console.error(`[ERROR] 이미지 디렉토리를 찾을 수 없음: ${IMAGES_DIR}`);
    process.exit(1);
  }
  const files = fs.readdirSync(IMAGES_DIR)
    .filter(f => /\.(jpg|jpeg|png)$/i.test(f))
    .map(f => path.join(IMAGES_DIR, f));

  if (files.length === 0) {
    console.error(`[ERROR] ${IMAGES_DIR} 에 jpg/png 파일이 없음`);
    process.exit(1);
  }
  return files;
}

// ─────────────────────────────────────────────────────────────────────────────
// 단일 요청
// ─────────────────────────────────────────────────────────────────────────────
async function sendRequest(imagePath: string): Promise<{ ms: number; success: boolean }> {
  const start = Date.now();
  try {
    const form = new FormData();
    form.append('image', fs.createReadStream(imagePath));
    form.append('childInfo', JSON.stringify({ age: 7, gender: 'male' }));

    await axios.post(TARGET_URL, form, {
      headers: form.getHeaders(),
      timeout: 30_000,
    });
    return { ms: Date.now() - start, success: true };
  } catch {
    return { ms: Date.now() - start, success: false };
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 배치 실행 (concurrency 단위)
// ─────────────────────────────────────────────────────────────────────────────
async function runBatch(images: string[], batchSize: number, count: number): Promise<{ ms: number; success: boolean }[]> {
  const results: { ms: number; success: boolean }[] = [];

  for (let i = 0; i < count; i += batchSize) {
    const batch = Array.from({ length: Math.min(batchSize, count - i) }, () => {
      const img = images[Math.floor(Math.random() * images.length)];
      return sendRequest(img);
    });
    const batchResults = await Promise.all(batch);
    results.push(...batchResults);
    process.stdout.write(`\r진행: ${results.length}/${count}`);
  }
  console.log();
  return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// 통계 출력
// ─────────────────────────────────────────────────────────────────────────────
function printStats(results: { ms: number; success: boolean }[]): void {
  const sorted = results.map(r => r.ms).sort((a, b) => a - b);
  const total = results.length;
  const succeeded = results.filter(r => r.success).length;
  const p50 = sorted[Math.floor(total * 0.50)];
  const p95 = sorted[Math.floor(total * 0.95)];
  const avg = Math.round(sorted.reduce((a, b) => a + b, 0) / total);

  console.log('\n── 결과 ─────────────────────────────────');
  console.log(`  총 요청:   ${total}`);
  console.log(`  성공:      ${succeeded} (${((succeeded / total) * 100).toFixed(1)}%)`);
  console.log(`  평균:      ${avg}ms`);
  console.log(`  P50:       ${p50}ms`);
  console.log(`  P95:       ${p95}ms`);
  console.log(`  최소:      ${sorted[0]}ms`);
  console.log(`  최대:      ${sorted[total - 1]}ms`);
  console.log('────────────────────────────────────────');
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
(async () => {
  console.log(`[트래픽 봇] URL=${TARGET_URL} / concurrency=${CONCURRENCY} / total=${TOTAL}`);
  const images = loadImagePaths();
  console.log(`[트래픽 봇] 이미지 ${images.length}개 로드됨`);

  const results = await runBatch(images, CONCURRENCY, TOTAL);
  printStats(results);
})();
