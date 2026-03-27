import path from 'node:path';
import fs from 'node:fs/promises';
import axios from 'axios';
import FormData from 'form-data';
import { RESULT_DIR, UPLOAD_DIR } from '../utils/fileStorage';
import { saveWithHash } from '../utils/hashIntegrity';
import logger, { maskPII } from '../utils/logger';

const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://127.0.0.1:8081';
// AI 서버 기본 포트는 ai-server/src/config.py :: ServerConfig.port = 8082 와 일치
const AI_SERVER_URL = process.env.AI_SERVER_URL || 'http://127.0.0.1:8082';

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
 * Python AI 서버 응답 형태 (ai-server/src/routes/analyze.py)
 * Gateway의 AnalysisResult 계약과 다르므로 명시적으로 매핑 필요
 */
interface AiServerResponse {
  iq: number | null;
  percentile: number | null;
  raw_score: number;
  items: Record<string, number>;
  head_scores: Record<string, number>;
  date: string;
  child_info: {
    age: number;
    child_gender: string;
    figure_gender: string;
  };
}

/**
 * AI 서버 응답(AiServerResponse)을 Gateway의 계약(AnalysisResult)으로 변환합니다.
 * - iq → score (IQ 점수)
 * - head_scores의 헤드별 점수 → details 하위 필드로 매핑
 * - null iq인 경우 raw_score로 대체 (fallback)
 */
function mapAiResponseToResult(ai: AiServerResponse): AnalysisResult {
  const score = ai.iq ?? ai.raw_score;
  // head_scores 키: 'head_a'(머리/얼굴=창의성), 'head_b'(몸통=표현), 'head_c'(사지=관찰)
  const maxHeadScore = (key: string, max: number) =>
    Math.round(((ai.head_scores?.[key] ?? 0) / max) * 100);

  return {
    score,
    percentile: ai.percentile ?? 50,
    date: ai.date ?? new Date().toLocaleDateString(),
    interpretation: `HFD 검사 결과 IQ ${score}점 (백분위 ${ai.percentile ?? '?'}%)`,
    details: {
      creativity:    maxHeadScore('head_a', 19), // 머리/얼굴 (19문항)
      expression:    maxHeadScore('head_b', 14), // 몸통/연결 (14문항)
      observational: maxHeadScore('head_c', 16), // 사지/말단 (16문항)
    },
  };
}

async function invokePreprocessServer(filePath: string, requestId: string): Promise<{ processedImagePath: string; sanitized: boolean }> {
  let processedImagePath = filePath;
  let sanitized = false;
  try {
    const preprocessRes = await axios.post(`${PREPROCESS_SERVER_URL}/preprocess`, { imagePath: filePath }, { headers: { 'X-Request-ID': requestId } });
    if (preprocessRes.data?.processedPath) {
      processedImagePath = preprocessRes.data.processedPath;
      sanitized = true;
      logger.info('Preprocessing completed (L6 sanitized):', maskPII({ path: processedImagePath, requestId }));
    } else {
      logger.warn('L6 Sanitization skipped: preprocessing did not return processedPath', { requestId });
    }
  } catch (error: unknown) {
    logger.warn('L6 Sanitization skipped: preprocessing failed, using original image:', {
      error: error instanceof Error ? error.message : String(error),
      requestId
    });
  }
  return { processedImagePath, sanitized };
}

async function invokeAiServer(processedImagePath: string, requestId: string): Promise<AnalysisResult> {
  const formData = new FormData();
  const resolvedPath = path.resolve(processedImagePath);
  const resolvedUploadDir = path.resolve(UPLOAD_DIR);

  // Windows case-insensitivity handling
  const isWindows = process.platform === 'win32';
  const checkPath = isWindows ? resolvedPath.toLowerCase() : resolvedPath;
  const checkUploadDir = (isWindows ? resolvedUploadDir.toLowerCase() : resolvedUploadDir) + path.sep;

  if (!checkPath.startsWith(checkUploadDir)) {
    logger.error('Security Alert: AI server path violation', { 
      path: processedImagePath, 
      resolvedPath, 
      expectedDir: resolvedUploadDir 
    });
    throw new Error('SECURITY_PATH_VIOLATION');
  }

  formData.append('file', await fs.readFile(resolvedPath), path.basename(resolvedPath));

  const aiRes = await axios.post(`${AI_SERVER_URL}/analyze`, formData, {
    headers: { ...formData.getHeaders(), 'X-Request-ID': requestId }
  });

  const resultData = mapAiResponseToResult(aiRes.data as AiServerResponse);
  logger.info('AI Analysis completed:', { requestId });
  return resultData;
}

async function cleanupTempImages(originalPath: string, processedPath: string, requestId: string): Promise<void> {
  const keepImages = process.env.KEEP_IMAGES === 'true';
  if (keepImages) return;

  const resolvedUploadDir = path.resolve(UPLOAD_DIR);
  const isWindows = process.platform === 'win32';
  const checkUploadDir = (isWindows ? resolvedUploadDir.toLowerCase() : resolvedUploadDir) + path.sep;

  const validateAndUnlink = async (targetPath: string) => {
    const resolvedPath = path.resolve(targetPath);
    const checkPath = isWindows ? resolvedPath.toLowerCase() : resolvedPath;

    if (!checkPath.startsWith(checkUploadDir)) {
      logger.warn('Cleanup blocked: path outside upload directory', { path: targetPath, requestId });
      return;
    }

    await fs.unlink(resolvedPath).catch(e => { 
      if (e.code !== 'ENOENT') {
        logger.error('File unlink failed:', { path: targetPath, error: e.message, requestId });
      }
    });
  };

  try {
    await validateAndUnlink(originalPath);
    if (processedPath !== originalPath) {
      await validateAndUnlink(processedPath);
    }
    logger.info('Cleaned up temp image files', { requestId });
  } catch (cleanupError) {
    logger.error('Failed to clean up temp image files', { 
      error: cleanupError instanceof Error ? cleanupError.message : String(cleanupError), 
      requestId 
    });
  }
}

/**
 * 이미지 분석 요청을 처리합니다.
 * @param {Express.Multer.File} file - 업로드된 파일 객체
 * @param {string} requestId - 요청 추적을 위한 고유 ID
 * @returns {Promise<AnalysisResult>} 분석 결과 객체
 */
export const processAnalysis = async (file: Express.Multer.File, requestId: string): Promise<AnalysisResult & { sanitized: boolean }> => {
  if (!file) {
    throw new Error('NO_FILE');
  }

  logger.info('Image uploaded:', maskPII({ path: file.path, requestId }));

  const timestamp = Date.now();
  const resultPath = path.join(RESULT_DIR, `${timestamp}_result.json`);

  let sanitized = false;
  let processedImagePath = file.path;

  try {
    const preprocessResult = await invokePreprocessServer(file.path, requestId);
    processedImagePath = preprocessResult.processedImagePath;
    sanitized = preprocessResult.sanitized;

    const resultData = await invokeAiServer(processedImagePath, requestId);

    const resultContent = JSON.stringify(resultData, null, 2);
    await saveWithHash(resultContent, resultPath);
    logger.info('Result saved:', maskPII({ path: resultPath, requestId }));

    return { ...resultData, sanitized };
  } finally {
    await cleanupTempImages(file.path, processedImagePath, requestId);
  }
};
