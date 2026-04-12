import path from 'node:path';
import fs from 'node:fs/promises';
import crypto from 'node:crypto';
import axios from 'axios';
import FormData from 'form-data';
import { RESULT_DIR, UPLOAD_DIR, PROCESSED_DIR } from '../utils/fileStorage';
import { saveWithHash } from '../utils/hashIntegrity';
import logger, { maskPII } from '../utils/logger';
import { analysisCache } from './cacheService';

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

async function invokePreprocessServer(filePath: string, requestId: string, adminProfileKey?: string): Promise<{ processedImagePath: string; sanitized: boolean, serverTiming?: string }> {
  let processedImagePath = filePath;
  let sanitized = false;
  let serverTiming: string | undefined;
  try {
    const headers: Record<string, string> = { 'X-Request-ID': requestId };
    if (adminProfileKey) headers['X-Admin-Profile-Key'] = adminProfileKey;

    const preprocessRes = await axios.post(`${PREPROCESS_SERVER_URL}/preprocess`, { imagePath: filePath }, { headers });
    if (preprocessRes.data?.processedPath) {
      processedImagePath = preprocessRes.data.processedPath;
      sanitized = true;
      logger.info('Preprocessing completed (L6 sanitized):', maskPII({ path: processedImagePath, requestId }));
    } else {
      logger.warn('L6 Sanitization skipped: preprocessing did not return processedPath', { requestId });
    }
    serverTiming = preprocessRes.headers['server-timing'];
  } catch (error: unknown) {
    // ADR-033 Tier 1: 422는 Fail-Fast — 컬러/비연필 이미지 감지 시 즉시 전파
    if (axios.isAxiosError(error) && error.response?.status === 422) {
      throw error;
    }
    // 이중 안전망: Crow가 422→500으로 변환한 경우에도 body의 error 필드로 감지
    if (
      axios.isAxiosError(error) &&
      (error.response?.status === 400 || error.response?.status === 500) &&
      error.response?.data?.error === 'COLOR_VALIDATION_FAILED'
    ) {
      error.response.status = 422;
      throw error;
    }
    logger.warn('L6 Sanitization skipped: preprocessing failed, using original image:', {
      error: error instanceof Error ? error.message : String(error),
      requestId
    });
  }
  return { processedImagePath, sanitized, serverTiming };
}

async function invokeAiServer(processedImagePath: string, sanitized: boolean, requestId: string, adminProfileKey?: string): Promise<AnalysisResult & { serverTiming?: string }> {
  const formData = new FormData();

  // Neutralize path for CodeQL: resolve filename against the appropriate trusted directory.
  // If sanitized, the file is in PROCESSED_DIR; otherwise fall back to UPLOAD_DIR.
  const fileName = path.basename(processedImagePath);
  const baseDir = sanitized ? PROCESSED_DIR : UPLOAD_DIR;
  const safePath = path.resolve(baseDir, fileName);

  formData.append('file', await fs.readFile(safePath), fileName);

  const headers: Record<string, string> = { ...formData.getHeaders(), 'X-Request-ID': requestId };
  if (adminProfileKey) headers['X-Admin-Profile-Key'] = adminProfileKey;

  const aiRes = await axios.post(`${AI_SERVER_URL}/analyze`, formData, { headers });

  const resultData = mapAiResponseToResult(aiRes.data as AiServerResponse);
  logger.info('AI Analysis completed:', { requestId });
  
  return { ...resultData, serverTiming: aiRes.headers['server-timing'] };
}

async function cleanupTempImages(originalPath: string, processedPath: string, requestId: string): Promise<void> {
  const keepImages = process.env.KEEP_IMAGES === 'true';
  if (keepImages) return;

  const validateAndUnlink = async (targetPath: string, baseDir: string) => {
    try {
      // Neutralize path for CodeQL: only use the filename joined with trusted base directory
      const fileName = path.basename(targetPath);
      const safePath = path.resolve(baseDir, fileName);
      
      await fs.unlink(safePath).catch(e => { 
        if (e.code !== 'ENOENT') {
          logger.error('File unlink failed:', { path: targetPath, error: e.message, requestId });
        }
      });
    } catch (e) {
      logger.warn('Cleanup blocked or invalid path:', { 
        path: targetPath, 
        requestId,
        error: e instanceof Error ? e.message : String(e)
      });
    }
  };

  try {
    await validateAndUnlink(originalPath, UPLOAD_DIR);
    if (processedPath !== originalPath) {
      await validateAndUnlink(processedPath, PROCESSED_DIR);
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
 * @param {string} [clientProfileKey] - 벤치마킹을 위한 X-Admin-Profile-Key 
 * @returns {Promise<AnalysisResult>} 분석 결과 객체
 */
export const processAnalysis = async (file: Express.Multer.File, requestId: string, clientProfileKey?: string): Promise<AnalysisResult & { sanitized: boolean; serverTiming?: string }> => {
  const gatewayStartTime = performance.now();

  if (!file) {
    throw new Error('NO_FILE');
  }

  // 캐시 히트 조회 — 동일 파일 내용이면 전처리/추론 건너뜀 (< 10ms 목표)
  // CodeQL 보안 경보 방지: path.basename과 UPLOAD_DIR을 결합하여 경로 정규화 (Neutralize)
  const fileName = path.basename(file.path);
  const safePath = path.resolve(UPLOAD_DIR, fileName);
  const imageBuffer = await fs.readFile(safePath);
  const imageHash = crypto.createHash('sha256').update(imageBuffer).digest('hex');
  const cached = analysisCache.get(imageHash);
  if (cached !== undefined) {
    logger.info('Cache hit:', { hash: imageHash.slice(0, 8), requestId });
    // 캐시 히트 시에도 Multer가 생성한 임시 파일 정리는 필요함 (P1 이슈 해결)
    await cleanupTempImages(file.path, file.path, requestId);
    return cached as AnalysisResult & { sanitized: boolean; serverTiming?: string };
  }

  logger.info('Image uploaded:', maskPII({ path: file.path, requestId }));

  const timestamp = Date.now();
  const resultPath = path.join(RESULT_DIR, `${timestamp}_result.json`);

  let sanitized = false;
  let processedImagePath = file.path;
  
  const envAdminKey = process.env.ADMIN_PROFILE_KEY;
  const isProfileMode = Boolean(envAdminKey && clientProfileKey === envAdminKey);
  const passKey = isProfileMode ? envAdminKey : undefined;

  try {
    const preprocessResult = await invokePreprocessServer(file.path, requestId, passKey);
    processedImagePath = preprocessResult.processedImagePath;
    sanitized = preprocessResult.sanitized;

    const resultData = await invokeAiServer(processedImagePath, sanitized, requestId, passKey);

    const { serverTiming: aiTiming, ...finalResultData } = resultData;

    const resultContent = JSON.stringify(finalResultData, null, 2);
    await saveWithHash(resultContent, resultPath);
    logger.info('Result saved:', maskPII({ path: resultPath, requestId }));

    // 캐시 저장 (serverTiming은 제외 — 캐시 히트 시 재사용 불가)
    analysisCache.set(imageHash, { ...finalResultData, sanitized });

    let serverTiming: string | undefined;
    if (isProfileMode) {
      const gatewayDur = (performance.now() - gatewayStartTime).toFixed(1);
      const parts = [`gateway;dur=${gatewayDur}`];
      if (preprocessResult.serverTiming) parts.push(preprocessResult.serverTiming);
      if (aiTiming) parts.push(aiTiming);
      serverTiming = parts.join(', ');
    }

    return { ...finalResultData, sanitized, serverTiming };
  } finally {
    await cleanupTempImages(file.path, processedImagePath, requestId);
  }
};
