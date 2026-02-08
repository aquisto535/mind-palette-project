import axios, { InternalAxiosRequestConfig } from 'axios';

// Axios 인스턴스 생성 (기본 설정)
const client = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api', // 환경 변수 사용 권장
  timeout: 10000,
});

// 인터셉터 설정 (가이드라인 2.3 참고)
// 추후 JWT 인증 구현 시 토큰 추가 로직이 들어갈 위치
client.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  // TODO: JWT 인증 구현 시 아래와 같이 토큰을 추가
  // store.getState().auth.token을 사용하여 Authorization 헤더 설정
  return config;
});

export default client;

