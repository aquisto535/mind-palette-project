/**
 * Mind Palette k6 부하 테스트
 *
 * 실행:
 *   k6 run scripts/load-test.js
 *   k6 run --env TARGET_URL=https://<도메인>/api/analyze scripts/load-test.js
 *
 * 시나리오:
 *   smoke         — 1 VU,  1분    (기본 동작 확인)
 *   load          — 100 VU, 5분   (c5.large 정상 부하)
 *   stress        — 200 VU, 10분  (한계 탐색, 점진적 증가)
 *   backpressure  — 50 VU,  2분   (큐 거절(503) + Retry-After 검증)
 *
 * 백프레셔 정책 (preprocess-server 개선 1+2):
 *   - ThreadPool 큐 크기 16개 제한, 초과 시 503 + Retry-After: 5 반환
 *   - 503은 의도된 정상 동작이므로 실패율 계산에서 제외
 *   - 200 응답만 P95 측정 (503 즉시 응답이 평균을 왜곡하는 것 방지)
 */

import http from 'k6/http';
import { check, sleep } from 'k6';
import { Trend, Rate } from 'k6/metrics';

// ─────────────────────────────────────────────────────────────────────────────
// 설정
// ─────────────────────────────────────────────────────────────────────────────
const TARGET_URL = __ENV.TARGET_URL || 'http://localhost:3000/api/analyze';

// 샘플 JPEG (1×1px, Base64 인코딩 최소 이미지)
const SAMPLE_IMAGE = open('./test-images/sample.jpg', 'b');

export const options = {
  scenarios: {
    smoke: {
      executor: 'constant-vus',
      vus: 1,
      duration: '1m',
      tags: { scenario: 'smoke' },
    },
    load: {
      executor: 'constant-vus',
      vus: 100,
      duration: '5m',
      startTime: '1m30s',    // smoke 완료 후 시작
      tags: { scenario: 'load' },
    },
    stress: {
      executor: 'ramping-vus',
      startVUs: 0,
      stages: [
        { duration: '2m', target: 100 },
        { duration: '5m', target: 200 },
        { duration: '3m', target: 0 },
      ],
      startTime: '7m',       // load 완료 후 시작
      tags: { scenario: 'stress' },
    },
    backpressure: {
      executor: 'constant-vus',
      vus: 50,                // 큐(16) + 워커(2) 한계의 약 3배 — 의도적 큐 초과
      duration: '2m',
      startTime: '17m',       // stress 완료 후 시작
      tags: { scenario: 'backpressure' },
    },
  },
  thresholds: {
    // 200 응답만 P95 측정 (503은 즉시 응답이라 평균 왜곡)
    'http_req_duration{scenario:load,status:200}':  ['p(95)<500'],
    'http_req_duration{scenario:smoke,status:200}': ['p(95)<10000'],

    // 진짜 에러(500/502/504)만 실패로 카운트 — 503은 의도된 백프레셔
    'real_error_rate': ['rate<0.01'],

    // 백프레셔 발동률: 시나리오별 허용 범위 차등
    'rejected_503_rate{scenario:load}':         ['rate<0.05'],   // 정상 부하: 5% 미만
    'rejected_503_rate{scenario:stress}':       ['rate<0.50'],   // 스트레스: 50%까지 허용
    'rejected_503_rate{scenario:backpressure}': ['rate>0.30'],   // 백프레셔 검증: 30% 초과 (발동 증명)
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// 커스텀 메트릭
// ─────────────────────────────────────────────────────────────────────────────
const analysisDuration = new Trend('analysis_duration_ms');
const rejected503Rate = new Rate('rejected_503_rate');     // 의도된 큐 거절
const realErrorRate = new Rate('real_error_rate');         // 진짜 에러 (5xx 중 503 제외)

// ─────────────────────────────────────────────────────────────────────────────
// 기본 VU 함수
// ─────────────────────────────────────────────────────────────────────────────
export default function () {
  const data = {
    image: http.file(SAMPLE_IMAGE, 'test.jpg', 'image/jpeg'),
    childInfo: JSON.stringify({ age: 7, gender: 'male' }),
  };

  const res = http.post(TARGET_URL, data);

  check(res, {
    'status 200 or 503': (r) => r.status === 200 || r.status === 503,
    'has score on 200': (r) => {
      if (r.status !== 200) return true; // 200이 아닌 경우 스킵
      try {
        return typeof JSON.parse(r.body).score === 'number';
      } catch {
        return false;
      }
    },
    'has Retry-After on 503': (r) =>
      r.status !== 503 || r.headers['Retry-After'] !== undefined,
    'SERVER_BUSY body on 503': (r) => {
      if (r.status !== 503) return true;
      try {
        return JSON.parse(r.body).error === 'SERVER_BUSY';
      } catch {
        return false;
      }
    },
  });

  // 메트릭 집계
  rejected503Rate.add(res.status === 503);
  realErrorRate.add(res.status >= 500 && res.status !== 503);

  if (res.status === 200) {
    analysisDuration.add(res.timings.duration);
  }

  sleep(1);
}
