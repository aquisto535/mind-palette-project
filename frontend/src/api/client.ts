import axios, { InternalAxiosRequestConfig } from 'axios';
import { AnalysisError } from '../types/errors';

const client = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api',
  timeout: 10000,
});

client.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  const token = localStorage.getItem('token');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

const ERROR_MAP: Record<number, { message: string; actionType: AnalysisError['actionType'] }> = {
  400: { message: '파일이 올바른 이미지 형식이 아닙니다. 다시 선택해주세요.', actionType: 'retry-upload' },
  422: { message: '연필로 그린 전신 인물화를 올려주세요.', actionType: 'retry-guide' },
  429: { message: '요청이 너무 많습니다. 잠시 후 다시 시도해주세요.', actionType: 'retry-later' },
  503: { message: '서버가 바쁩니다. 잠시 후 다시 시도해주세요.', actionType: 'retry-later' },
};

client.interceptors.response.use(
  (response) => response,
  (error) => {
    const status: number = error.response?.status ?? 0;
    if (status === 401) localStorage.removeItem('token');
    const mapped = ERROR_MAP[status] ?? {
      message: '분석 중 오류가 발생했습니다. 잠시 후 다시 시도해주세요.',
      actionType: 'retry-later' as AnalysisError['actionType'],
    };
    return Promise.reject(new AnalysisError(status, mapped.message, mapped.actionType));
  }
);

export default client;

