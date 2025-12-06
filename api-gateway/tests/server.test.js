const request = require('supertest');
const app = require('../server');
const path = require('path');
const fs = require('fs');

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
    
    // 테스트 전후 정리
    beforeAll(() => {
      if (!fs.existsSync(TEST_UPLOAD_DIR)) {
        fs.mkdirSync(TEST_UPLOAD_DIR, { recursive: true });
      }
    });

    it('should return 400 if no file is uploaded', async () => {
      const res = await request(app).post('/analyze');
      expect(res.statusCode).toEqual(400);
      expect(res.body).toHaveProperty('error', 'No image file uploaded');
    });

    // Note: 실제 파일 업로드 테스트는 jest 환경 설정이 필요하므로 
    // 여기서는 기본적인 에러 처리와 라우트 연결만 확인합니다.
    // 실제 구현 시에는 mock-fs 등을 활용하거나 실제 파일을 생성하여 테스트합니다.
  });
});

