"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.processAnalysis = void 0;
const node_path_1 = __importDefault(require("node:path"));
const promises_1 = __importDefault(require("node:fs/promises"));
const axios_1 = __importDefault(require("axios"));
const fileStorage_1 = require("../utils/fileStorage");
const hashIntegrity_1 = require("../utils/hashIntegrity");
const logger_1 = __importStar(require("../utils/logger"));
const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
const AI_SERVER_URL = process.env.AI_SERVER_URL || 'http://localhost:8002';
/**
 * 더미 분석 결과를 생성합니다.
 * 실제 AI 모델 연동 시 제거 또는 Mocking 용도로 사용
 */
function generateDummyResult() {
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
const processAnalysis = async (file, requestId) => {
    if (!file) {
        throw new Error('NO_FILE');
    }
    logger_1.default.info('Image uploaded:', (0, logger_1.maskPII)({ path: file.path, requestId }));
    const timestamp = Date.now();
    const resultPath = node_path_1.default.join(fileStorage_1.RESULT_DIR, `${timestamp}_result.json`);
    // [Phase 3] C++ 전처리 서버 호출
    let processedImagePath = file.path; // 기본값은 원본 이미지
    try {
        const preprocessRes = await axios_1.default.post(`${PREPROCESS_SERVER_URL}/preprocess`, {
            imagePath: file.path
        }, {
            headers: { 'X-Request-ID': requestId }
        });
        if (preprocessRes.data?.processedPath) {
            processedImagePath = preprocessRes.data.processedPath;
            logger_1.default.info('Preprocessing completed:', (0, logger_1.maskPII)({ path: processedImagePath, requestId }));
        }
    }
    catch (error) {
        logger_1.default.warn('Preprocessing failed, using original image:', {
            error: error instanceof Error ? error.message : String(error),
            requestId
        });
        // 전처리 실패 시에도 일단 원본으로 계속 진행 (또는 에러 throw 선택 가능)
        // 현재는 테스트 단계이므로 로그만 남김
    }
    // Phase 4 - Python AI 서버 호출
    let resultData;
    try {
        // 실제 파일을 읽어서 AI 서버로 전송 (multipart/form-data)
        const FormData = require('form-data');
        const formData = new FormData();
        formData.append('file', await promises_1.default.readFile(processedImagePath), node_path_1.default.basename(processedImagePath));
        const aiRes = await axios_1.default.post(`${AI_SERVER_URL}/analyze`, formData, {
            headers: {
                ...formData.getHeaders(),
                'X-Request-ID': requestId
            }
        });
        resultData = aiRes.data;
        logger_1.default.info('AI Analysis completed:', { requestId });
    }
    catch (error) {
        logger_1.default.warn('AI Analysis failed, falling back to dummy data:', {
            error: error instanceof Error ? error.message : String(error),
            requestId
        });
        // AI 서버 실패 시에만 더미 데이터 생성 (또는 에러 상황에 따라 다르게 처리 가능)
        resultData = generateDummyResult();
    }
    // 결과 JSON 파일 저장 (SHA-256 해시 함께 저장 — 무결성 검증용)
    const resultContent = JSON.stringify(resultData, null, 2);
    await (0, hashIntegrity_1.saveWithHash)(resultContent, resultPath);
    logger_1.default.info('Result saved:', (0, logger_1.maskPII)({ path: resultPath, requestId }));
    return resultData;
};
exports.processAnalysis = processAnalysis;
