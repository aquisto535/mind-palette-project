import nock from 'nock';
import path from 'node:path';
import fs from 'node:fs';
import * as analysisService from '../src/services/analysisService';
import { UPLOAD_DIR, PROCESSED_DIR } from '../src/utils/fileStorage';

describe('Analysis Service', () => {
  const dummyFilename = 'dummy.jpg';
  const dummyProcessedFilename = 'dummy_clean.jpg';
  const mockFile = {
    path: path.join(UPLOAD_DIR, dummyFilename),
    originalname: dummyFilename
  } as any;

  const PREPROCESS_SERVER_URL = 'http://127.0.0.1:8081';
  const AI_SERVER_URL = 'http://127.0.0.1:8082';

  beforeEach(() => {
    if (!fs.existsSync(UPLOAD_DIR)) fs.mkdirSync(UPLOAD_DIR, { recursive: true });
    if (!fs.existsSync(PROCESSED_DIR)) fs.mkdirSync(PROCESSED_DIR, { recursive: true });
    // 실제 파일 읽기 로직이 있으므로 파일이 존재해야 함
    fs.writeFileSync(mockFile.path, Buffer.from('fake-image-data'));
    fs.writeFileSync(path.join(PROCESSED_DIR, dummyProcessedFilename), Buffer.from('fake-processed-data'));
  });

  afterAll(() => {
    if (fs.existsSync(mockFile.path)) fs.unlinkSync(mockFile.path);
    const pPath = path.join(PROCESSED_DIR, dummyProcessedFilename);
    if (fs.existsSync(pPath)) fs.unlinkSync(pPath);
  });

  afterEach(() => {
    nock.cleanAll();
  });

  test('should call C++ preprocess server and handle success response', async () => {
    const processedPath = path.join(UPLOAD_DIR, dummyProcessedFilename);

    nock(PREPROCESS_SERVER_URL)
      .post('/preprocess')
      .reply(200, { processedPath });

    nock(AI_SERVER_URL)
      .post('/analyze')
      .reply(200, {
        iq: 100,
        percentile: 95,
        raw_score: 10,
        head_scores: { head_a: 10, head_b: 10, head_c: 10 },
        date: '2026. 3. 27.'
      });

    const result = await analysisService.processAnalysis(mockFile, 'test-request-id');
    expect(result).toHaveProperty('score');
    expect(result.score).toBe(100);
  });

  test('should handle C++ server failure gracefully', async () => {
    nock(PREPROCESS_SERVER_URL)
      .post('/preprocess')
      .reply(500, { error: 'Internal Server Error' });

    await expect(
      analysisService.processAnalysis(mockFile, 'test-request-id')
    ).rejects.toMatchObject({
      name: 'PreprocessServiceError',
      message: 'PREPROCESS_SERVICE_UNAVAILABLE',
    });
  });

  test('should propagate preprocess 422 without falling back to original image', async () => {
    nock(PREPROCESS_SERVER_URL)
      .post('/preprocess')
      .reply(422, { error: 'COLOR_VALIDATION_FAILED' });

    await expect(
      analysisService.processAnalysis(mockFile, 'test-request-id')
    ).rejects.toMatchObject({
      response: {
        status: 422,
        data: { error: 'COLOR_VALIDATION_FAILED' },
      },
    });
  });
});
