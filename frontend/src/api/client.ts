import axios from 'axios';

// Axios 인스턴스 생성 (기본 설정)
const client = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api', // 환경 변수 사용 권장
  timeout: 10000,
});

// 인터셉터 설정 (가이드라인 2.3 참고)
// 추후 JWT 인증 구현 시 토큰 추가 로직이 들어갈 위치
client.interceptors.request.use((config) => {
  // const token = store.getState().auth.token;
  // if (token) config.headers.Authorization = `Bearer ${token}`;
  return config;
});

export default client;

