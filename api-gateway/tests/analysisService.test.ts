import nock from 'nock';
import path from 'path';
import * as analysisService from '../src/services/analysisService';
import { UPLOAD_DIR } from '../src/utils/fileStorage';

describe('Analysis Service', () => {
  const mockFile = {
    path: path.join(UPLOAD_DIR, 'dummy.jpg'),
    originalname: 'dummy.jpg'
  } as any;

  const PREPROCESS_SERVER_URL = 'http://localhost:8081';

  beforeAll(() => {
    // 테스트용 더미 파일 경로가 실제 존재하지 않아도 서비스 로직상 파일 읽기를 수행하지 않으면 상관없음
    // 하지만 fileStorage 관련 로직이 있다면 주의
  });

  afterEach(() => {
    nock.cleanAll();
  });

  test('should call C++ preprocess server and handle success response', async () => {
    const processedPath = '/shared/processed/dummy_clean.jpg';

    // Nock 설정: C++ 서버 요청 가로채기
    const scope = nock(PREPROCESS_SERVER_URL)
      .post('/preprocess', {
        imagePath: mockFile.path
      })
      .reply(200, {
        processedPath: processedPath
      });

    const result = await analysisService.processAnalysis(mockFile, 'test-request-id');

    // Nock이 인터셉트했는지 확인 (즉, 요청이 실제로 발송되었는지)
    expect(scope.isDone()).toBe(true);

    // 결과값은 더미 데이터지만 정상 반환되어야 함
    expect(result).toHaveProperty('score');
  });

  test('should handle C++ server failure gracefully', async () => {
    // Nock 설정: C++ 서버 에러 응답
    const scope = nock(PREPROCESS_SERVER_URL)
      .post('/preprocess')
      .reply(500, { error: 'Internal Server Error' });

    // 에러가 발생해도 throw되지 않고 결과가 반환되어야 함 (Graceful degradation)
    const result = await analysisService.processAnalysis(mockFile, 'test-request-id');

    expect(scope.isDone()).toBe(true);
    expect(result).toHaveProperty('score');
  });
});
