const request = require('supertest');
const path = require('path');
const fs = require('fs');
const app = require('../server'); // Express 앱 가져오기

// 통합 테스트: 실제 파일 업로드 시나리오 검증
describe('Integration Test: File Upload Flow', () => {
  const TEST_UPLOAD_DIR = path.join(__dirname, '../../shared_volume/uploads');
  const TEST_RESULT_DIR = path.join(__dirname, '../../shared_volume/results');
  const DUMMY_IMAGE_PATH = path.join(__dirname, 'test-image.jpg');

  // 테스트 전: 더미 이미지 생성
  beforeAll(() => {
    // 테스트용 0바이트 더미 이미지 생성 (없으면)
    if (!fs.existsSync(DUMMY_IMAGE_PATH)) {
      fs.writeFileSync(DUMMY_IMAGE_PATH, 'dummy image content');
    }
    
    // 업로드/결과 디렉토리 확인
    if (!fs.existsSync(TEST_UPLOAD_DIR)) fs.mkdirSync(TEST_UPLOAD_DIR, { recursive: true });
    if (!fs.existsSync(TEST_RESULT_DIR)) fs.mkdirSync(TEST_RESULT_DIR, { recursive: true });
  });

  // 테스트 후: 더미 이미지 정리
  afterAll(() => {
    if (fs.existsSync(DUMMY_IMAGE_PATH)) {
      fs.unlinkSync(DUMMY_IMAGE_PATH);
    }
  });

  test('POST /analyze - Should upload file and return analysis result', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', DUMMY_IMAGE_PATH) // 파일 첨부 시뮬레이션
      .expect(200)
      .expect('Content-Type', /json/);

    // 1. 응답 데이터 구조 검증
    expect(response.body).toHaveProperty('score');
    expect(response.body).toHaveProperty('interpretation');
    
    // 2. 파일 저장 여부 검증 (uploads 폴더에 파일이 생겼는지)
    const uploadedFiles = fs.readdirSync(TEST_UPLOAD_DIR);
    // 방금 업로드한 파일이 있어야 함 (파일명은 timestamp 포함이라 정확히 매칭은 어렵지만 개수로 확인)
    expect(uploadedFiles.length).toBeGreaterThan(0);

    // 3. 결과 저장 여부 검증 (results 폴더에 JSON이 생겼는지)
    const resultFiles = fs.readdirSync(TEST_RESULT_DIR);
    expect(resultFiles.length).toBeGreaterThan(0);
  });
});

