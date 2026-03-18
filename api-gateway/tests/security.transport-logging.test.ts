/**
 * [TDD Red] 전송 보안 + 로깅 보안 검증 테스트
 *
 * 전송 보안 (Transport Security):
 *   - 외부 HTTPS: PREPROCESS_SERVER_URL / AI_SERVER_URL이 내부망 주소(localhost/내부IP)인지 검증
 *   - 내부망 격리: 서비스 간 URL이 http:// 이고 공개 인터넷 주소가 아닌지 검증
 *
 * 로깅 보안 (Logging Hygiene):
 *   - PII 마스킹: 파일 경로 중 개인 식별 가능한 원본 파일명이 로그에 노출되지 않아야 함
 *   - 에러 메시지 최소화: 500 응답에 내부 스택/경로가 노출되지 않아야 함
 */
import request from 'supertest';
import app from '../src/server';
import { maskPII } from '../src/utils/logger';

// ─────────────────────────────────────────────────────────
// 전송 보안: 내부망 URL 검증
// ─────────────────────────────────────────────────────────
describe('전송 보안 — 내부망 격리 (Transport Security)', () => {
  it('PREPROCESS_SERVER_URL은 공개 인터넷 주소가 아니어야 한다', () => {
    const url = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
    expect(isInternalUrl(url)).toBe(true);
  });

  it('AI_SERVER_URL은 공개 인터넷 주소가 아니어야 한다', () => {
    const url = process.env.AI_SERVER_URL || 'http://localhost:8002';
    expect(isInternalUrl(url)).toBe(true);
  });

  it('서비스 간 통신 URL은 http:// (비SSL) 이어야 한다 — 내부망이므로 SSL 오버헤드 불필요', () => {
    const preprocessUrl = process.env.PREPROCESS_SERVER_URL || 'http://localhost:8081';
    const aiUrl = process.env.AI_SERVER_URL || 'http://localhost:8002';
    expect(preprocessUrl.startsWith('http://')).toBe(true);
    expect(aiUrl.startsWith('http://')).toBe(true);
  });
});

// ─────────────────────────────────────────────────────────
// 로깅 보안: PII 마스킹
// ─────────────────────────────────────────────────────────
describe('로깅 보안 — PII 마스킹 (Logging Hygiene)', () => {
  it('maskPII 함수가 export 되어 있어야 한다', () => {
    expect(typeof maskPII).toBe('function');
  });

  it('원본 파일명(originalname)을 마스킹해야 한다', () => {
    const input = { originalname: '홍길동_2015년생.jpg', requestId: 'abc-123' };
    const result = maskPII(input);
    expect(result.originalname).toMatch(/\*+/);
    expect(result.originalname).not.toBe('홍길동_2015년생.jpg');
  });

  it('파일 경로에서 원본 파일명 부분을 마스킹해야 한다', () => {
    const input = { path: '/shared/uploads/홍길동_그림.png', requestId: 'abc-123' };
    const result = maskPII(input);
    // 경로 자체는 유지하되 파일명 부분을 마스킹
    expect(result.path).not.toContain('홍길동_그림.png');
  });

  it('requestId, error 등 비PII 필드는 그대로 유지해야 한다', () => {
    const input = { requestId: 'abc-123', error: 'some error', score: 85 };
    const result = maskPII(input);
    expect(result.requestId).toBe('abc-123');
    expect(result.error).toBe('some error');
    expect(result.score).toBe(85);
  });

  it('PII 필드가 없는 객체는 변경 없이 반환해야 한다', () => {
    const input = { status: 'healthy', uptime: '5 minutes' };
    const result = maskPII(input);
    expect(result).toEqual(input);
  });
});

// ─────────────────────────────────────────────────────────
// 로깅 보안: 에러 메시지 최소화
// ─────────────────────────────────────────────────────────
describe('로깅 보안 — 에러 메시지 최소화 (Error Exposure)', () => {
  it('500 에러 응답에 스택 트레이스가 포함되지 않아야 한다', async () => {
    // 존재하지 않는 라우트에 잘못된 Content-Type 전송 → 에러 유도
    const res = await request(app)
      .post('/analyze')
      .set('Content-Type', 'application/json')
      .send({ malformed: true });

    // 400 또는 500이어도 스택/내부 경로 노출 금지
    const body = JSON.stringify(res.body);
    expect(body).not.toMatch(/at \w+.*\(.*\.ts:\d+/);     // TS 스택 트레이스
    expect(body).not.toMatch(/at \w+.*\(.*\.js:\d+/);     // JS 스택 트레이스
    expect(body).not.toMatch(/node_modules/);             // node_modules 경로
    expect(body).not.toMatch(/C:\\|\/Users\/|\/home\//);  // 절대 경로
  });

  it('400 에러 응답 body에는 error 필드 하나만 있어야 한다', async () => {
    const res = await request(app).post('/analyze');
    expect(res.status).toBe(400);
    const keys = Object.keys(res.body);
    expect(keys).toEqual(['error']);
  });

  it('에러 응답의 error 메시지는 짧아야 한다 (200자 미만)', async () => {
    const res = await request(app).post('/analyze');
    expect(res.body.error.length).toBeLessThan(200);
  });
});

// ─────────────────────────────────────────────────────────
// 헬퍼
// ─────────────────────────────────────────────────────────
function isInternalUrl(url: string): boolean {
  try {
    const { hostname } = new URL(url);
    return (
      hostname === 'localhost' ||
      hostname === '127.0.0.1' ||
      hostname.startsWith('10.') ||
      hostname.startsWith('172.16.') ||
      hostname.startsWith('192.168.') ||
      hostname.endsWith('.internal') ||
      hostname.endsWith('.local')
    );
  } catch {
    return false;
  }
}
