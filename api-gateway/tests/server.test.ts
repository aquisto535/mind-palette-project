import request from 'supertest';
import app from '../src/server';
import path from 'path';
import fs from 'fs';

describe('API Gateway Tests', () => {

  // 기본 라우트 테스트
  describe('GET /', () => {
    it('should return 200 and welcome message', async () => {
      const res = await request(app).get('/');
      expect(res.statusCode).toEqual(200);
      expect(res.text).toContain('Mind Palette API Gateway is running');
    });
  });

  // 분석 요청 테스트
  describe('POST /analyze', () => {
    const TEST_UPLOAD_DIR = path.join(__dirname, '../../shared_volume/uploads');
    const DUMMY_IMAGE_PATH = path.join(__dirname, 'test-image.jpg');

    // 테스트 전: 업로드 폴더 및 더미 이미지 준비
    beforeAll(() => {
      if (!fs.existsSync(TEST_UPLOAD_DIR)) {
        fs.mkdirSync(TEST_UPLOAD_DIR, { recursive: true });
      }
      // PNG 매직 바이트(89 50 4E 47)로 시작하는 최소 픽스처
      if (!fs.existsSync(DUMMY_IMAGE_PATH)) {
        fs.writeFileSync(DUMMY_IMAGE_PATH, Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]));
      }
    });

    // 테스트 후: 더미 이미지 정리
    afterAll(() => {
      if (fs.existsSync(DUMMY_IMAGE_PATH)) {
        fs.unlinkSync(DUMMY_IMAGE_PATH);
      }
    });

    it('should return 400 if no file is uploaded', async () => {
      const res = await request(app).post('/analyze');
      expect(res.statusCode).toEqual(400);
      expect(res.body).toHaveProperty('error', 'No image file uploaded');
    });

    it('should return 200 and analysis result when valid file is uploaded', async () => {
      // 1. 기존 파일 목록 확인 (비교용)
      let initialUploads: string[] = [];
      if (fs.existsSync(TEST_UPLOAD_DIR)) {
        initialUploads = fs.readdirSync(TEST_UPLOAD_DIR);
      }
      const TEST_RESULT_DIR = path.join(__dirname, '../../shared_volume/results');
      if (!fs.existsSync(TEST_RESULT_DIR)) fs.mkdirSync(TEST_RESULT_DIR, { recursive: true });
      let initialResults = fs.readdirSync(TEST_RESULT_DIR);

      // 2. 요청 수행
      const res = await request(app)
        .post('/analyze')
        .attach('image', DUMMY_IMAGE_PATH);

      // 3. 응답 검증
      expect(res.statusCode).toEqual(200);
      expect(res.body).toHaveProperty('score');
      expect(res.body).toHaveProperty('interpretation');
      expect(res.body).toHaveProperty('details');

      // 4. 파일 저장 검증 (Uploads)
      const finalUploads = fs.readdirSync(TEST_UPLOAD_DIR);
      expect(finalUploads.length).toBeGreaterThan(initialUploads.length);

      // 5. 결과 저장 검증 (Results)
      const finalResults = fs.readdirSync(TEST_RESULT_DIR);
      expect(finalResults.length).toBeGreaterThan(initialResults.length);
    });
  });
});
