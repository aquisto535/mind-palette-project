import path from 'node:path';
import fs from 'node:fs/promises';
import axios from 'axios';
import FormData from 'form-data';
import { RESULT_DIR, UPLOAD_DIR } from '../utils/fileStorage';
import { saveWithHash } from '../utils/hashIntegrity';
import logger, { maskPII } from '../utils/logger';

const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
const AI_SERVER_URL = process.env.AI_SERVER_URL || 'http://localhost:8002';

interface AnalysisResult {
  score: number;
  percentile: number;
  date: string;
  interpretation: string;
  details: {
    creativity: number;
    expression: number;
    observational: number;
  };
}

/**
 * 더미 분석 결과를 생성합니다.
 * 실제 AI 모델 연동 시 제거 또는 Mocking 용도로 사용
 */
function generateDummyResult(): AnalysisResult {
  return {
    score: Math.floor(Math.random() * (95 - 70) + 70), // 70~95점 랜덤
    percentile: Math.floor(Math.random() * (99 - 60) + 60),
    date: new Date().toLocaleDateString(),
    interpretation: "AI 분석 결과가 여기에 표시됩니다. (현재는 Mock 데이터입니다)",
    details: {
      creativity: 85,
      expression: 90,
      observational: 88
    }
  };
}

/**
 * 이미지 분석 요청을 처리합니다.
 * @param {Express.Multer.File} file - 업로드된 파일 객체
 * @param {string} requestId - 요청 추적을 위한 고유 ID
 * @returns {Promise<AnalysisResult>} 분석 결과 객체
 */
export const processAnalysis = async (file: Express.Multer.File, requestId: string): Promise<AnalysisResult> => {
  if (!file) {
    throw new Error('NO_FILE');
  }

  logger.info('Image uploaded:', maskPII({ path: file.path, requestId }));

  const timestamp = Date.now();
  const resultPath = path.join(RESULT_DIR, `${timestamp}_result.json`);

  // [Phase 3] C++ 전처리 서버 호출
  let processedImagePath = file.path; // 기본값은 원본 이미지
  try {
    const preprocessRes = await axios.post(`${PREPROCESS_SERVER_URL}/preprocess`, {
      imagePath: file.path
    }, {
      headers: { 'X-Request-ID': requestId }
    });

    if (preprocessRes.data?.processedPath) {
      processedImagePath = preprocessRes.data.processedPath;
      logger.info('Preprocessing completed:', maskPII({ path: processedImagePath, requestId }));
    }
  } catch (error: unknown) {
    logger.warn('Preprocessing failed, using original image:', {
      error: error instanceof Error ? error.message : String(error),
      requestId
    });
    // 전처리 실패 시에도 일단 원본으로 계속 진행 (또는 에러 throw 선택 가능)
    // 현재는 테스트 단계이므로 로그만 남김
  }

  // Phase 4 - Python AI 서버 호출
  let resultData: AnalysisResult;
  try {
    // 실제 파일을 읽어서 AI 서버로 전송 (multipart/form-data)
    const formData = new FormData();
    const resolvedPath = path.resolve(processedImagePath);
    const resolvedUploadDir = path.resolve(UPLOAD_DIR);

    if (!resolvedPath.startsWith(resolvedUploadDir)) {
      throw new Error('SECURITY_PATH_VIOLATION');
    }

    formData.append('file', await fs.readFile(resolvedPath), path.basename(resolvedPath));

    const aiRes = await axios.post(`${AI_SERVER_URL}/analyze`, formData, {
      headers: {
        ...formData.getHeaders(),
        'X-Request-ID': requestId
      }
    });

    resultData = aiRes.data;
    logger.info('AI Analysis completed:', { requestId });
  } catch (error: unknown) {
    logger.warn('AI Analysis failed, falling back to dummy data:', {
      error: error instanceof Error ? error.message : String(error),
      requestId
    });
    // AI 서버 실패 시에만 더미 데이터 생성 (또는 에러 상황에 따라 다르게 처리 가능)
    resultData = generateDummyResult();
  }

  // 결과 JSON 파일 저장 (SHA-256 해시 함께 저장 — 무결성 검증용)
  const resultContent = JSON.stringify(resultData, null, 2);
  await saveWithHash(resultContent, resultPath);
  logger.info('Result saved:', maskPII({ path: resultPath, requestId }));

  return resultData;
};
