import request from 'supertest';
import path from 'node:path';
import fs from 'node:fs';
import nock from 'nock';
import app from '../src/server';
import { TrafficBot, TrafficBotConfig, TrafficBotResult } from '../src/tools/trafficBot';

// ─────────────────────────────────────────────────────────────
// Traffic Bot Tests
// Phase 3-4: 트래픽 생성 도구 TDD
// ─────────────────────────────────────────────────────────────

describe('TrafficBot: Core Behavior', () => {
  const TEST_LOG_DIR = path.join(__dirname, 'tmp-logs');
  let originalLogDir: string | undefined;

  beforeEach(() => {
    // 임시 로그 디렉토리 격리
    if (!fs.existsSync(TEST_LOG_DIR)) {
      fs.mkdirSync(TEST_LOG_DIR, { recursive: true });
    }
    // nock 초기화
    nock.cleanAll();
  });

  afterEach(() => {
    // 임시 로그 정리
    if (fs.existsSync(TEST_LOG_DIR)) {
      const files = fs.readdirSync(TEST_LOG_DIR);
      for (const f of files) {
        fs.unlinkSync(path.join(TEST_LOG_DIR, f));
      }
      fs.rmdirSync(TEST_LOG_DIR);
    }
    nock.cleanAll();
    nock.enableNetConnect();
  });

  // ─────────────────────────────────────────────────────────────
  // L1: 데이터 구조 / 초기화 검증
  // ─────────────────────────────────────────────────────────────
  test('should_initialize_with_default_config_when_no_config_provided', () => {
    const bot = new TrafficBot();
    const config = bot.getConfig();

    expect(config.targetUrl).toBe('http://localhost:3000');
    expect(config.endpoint).toBe('/analyze');
    expect(config.intervalMs).toBe(1000);
    expect(config.totalRequests).toBe(100);
    expect(config.concurrency).toBe(1);
  });

  test('should_override_defaults_when_config_provided', () => {
    const customConfig: Partial<TrafficBotConfig> = {
      targetUrl: 'http://localhost:9999',
      totalRequests: 5,
      intervalMs: 0,
    };
    const bot = new TrafficBot(customConfig);
    const config = bot.getConfig();

    expect(config.targetUrl).toBe('http://localhost:9999');
    expect(config.totalRequests).toBe(5);
    expect(config.intervalMs).toBe(0);
    // 덮어쓰지 않은 값은 기본값 유지
    expect(config.endpoint).toBe('/analyze');
    expect(config.concurrency).toBe(1);
  });

  // ─────────────────────────────────────────────────────────────
  // L2: 변환 로직 — 요청 수 집계
  // ─────────────────────────────────────────────────────────────
  test('should_send_correct_number_of_requests_when_totalRequests_is_N', async () => {
    const N = 3;

    // nock으로 외부 서버 모킹 (analyze 엔드포인트)
    nock('http://localhost:3000')
      .post('/analyze')
      .times(N)
      .reply(200, {
        score: 100,
        interpretation: 'Normal'
      });

    const bot = new TrafficBot({
      targetUrl: 'http://localhost:3000',
      endpoint: '/analyze',
      totalRequests: N,
      intervalMs: 0,
      concurrency: 1,
    });

    const result: TrafficBotResult = await bot.run();

    expect(result.totalSent).toBe(N);
  }, 15000);

  // ─────────────────────────────────────────────────────────────
  // L2: 변환 로직 — 성공/실패 카운트 집계
  // ─────────────────────────────────────────────────────────────
  test('should_count_success_and_failure_correctly_when_mixed_responses', async () => {
    // 2번은 성공(200), 1번은 실패(500)
    nock('http://localhost:3000')
      .post('/analyze')
      .reply(200, { score: 100, interpretation: 'Normal' })
      .post('/analyze')
      .reply(500, { error: 'Internal Server Error' })
      .post('/analyze')
      .reply(200, { score: 90, interpretation: 'Normal' });

    const bot = new TrafficBot({
      targetUrl: 'http://localhost:3000',
      endpoint: '/analyze',
      totalRequests: 3,
      intervalMs: 0,
      concurrency: 1,
    });

    const result: TrafficBotResult = await bot.run();

    expect(result.totalSent).toBe(3);
    expect(result.successCount).toBe(2);
    expect(result.failureCount).toBe(1);
    expect(result.successCount + result.failureCount).toBe(result.totalSent);
  }, 15000);

  // ─────────────────────────────────────────────────────────────
  // L2: 변환 로직 — P95 응답시간 계산
  // ─────────────────────────────────────────────────────────────
  test('should_calculate_p95_response_time_when_given_response_times', () => {
    const bot = new TrafficBot();

    // 10개의 응답시간: [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
    // P95 인덱스: Math.ceil(10 * 0.95) - 1 = Math.ceil(9.5) - 1 = 10 - 1 = 9
    // 정렬 후 인덱스 9 = 100
    const responseTimes = [50, 10, 90, 30, 70, 20, 80, 40, 60, 100];
    const p95 = bot.calculateP95(responseTimes);

    expect(p95).toBe(100);
  });

  test('should_calculate_p95_correctly_when_single_element', () => {
    const bot = new TrafficBot();
    const p95 = bot.calculateP95([42]);
    expect(p95).toBe(42);
  });

  // ─────────────────────────────────────────────────────────────
  // L3: 경계 조건 — 로그 파일 증가 확인
  // supertest로 Express 앱에 직접 요청 → 실제 Winston 로그 발생
  // ─────────────────────────────────────────────────────────────
  test('should_log_file_grow_after_requests_when_N_requests_sent', async () => {
    // NODE_ENV=test 이므로 rate limiter는 skip됨
    // nock은 비활성화 (supertest가 직접 Express 앱에 접속)
    nock.enableNetConnect('127.0.0.1');

    // JPEG 매직 바이트로 시작하는 더미 이미지
    const dummyJpeg = Buffer.concat([
      Buffer.from([0xFF, 0xD8, 0xFF, 0xE0]),
      Buffer.alloc(20, 0x00),
    ]);

    // 로그 디렉토리 경로 (logger.ts LOG_DIR: src/utils/__dirname + ../../logs = api-gateway/logs)
    const logsDir = path.join(__dirname, '../logs');
    if (!fs.existsSync(logsDir)) {
      fs.mkdirSync(logsDir, { recursive: true });
    }

    // 요청 전 combined 로그 파일 크기 측정
    const today = new Date().toISOString().slice(0, 10); // YYYY-MM-DD
    const combinedLogPath = path.join(logsDir, `combined-${today}.log`);

    const sizeBefore = fs.existsSync(combinedLogPath)
      ? fs.statSync(combinedLogPath).size
      : 0;

    // supertest로 실제 요청 2건 전송 (axios는 mock됨 -> 500 응답 예상)
    // 우리가 검증하는 것은 "로그가 기록되는가" 이므로 응답 상태는 무관
    await request(app)
      .post('/analyze')
      .attach('image', dummyJpeg, { filename: 'test.jpg', contentType: 'image/jpeg' });

    await request(app)
      .post('/analyze')
      .attach('image', dummyJpeg, { filename: 'test.jpg', contentType: 'image/jpeg' });

    // Winston은 비동기로 파일에 씀 — 잠시 대기
    await new Promise(resolve => setTimeout(resolve, 500));

    const sizeAfter = fs.existsSync(combinedLogPath)
      ? fs.statSync(combinedLogPath).size
      : 0;

    // 로그 파일 크기가 증가했는지 확인
    expect(sizeAfter).toBeGreaterThan(sizeBefore);
  }, 15000);

  // ─────────────────────────────────────────────────────────────
  // L3: 경계 조건 — 에러 분류
  // ─────────────────────────────────────────────────────────────
  test('should_aggregate_errors_by_status_code_when_multiple_failures', async () => {
    // 500 두 번, 503 한 번
    nock('http://localhost:3000')
      .post('/analyze')
      .reply(500, { error: 'Internal Server Error' })
      .post('/analyze')
      .reply(503, { error: 'Service Unavailable' })
      .post('/analyze')
      .reply(500, { error: 'Internal Server Error' });

    const bot = new TrafficBot({
      targetUrl: 'http://localhost:3000',
      endpoint: '/analyze',
      totalRequests: 3,
      intervalMs: 0,
      concurrency: 1,
    });

    const result: TrafficBotResult = await bot.run();

    expect(result.failureCount).toBe(3);
    expect(result.errors.length).toBeGreaterThan(0);

    const status500 = result.errors.find((e: { status: number; count: number }) => e.status === 500);
    const status503 = result.errors.find((e: { status: number; count: number }) => e.status === 503);

    expect(status500).toBeDefined();
    expect(status500!.count).toBe(2);
    expect(status503).toBeDefined();
    expect(status503!.count).toBe(1);
  }, 15000);
});

// ─────────────────────────────────────────────────────────────
// TrafficBot: Server-Timing 통합 (ADR-026 연동)
// ─────────────────────────────────────────────────────────────

describe('TrafficBot: Server-Timing Integration', () => {
  beforeEach(() => {
    nock.cleanAll();
  });

  afterEach(() => {
    nock.cleanAll();
    nock.enableNetConnect();
  });

  // ─────────────────────────────────────────────────────────────
  // L2: profileKey 설정 시 Server-Timing 헤더 수집 및 집계
  // ─────────────────────────────────────────────────────────────
  test('should_collect_server_timing_when_profileKey_is_set', async () => {
    nock('http://localhost:3000')
      .post('/analyze')
      .times(2)
      .reply(200, { score: 100, interpretation: 'Normal' }, {
        'Server-Timing': 'gateway;dur=12.3, preprocess;dur=18.7, ai_inference;dur=8.5',
      });

    const bot = new TrafficBot({
      targetUrl: 'http://localhost:3000',
      endpoint: '/analyze',
      totalRequests: 2,
      intervalMs: 0,
      concurrency: 1,
      profileKey: 'test-secret',
    });

    const result: TrafficBotResult = await bot.run();

    expect(result.avgTimings).toBeDefined();
    expect(result.avgTimings!.gateway).toBeCloseTo(12.3, 0);
    expect(result.avgTimings!.preprocess).toBeCloseTo(18.7, 0);
    expect(result.avgTimings!.aiInference).toBeCloseTo(8.5, 0);
  }, 15000);

  // ─────────────────────────────────────────────────────────────
  // L2: profileKey 없으면 avgTimings 미포함
  // ─────────────────────────────────────────────────────────────
  test('should_not_collect_server_timing_when_profileKey_is_not_set', async () => {
    nock('http://localhost:3000')
      .post('/analyze')
      .reply(200, { score: 100, interpretation: 'Normal' }, {
        'Server-Timing': 'gateway;dur=12.3',
      });

    const bot = new TrafficBot({
      targetUrl: 'http://localhost:3000',
      endpoint: '/analyze',
      totalRequests: 1,
      intervalMs: 0,
      concurrency: 1,
    });

    const result: TrafficBotResult = await bot.run();

    expect(result.avgTimings).toBeUndefined();
  }, 15000);
});
