import { TrafficBot } from './trafficBot';

// ─────────────────────────────────────────────────────────────
// runTrafficBot: CLI 진입점
// 사용법: ts-node src/tools/runTrafficBot.ts [옵션]
//   --requests <N>       전송할 요청 수 (기본: 100)
//   --interval <ms>      요청 간격 밀리초 (기본: 1000)
//   --url <url>          대상 서버 URL (기본: http://localhost:3000)
//   --endpoint <path>    대상 엔드포인트 (기본: /analyze)
//   --concurrency <N>    동시 요청 수 (기본: 1)
//   --profile-key <key>  X-Admin-Profile-Key 값 (설정 시 Server-Timing 수집 활성화, ADR-026)
// ─────────────────────────────────────────────────────────────

function parseArgs(args: string[]): Record<string, string> {
  const result: Record<string, string> = {};
  for (let i = 0; i < args.length; i += 2) {
    const key = args[i]?.replace(/^--/, '');
    const value = args[i + 1];
    if (key && value !== undefined) {
      result[key] = value;
    }
  }
  return result;
}

async function main(): Promise<void> {
  const args = process.argv.slice(2);
  const params = parseArgs(args);

  const bot = new TrafficBot({
    targetUrl: params['url'] ?? 'http://localhost:3000',
    endpoint: params['endpoint'] ?? '/analyze',
    intervalMs: params['interval'] === undefined ? 1000 : Number.parseInt(params['interval'], 10),
    totalRequests: params['requests'] === undefined ? 100 : Number.parseInt(params['requests'], 10),
    concurrency: params['concurrency'] === undefined ? 1 : Number.parseInt(params['concurrency'], 10),
    profileKey: params['profile-key'],
  });

  const config = bot.getConfig();
  console.log('TrafficBot 시작');
  console.log(`  대상: ${config.targetUrl}${config.endpoint}`);
  console.log(`  총 요청: ${config.totalRequests}`);
  console.log(`  동시 요청: ${config.concurrency}`);
  console.log(`  요청 간격: ${config.intervalMs}ms`);
  console.log('');

  const result = await bot.run();

  console.log(
    `[TrafficBot] 완료: ${result.successCount}/${result.totalSent} 성공, ` +
    `avgResponseMs=${result.avgResponseMs}ms, P95=${result.p95ResponseMs}ms`
  );

  if (result.errors.length > 0) {
    console.log('에러 분포:');
    for (const err of result.errors) {
      console.log(`  HTTP ${err.status}: ${err.count}회`);
    }
  }

  if (result.avgTimings) {
    console.log('서비스별 평균 응답시간 (Server-Timing):');
    if (result.avgTimings.gateway !== undefined)
      console.log(`  gateway:      ${result.avgTimings.gateway.toFixed(1)}ms`);
    if (result.avgTimings.preprocess !== undefined)
      console.log(`  preprocess:   ${result.avgTimings.preprocess.toFixed(1)}ms`);
    if (result.avgTimings.aiInference !== undefined)
      console.log(`  ai_inference: ${result.avgTimings.aiInference.toFixed(1)}ms`);
  }
}

void main().catch((err: unknown) => { // NOSONAR: top-level await unavailable with commonjs module
  console.error('TrafficBot 실행 오류:', err);
  process.exit(1);
});
