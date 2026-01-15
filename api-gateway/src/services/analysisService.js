const path = require('path');
const fs = require('fs');
const axios = require('axios');
const { RESULT_DIR } = require('../utils/fileStorage');

const PREPROCESS_SERVER_URL = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';

/**
 * 더미 분석 결과를 생성합니다.
 * TODO: 실제 AI 모델 연동 시 제거 또는 Mocking 용도로 사용
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
 * @param {Object} file - 업로드된 파일 객체
 * @returns {Object} 분석 결과 객체
 */
exports.processAnalysis = async (file) => {
  if (!file) {
    throw new Error('NO_FILE');
  }

  console.log('Image uploaded:', file.path);

  const timestamp = Date.now();
  const resultPath = path.join(RESULT_DIR, `${timestamp}_result.json`);

  // [Phase 3] C++ 전처리 서버 호출
  let processedImagePath = file.path; // 기본값은 원본 이미지
  try {
    const preprocessRes = await axios.post(`${PREPROCESS_SERVER_URL}/preprocess`, {
      imagePath: file.path
    });
    
    if (preprocessRes.data && preprocessRes.data.processedPath) {
      processedImagePath = preprocessRes.data.processedPath;
      console.log('Preprocessing completed:', processedImagePath);
    }
  } catch (error) {
    console.warn('Preprocessing failed, using original image:', error.message);
    // 전처리 실패 시에도 일단 원본으로 계속 진행 (또는 에러 throw 선택 가능)
    // 현재는 테스트 단계이므로 로그만 남김
  }

  // TODO: Phase 4 - Python AI 서버 호출
  
  // [Phase 2] 임시 더미 데이터 생성
  const resultData = generateDummyResult();

  // 결과 JSON 파일 저장
  fs.writeFileSync(resultPath, JSON.stringify(resultData, null, 2));
  console.log('Result saved:', resultPath);

  return resultData;
};

