import axios from 'axios';
import FormData from 'form-data';
import { randomUUID } from 'node:crypto';

// ─────────────────────────────────────────────────────────────
// TrafficBot: API 서버 부하 테스트 트래픽 생성 도구
// ─────────────────────────────────────────────────────────────

export interface TrafficBotConfig {
  targetUrl: string;
  endpoint: string;
  intervalMs: number;
  totalRequests: number;
  concurrency: number;
  profileKey?: string;  // X-Admin-Profile-Key 값. 설정 시 Server-Timing 수집 활성화 (ADR-026)
}

export interface TrafficBotResult {
  totalSent: number;
  successCount: number;
  failureCount: number;
  avgResponseMs: number;
  p95ResponseMs: number;
  errors: { status: number; count: number }[];
  avgTimings?: {
    gateway?: number;       // API Gateway 처리 시간 평균 (ms)
    preprocess?: number;    // C++ 전처리 서버 처리 시간 평균 (ms)
    aiInference?: number;   // Python AI 추론 시간 평균 (ms)
  };
}

const DEFAULT_CONFIG: Omit<TrafficBotConfig, 'profileKey'> = {
  targetUrl: 'http://localhost:3000',
  endpoint: '/analyze',
  intervalMs: 1000,
  totalRequests: 100,
  concurrency: 1,
};

export class TrafficBot {
  private readonly config: TrafficBotConfig;

  constructor(partialConfig?: Partial<TrafficBotConfig>) {
    this.config = { ...DEFAULT_CONFIG, ...partialConfig };
  }

  getConfig(): TrafficBotConfig {
    return { ...this.config };
  }

  /**
   * P95 응답시간 계산
   * 응답시간 배열 정렬 후 Math.ceil(arr.length * 0.95) - 1 인덱스 값 반환
   */
  calculateP95(responseTimes: number[]): number {
    if (responseTimes.length === 0) return 0;

    const sorted = [...responseTimes].sort((a, b) => a - b);
    const index = Math.ceil(sorted.length * 0.95) - 1;
    return sorted[index] ?? 0;
  }

  /**
   * Server-Timing 헤더 파싱
   * "gateway;dur=12.3, preprocess;dur=18.7" → { gateway: 12.3, preprocess: 18.7 }
   */
  private parseServerTiming(header: string): Record<string, number> {
    const result: Record<string, number> = {};
    const pattern = /^(\S+?);dur=([\d.]+)/;
    for (const part of header.split(',')) {
      const match = pattern.exec(part.trim());
      if (match?.[1] !== undefined && match?.[2] !== undefined) {
        result[match[1]] = Number.parseFloat(match[2]);
      }
    }
    return result;
  }

  /**
   * 최소 유효 JPEG 이미지 버퍼 생성 (1×1 픽셀, JFIF baseline, 표준 Huffman 테이블)
   * OpenCV imread 및 Python AI 추론 파이프라인 통과 가능
   */
  private createDummyJpeg(): Buffer {
    return Buffer.from(
      // SOI
      'ffd8' +
      // APP0 (JFIF marker)
      'ffe000104a46494600010100000100010000' +
      // DQT (모든 양자화 계수 = 1, 64 bytes)
      'ffdb004300' +
      '0101010101010101010101010101010101010101010101010101010101010101' +
      '0101010101010101010101010101010101010101010101010101010101010101' +
      // SOF0 (1×1 grayscale)
      'ffc0000b080001000101011100' +
      // DHT DC table 0 (표준 JPEG Annex K)
      'ffc4001f00' +
      '00010501010101010100000000000000' +
      '000102030405060708090a0b' +
      // DHT AC table 0 (표준 JPEG Annex K, 162 values)
      'ffc400b510' +
      '0002010303020403050504040000017d' +
      '0102030004110512213141061351610722711432' +
      '8191a1082342b1c11552d1f0243362728209' +
      '0a161718191a25262728292a343536373839' +
      '3a434445464748494a535455565758595a' +
      '636465666768696a737475767778797a' +
      '838485868788898a9293949596979899' +
      '9aa2a3a4a5a6a7a8a9aab2b3b4b5b6b7b8b9ba' +
      'c2c3c4c5c6c7c8c9cad2d3d4d5d6d7d8d9da' +
      'e1e2e3e4e5e6e7e8e9eaf1f2f3f4f5f6f7f8f9fa' +
      // SOS header (1 component, DC/AC table 0)
      'ffda0008010100003f00' +
      // Scan data: DC=0 (2-bit code 00) + AC EOB (4-bit code 1010) → 0x2B
      '2b' +
      // EOI
      'ffd9',
      'hex',
    );
  }

  /**
   * 단일 요청 전송
   * multipart/form-data 형식으로 더미 JPEG 이미지 전송
   */
  private async sendRequest(): Promise<{ success: boolean; responseMs: number; status: number; serverTiming?: string; errorMessage?: string }> {
    const startTime = Date.now();
    const url = `${this.config.targetUrl}${this.config.endpoint}`;
    const requestId = randomUUID();

    try {
      const form = new FormData();
      const dummyImage = this.createDummyJpeg();
      form.append('image', dummyImage, {
        filename: 'test.jpg',
        contentType: 'image/jpeg',
      });

      const headers: Record<string, string> = {
        ...form.getHeaders(),
        'X-Request-ID': requestId,
      };
      if (this.config.profileKey) {
        headers['X-Admin-Profile-Key'] = this.config.profileKey;
      }

      const response = await axios.post(url, form, {
        headers,
        validateStatus: () => true, // 모든 상태 코드를 정상으로 처리
      });

      const responseMs = Date.now() - startTime;
      const success = response.status >= 200 && response.status < 400;
      const serverTiming = this.config.profileKey
        ? (response.headers['server-timing'] as string | undefined)
        : undefined;

      return { success, responseMs, status: response.status, serverTiming };
    } catch (error: unknown) {
      const responseMs = Date.now() - startTime;
      // 네트워크 오류(연결 거부, 타임아웃 등) — status 0으로 실패 처리
      const message = error instanceof Error ? error.message : String(error);
      return { success: false, responseMs, status: 0, errorMessage: message };
    }
  }

  /**
   * TrafficBot 실행
   * totalRequests 수만큼 요청을 전송하고 결과를 집계
   */
  async run(): Promise<TrafficBotResult> {
    const responseTimes: number[] = [];
    let successCount = 0;
    let failureCount = 0;
    const errorMap = new Map<number, number>();

    // Server-Timing 집계용 배열 (profileKey 설정 시에만 사용)
    const gatewayTimes: number[] = [];
    const preprocessTimes: number[] = [];
    const aiInferenceTimes: number[] = [];

    const sendBatch = async (batchSize: number): Promise<void> => {
      const promises: Promise<void>[] = [];

      for (let i = 0; i < batchSize; i++) {
        const promise = this.sendRequest().then(({ success, responseMs, status, serverTiming }) => {
          responseTimes.push(responseMs);

          if (success) {
            successCount++;
          } else {
            failureCount++;
            const prevCount = errorMap.get(status) || 0;
            errorMap.set(status, prevCount + 1);
          }

          if (serverTiming) {
            const timings = this.parseServerTiming(serverTiming);
            if (timings['gateway'] !== undefined) gatewayTimes.push(timings['gateway']);
            if (timings['preprocess'] !== undefined) preprocessTimes.push(timings['preprocess']);
            if (timings['ai_inference'] !== undefined) aiInferenceTimes.push(timings['ai_inference']);
          }
        });
        promises.push(promise);
      }

      await Promise.all(promises);
    };

    let remaining = this.config.totalRequests;

    while (remaining > 0) {
      const batchSize = Math.min(this.config.concurrency, remaining);
      await sendBatch(batchSize);
      remaining -= batchSize;

      if (remaining > 0 && this.config.intervalMs > 0) {
        await new Promise(resolve => setTimeout(resolve, this.config.intervalMs));
      }
    }

    const totalSent = successCount + failureCount;
    const avgResponseMs = totalSent > 0
      ? Math.round(responseTimes.reduce((sum, t) => sum + t, 0) / totalSent)
      : 0;
    const p95ResponseMs = this.calculateP95(responseTimes);

    const errors = Array.from(errorMap.entries()).map(([status, count]) => ({ status, count }));

    const avgOf = (arr: number[]): number | undefined =>
      arr.length > 0 ? Number.parseFloat((arr.reduce((s, v) => s + v, 0) / arr.length).toFixed(1)) : undefined;

    const avgTimings = this.config.profileKey ? {
      gateway: avgOf(gatewayTimes),
      preprocess: avgOf(preprocessTimes),
      aiInference: avgOf(aiInferenceTimes),
    } : undefined;

    return {
      totalSent,
      successCount,
      failureCount,
      avgResponseMs,
      p95ResponseMs,
      errors,
      avgTimings,
    };
  }
}
