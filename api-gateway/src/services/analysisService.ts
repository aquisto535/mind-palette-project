import path from 'node:path';
import fs from 'node:fs/promises';
import axios from 'axios';
import FormData from 'form-data';
import { RESULT_DIR, UPLOAD_DIR } from '../utils/fileStorage';
import { saveWithHash } from '../utils/hashIntegrity';
import logger, { maskPII } from '../utils/logger';

const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
// AI 서버 기본 포트는 ai-server/src/config.py :: ServerConfig.port = 8082 와 일치
const AI_SERVER_URL = process.env.AI_SERVER_URL || 'http://localhost:8082';

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
  }

  // Phase 4 - Python AI 서버 호출
  let resultData: AnalysisResult;
  try {
    const formData = new FormData();
    const resolvedPath = path.resolve(processedImagePath);
    // ─────────────────────────────────────────────────────────
    // P2 Fix: 트레일링 path.sep을 추가하여 sibling prefix bypass 방어
    //   예: resolvedUploadDir = /foo/uploads
    //       /foo/uploads_malicious/ → startsWith(/foo/uploads/) → false ✅
    // ─────────────────────────────────────────────────────────
    const resolvedUploadDir = path.resolve(UPLOAD_DIR) + path.sep;

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

    resultData = mapAiResponseToResult(aiRes.data as AiServerResponse);
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
