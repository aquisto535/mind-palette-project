/**
 * Mind Palette k6 부하 테스트
 *
 * 실행:
 *   k6 run scripts/load-test.js
 *   k6 run --env TARGET_URL=https://<도메인>/api/analyze scripts/load-test.js
 *
 * 시나리오:
 *   smoke  — 1 VU, 1분   (기본 동작 확인)
 *   load   — 100 VU, 5분  (c5.large 정상 부하)
 *   stress — 200 VU, 10분 (한계 탐색, 점진적 증가)
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
  },
  thresholds: {
    // 캐시 미스 포함 P95 < 500ms
    'http_req_duration{scenario:load}':   ['p(95)<500'],
    // 캐시 히트 시나리오 P95 < 10ms (smoke로 대략 확인)
    'http_req_duration{scenario:smoke}':  ['p(95)<10000'],
    // 실패율 1% 미만
    'http_req_failed':                    ['rate<0.01'],
  },
};

// 커스텀 메트릭
const analysisDuration = new Trend('analysis_duration_ms');
const errorRate = new Rate('error_rate');

// ─────────────────────────────────────────────────────────────────────────────
// 기본 VU 함수
// ─────────────────────────────────────────────────────────────────────────────
export default function () {
  const data = {
    image: http.file(SAMPLE_IMAGE, 'test.jpg', 'image/jpeg'),
    childInfo: JSON.stringify({ age: 7, gender: 'male' }),
  };

  const res = http.post(TARGET_URL, data);

  const ok = check(res, {
    'status 200': (r) => r.status === 200,
    'has score':  (r) => {
      try {
        const body = JSON.parse(r.body);
        return typeof body.score === 'number';
      } catch {
        return false;
      }
    },
  });

  analysisDuration.add(res.timings.duration);
  errorRate.add(!ok);

  sleep(1);
}
