#!/usr/bin/env node
/**
 * verify-env.mjs — Vite 빌드 전 환경 변수 안전성 검증
 *
 * 실행 위치: package.json의 "prebuild" 훅
 * 목적: 프로덕션 빌드에 위험한 설정(Mock 모드 등)이 포함되는 것을 사전 차단
 *
 * 종료 코드:
 *   0 — 검증 통과
 *   1 — 위험 설정 감지 (빌드 중단)
 */

const isCI = process.env.CI === 'true';
const nodeEnv = process.env.NODE_ENV ?? 'development';

/**
 * 프로덕션 빌드 판단 기준:
 * - NODE_ENV=production 이거나
 * - CI 환경(GitHub Actions 등)인 경우
 */
const isProductionBuild = nodeEnv === 'production' || isCI;

// ─── 검사 1: VITE_USE_MOCK ──────────────────────────────────────────────────
const useMock = process.env.VITE_USE_MOCK;

if (isProductionBuild && useMock === 'true') {
  console.error('');
  console.error('╔══════════════════════════════════════════════════════════════╗');
  console.error('║  ❌  PRODUCTION BUILD BLOCKED — DANGEROUS CONFIGURATION     ║');
  console.error('╠══════════════════════════════════════════════════════════════╣');
  console.error('║  VITE_USE_MOCK=true 가 프로덕션 빌드에서 감지되었습니다.    ║');
  console.error('║                                                              ║');
  console.error('║  이 상태로 빌드하면 실제 AI 분석 대신 Mock 데이터가         ║');
  console.error('║  사용자에게 반환됩니다.                                      ║');
  console.error('║                                                              ║');
  console.error('║  해결 방법:                                                  ║');
  console.error('║    VITE_USE_MOCK=false (또는 완전히 제거) 후 재빌드          ║');
  console.error('╚══════════════════════════════════════════════════════════════╝');
  console.error('');
  process.exit(1);
}

// ─── 검사 2: VITE_API_URL 경고 ──────────────────────────────────────────────
const apiUrl = process.env.VITE_API_URL;
if (apiUrl && !apiUrl.startsWith('/') && !apiUrl.startsWith('http')) {
  console.warn(`[verify-env] ⚠️  VITE_API_URL="${apiUrl}" 형식이 올바르지 않습니다.`);
  console.warn('             /api 처럼 상대경로 또는 https:// 로 시작하는 절대경로를 사용하세요.');
}

// ─── 통과 ────────────────────────────────────────────────────────────────────
if (isProductionBuild) {
  console.log('[verify-env] ✅ 프로덕션 빌드 환경 변수 검증 통과');
  console.log(`             NODE_ENV=${nodeEnv}, CI=${isCI}, VITE_USE_MOCK=${useMock ?? '(미설정=false)'}`);
} else {
  console.log('[verify-env] ℹ️  개발 빌드 — 환경 변수 경고 검사만 수행합니다.');
}
