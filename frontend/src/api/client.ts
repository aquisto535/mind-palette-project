import axios, { InternalAxiosRequestConfig } from 'axios';

// Axios 인스턴스 생성 (기본 설정)
const client = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api', // 환경 변수 사용 권장
  timeout: 10000,
});

// 인터셉터 설정 (가이드라인 2.3 참고)
client.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  // localStorage에서 토큰을 가져와 Authorization 헤더에 추가
  const token = localStorage.getItem('token');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// 응답 인터셉터 추가 (ADR-033 비연필 이미지 예외 처리 대응)
client.interceptors.response.use(
  (response) => response,
  (error) => {
    // 422 Unprocessable Entity: ADR-033 (비연필/컬러 이미지 감지 등)
    if (error.response?.status === 422) {
      console.error('Validation Error (ADR-033):', error.response.data);
      // 알림창이나 UI 상의 에러 처리를 위한 커스텀 에러 객체 반환 가능
    }

    // 401 Unauthorized: 토큰 만료 등
    if (error.response?.status === 401) {
      localStorage.removeItem('token');
    }

    return Promise.reject(error);
  }
);

export default client;

