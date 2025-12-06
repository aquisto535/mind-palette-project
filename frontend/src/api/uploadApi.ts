import client from './client';
import { AnalysisResult, ChildInfo } from '../types';

// Mock 데이터 생성 함수 (개발용)
const generateMockResult = (childName: string): AnalysisResult => {
  const mockScore = Math.floor(Math.random() * (95 - 70) + 70);
  return {
    score: mockScore,
    percentile: Math.floor(Math.random() * (99 - 60) + 60),
    date: new Date().toLocaleDateString(),
    interpretation: `${childName} 어린이는 그림을 통해 풍부한 상상력을 표현하고 있습니다.\n\n` +
      `선이 굵고 힘이 있는 것으로 보아 자신감이 넘치고 에너지가 많은 성향으로 보입니다. ` +
      `세부적인 묘사가 ${mockScore > 80 ? '매우 뛰어나며' : '잘 나타나 있으며'}, 이는 관찰력이 우수함을 의미합니다.\n\n` +
      `전체적인 그림의 크기가 종이에 꽉 차는 것은 외향적이고 적극적인 성격을 나타낼 수 있습니다. ` +
      `또래 친구들에 비해 인지적 표현력이 상위권에 속하는 것으로 분석됩니다.`
  };
};

// 이미지 업로드 및 분석 요청 API
export const uploadImage = async (file: File, childInfo: ChildInfo | null): Promise<AnalysisResult> => {
  // Phase 2: 실제 서버 연동 시 사용할 코드
  /*
  const formData = new FormData();
  formData.append('image', file);
  if (childInfo) {
    formData.append('childInfo', JSON.stringify(childInfo));
  }

  const response = await client.post<AnalysisResult>('/analyze', formData, {
    headers: { 'Content-Type': 'multipart/form-data' },
  });
  return response.data;
  */

  // Phase 1: Mock 응답 (3초 딜레이 시뮬레이션)
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(generateMockResult(childInfo?.name || '아이'));
    }, 3000);
  });
};

