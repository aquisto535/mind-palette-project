const request = require('supertest');
const path = require('path');
const fs = require('fs');
const app = require('../server');
const { UPLOAD_DIR } = require('../src/utils/fileStorage');

// Mock fileStorage to use a temporary directory for tests
// Note: We are testing the integration with multer and express, so we use the real file storage
// but we'll clean up afterwards.
// However, to mock fileFilter behavior specifically if we were unit testing, we might mock multer.
// But for integration test, let's use the real app.

describe('File Upload Security', () => {
  const TEST_IMAGE_PATH = path.join(__dirname, 'fixtures', 'test-image.png');
  const TEST_TEXT_PATH = path.join(__dirname, 'fixtures', 'test.txt');
  const LARGE_FILE_PATH = path.join(__dirname, 'fixtures', 'large-file.png');

  // Create dummy files for testing
  beforeAll(() => {
    if (!fs.existsSync(path.join(__dirname, 'fixtures'))) {
      fs.mkdirSync(path.join(__dirname, 'fixtures'));
    }
    // Create a valid PNG file (with PNG magic bytes if needed, but for now simple content)
    // Multer's default fileFilter usually checks mimetype based on extension or content-type header
    // But for stricter security, we might need magic bytes check.
    // For this TDD, we start with standard checks.
    fs.writeFileSync(TEST_IMAGE_PATH, 'fake-png-content');
    fs.writeFileSync(TEST_TEXT_PATH, 'fake-text-content');
    
    // Create a large file (> 5MB)
    const largeBuffer = Buffer.alloc(6 * 1024 * 1024); // 6MB
    fs.writeFileSync(LARGE_FILE_PATH, largeBuffer);
  });

  afterAll(() => {
    // Cleanup fixtures
    if (fs.existsSync(TEST_IMAGE_PATH)) fs.unlinkSync(TEST_IMAGE_PATH);
    if (fs.existsSync(TEST_TEXT_PATH)) fs.unlinkSync(TEST_TEXT_PATH);
    if (fs.existsSync(LARGE_FILE_PATH)) fs.unlinkSync(LARGE_FILE_PATH);
    if (fs.existsSync(path.join(__dirname, 'fixtures'))) fs.rmdirSync(path.join(__dirname, 'fixtures'));
  });

  it('should accept valid image files (png/jpg/jpeg)', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', TEST_IMAGE_PATH)
      .expect(200);

    // Clean up uploaded file
    // In a real scenario, we might want to check if the file was actually saved
  });

  it('should reject non-image files', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', TEST_TEXT_PATH)
      .expect(400);

    expect(response.body).toHaveProperty('error');
    expect(response.body.error).toMatch(/Only image files are allowed/i);
  });

  it('should reject files larger than limit (e.g. 5MB)', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', LARGE_FILE_PATH)
      .expect(400);

    expect(response.body).toHaveProperty('error');
    // Multer's default error for file size limit
    expect(response.body.error).toMatch(/File too large/i);
  });
});
