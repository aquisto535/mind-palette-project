import request from 'supertest';
import path from 'node:path';
import fs from 'node:fs';
import app from '../src/server';
import axios from 'axios';

jest.mock('axios');
const mockedAxios = axios as jest.Mocked<typeof axios>;

describe('File Upload Security', () => {
  const FIXTURES_DIR = path.join(__dirname, 'fixtures');
  const TEST_IMAGE_PATH = path.join(FIXTURES_DIR, 'test-image.png');
  const TEST_TEXT_PATH = path.join(FIXTURES_DIR, 'test.txt');
  const LARGE_FILE_PATH = path.join(FIXTURES_DIR, 'large-file.png');

  // Create dummy files for testing
  beforeAll(() => {
    if (!fs.existsSync(FIXTURES_DIR)) {
      fs.mkdirSync(FIXTURES_DIR);
    }
    // PNG 매직 바이트(89 50 4E 47)로 시작하는 최소 파일
    fs.writeFileSync(TEST_IMAGE_PATH, Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]));
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
    if (fs.existsSync(FIXTURES_DIR)) fs.rmdirSync(FIXTURES_DIR);
  });

  it('should accept valid image files (png/jpg/jpeg)', async () => {
    const PROCESSED_DIR = path.join(__dirname, '../../shared_volume/processed');
    if (!fs.existsSync(PROCESSED_DIR)) fs.mkdirSync(PROCESSED_DIR, { recursive: true });
    const MOCK_PROCESSED_FILENAME = 'test-image_clean.jpg';
    const MOCK_PROCESSED_PATH = path.join(PROCESSED_DIR, MOCK_PROCESSED_FILENAME);
    fs.writeFileSync(MOCK_PROCESSED_PATH, Buffer.from('fake-processed-data'));

    mockedAxios.post.mockImplementation((url: string) => {
      if (url.includes('/preprocess')) {
        return Promise.resolve({ data: { processedPath: MOCK_PROCESSED_PATH }, headers: {} });
      }
      return Promise.resolve({
        data: {
          iq: 100,
          percentile: 95,
          raw_score: 10,
          head_scores: { head_a: 10, head_b: 10, head_c: 10 },
          date: '2026. 3. 27.'
        },
        headers: {}
      });
    });

    await request(app)
      .post('/analyze')
      .attach('image', TEST_IMAGE_PATH)
      .expect(200);

    if (fs.existsSync(MOCK_PROCESSED_PATH)) fs.unlinkSync(MOCK_PROCESSED_PATH);
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

// ─────────────────────────────────────────────────────────────
// [TDD][RED] 입력 검증: 매직 바이트 기반 파일 형식 검증
// plan.md Security > 입력 검증 (Tier 1 필수)
// "파일 업로드 검증: .txt 파일을 .jpg로 속여 업로드 시 차단"
// ─────────────────────────────────────────────────────────────
describe('Magic Byte Validation', () => {
  const FIXTURES_DIR = path.join(__dirname, 'fixtures');

  beforeAll(() => {
    if (!fs.existsSync(FIXTURES_DIR)) {
      fs.mkdirSync(FIXTURES_DIR);
    }
  });

  afterAll(() => {
    // fixtures 내 생성된 파일들 정리
    const files = fs.readdirSync(FIXTURES_DIR);
    for (const f of files) fs.unlinkSync(path.join(FIXTURES_DIR, f));
    if (fs.existsSync(FIXTURES_DIR)) fs.rmdirSync(FIXTURES_DIR);
  });

  it('[RED] 텍스트 내용을 .jpg로 위장하여 업로드 시 400을 반환해야 한다', async () => {
    // MIME type은 image/jpeg이지만 실제 내용은 텍스트
    const response = await request(app)
      .post('/analyze')
      .attach('image', Buffer.from('I am not an image, just plain text'), {
        filename: 'fake.jpg',
        contentType: 'image/jpeg',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });

  it('[GREEN] JPEG 매직 바이트로 시작하는 파일은 Node.js 레이어에서 허용된다 (심층 파싱은 AI 서버 담당)', async () => {
    // Node.js는 매직 바이트(FF D8 FF)만 검증 — 손상 여부 판단은 Python AI 서버 레이어 책임
    const partialJpeg = Buffer.concat([
      Buffer.from([0xFF, 0xD8, 0xFF, 0xE0]),
      Buffer.alloc(10, 0x00),
    ]);
    const response = await request(app)
      .post('/analyze')
      .attach('image', partialJpeg, {
        filename: 'partial.jpg',
        contentType: 'image/jpeg',
      });

    expect(response.status).not.toBe(400);
  });

  it('[RED] 유효한 JPEG 매직 바이트와 콘텐츠를 가진 파일은 허용되어야 한다', async () => {
    // JFIF 형식의 최소 유효 JPEG (1x1 픽셀)
    // SOI + APP0(JFIF) + DQT + SOF0 + DHT + SOS + EOI
    const minimalJpeg = Buffer.from([
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
      0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
      0xFF, 0xDB, 0x00, 0x43, 0x00,
      ...new Array(64).fill(0x10),
      0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x11, 0x00,
      0xFF, 0xC4, 0x00, 0x1F, 0x00,
      0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
      0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0xF5,
      0xFF, 0xD9,
    ]);

    const response = await request(app)
      .post('/analyze')
      .attach('image', minimalJpeg, {
        filename: 'valid.jpg',
        contentType: 'image/jpeg',
      });

    // 매직 바이트/이미지 형식 검증은 통과 (200 또는 내부 처리 단계 오류)
    expect(response.status).not.toBe(400);
  });

  it('[RED] PNG 매직 바이트 없이 .png 확장자로 업로드 시 400을 반환해야 한다', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', Buffer.from('Not a PNG file at all'), {
        filename: 'fake.png',
        contentType: 'image/png',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });

  // ─────────────────────────────────────────────────────────────
  // [TDD][RED] PNG 시그니처 8바이트 완전 검증
  // context7 리서치 결론: PNG 완전 시그니처는 8바이트(89 50 4E 47 0D 0A 1A 0A)
  // 현재 구현(4바이트)으로는 앞 4바이트만 일치하는 위조 파일을 허용할 수 있음
  // ─────────────────────────────────────────────────────────────
  it('[RED] PNG 앞 4바이트만 일치하고 나머지 시그니처가 다른 파일은 400을 반환해야 한다', async () => {
    // PNG 시그니처: 89 50 4E 47 0D 0A 1A 0A (8바이트)
    // 앞 4바이트(89 50 4E 47)만 같고 나머지는 다른 위조 데이터
    const fakePartialPng = Buffer.from([
      0x89, 0x50, 0x4E, 0x47,  // PNG 앞 4바이트 일치
      0xFF, 0xFF, 0xFF, 0xFF,  // 나머지 4바이트 불일치 (올바른 값: 0D 0A 1A 0A)
      ...Buffer.alloc(100, 0xAA),
    ]);

    const response = await request(app)
      .post('/analyze')
      .attach('image', fakePartialPng, {
        filename: 'partial-sig.png',
        contentType: 'image/png',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });

  it('[GREEN] 완전한 PNG 8바이트 시그니처를 가진 파일은 허용되어야 한다', async () => {
    // 완전한 PNG 시그니처 8바이트
    const validPngSignature = Buffer.from([
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
      ...Buffer.alloc(20, 0x00),
    ]);

    const response = await request(app)
      .post('/analyze')
      .attach('image', validPngSignature, {
        filename: 'valid-sig.png',
        contentType: 'image/png',
      });

    // 매직 바이트 검증은 통과해야 함 (200 또는 내부 처리 오류, 400은 아님)
    expect(response.status).not.toBe(400);
  });
});

// ─────────────────────────────────────────────────────────────
// [TDD][RED] 입력 검증: Path Traversal 공격 차단
// plan.md Security > 경로 정규화
// "../../etc/passwd와 같은 Path Traversal 공격 시 차단"
// ─────────────────────────────────────────────────────────────
describe('Path Traversal Prevention', () => {
  it('[RED] ../../ 가 포함된 파일명으로 업로드 시 400을 반환해야 한다', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', Buffer.from('content'), {
        filename: '../../etc/passwd',
        contentType: 'image/jpeg',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });

  it('[RED] null byte(%00)가 포함된 파일명으로 업로드 시 400을 반환해야 한다', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', Buffer.from('content'), {
        filename: 'test\x00.jpg',
        contentType: 'image/jpeg',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });

  it('[RED] 절대 경로로 시작하는 파일명으로 업로드 시 400을 반환해야 한다', async () => {
    const response = await request(app)
      .post('/analyze')
      .attach('image', Buffer.from('content'), {
        filename: '/etc/passwd',
        contentType: 'image/jpeg',
      })
      .expect(400);

    expect(response.body).toHaveProperty('error');
  });
});
