import request from 'supertest';
import app from '../src/server';

// ─────────────────────────────────────────────────────────
// [L3] 제약 상수 (Magic Number 제거 → Named Constants)
// ─────────────────────────────────────────────────────────
const ANALYZE_ENDPOINT = '/analyze';
const FILE_LIMIT_MB = 5;
const OVER_LIMIT_MB = FILE_LIMIT_MB + 1;

// ─────────────────────────────────────────────────────────
// 픽스처 팩토리 (Extract Method → 더미 파일 생성 로직 분리)
// ─────────────────────────────────────────────────────────
const makeJpegBuffer = (sizeMb: number): Buffer =>
  Buffer.alloc(sizeMb * 1024 * 1024);

const makeCorruptedBuffer = (): Buffer =>
  Buffer.from('이것은이미지가아닙니다');

const postAnalyze = (file: Buffer, filename: string) =>
  request(app).post(ANALYZE_ENDPOINT).attach('image', file, filename);

// ─────────────────────────────────────────────────────────
// L3 테스트: 파일 크기 제약 (Size Constraints)
// ─────────────────────────────────────────────────────────
describe('Analysis API [L3] Security & Constraint Tests', () => {
  describe(`POST ${ANALYZE_ENDPOINT}`, () => {
    describe('파일 크기 제약 (File Size Constraints)', () => {
      /**
       * [L3] 설정된 제한(5MB)을 초과하는 요청 시 400 에러를 반환해야 함
       * multer fileSize 설정 기준
       */
      it('should reject files larger than 5MB with 400 Bad Request', async () => {
        const response = await postAnalyze(makeJpegBuffer(OVER_LIMIT_MB), 'too_large.jpg');

        expect(response.status).toBe(400);
        expect(response.body.error).toMatch(/too large/i);
      });
    });

    describe('파일 콘텐츠 무결성 (Content Integrity)', () => {
      /**
       * [L3] 이미지가 아닌 파일이나 손상된 헤더를 가진 파일 업로드 시 400 에러 반환
       * Magic Bytes 기반 검증 (MIME 위조 대응)
       */
      it('should reject files with invalid magic bytes (not a real image)', async () => {
        const response = await postAnalyze(makeCorruptedBuffer(), 'fake.jpg');

        expect(response.status).toBe(400);
        expect(response.body.error).toMatch(/형식이 아닙니다/);
      });
    });

    describe('필수 입력 검증 (Required Field Validation)', () => {
      /**
       * [L1/L3] 파일 없이 multipart 요청 시 400 에러 반환
       */
      it('should return 400 when the image field is missing from multipart request', async () => {
        const response = await request(app)
          .post(ANALYZE_ENDPOINT)
          .field('title', 'missing file test');

        expect(response.status).toBe(400);
        expect(response.body.error).toMatch(/No image file uploaded/i);
      });
    });
  });
});
