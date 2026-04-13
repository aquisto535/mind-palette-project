import express, { Request, Response } from 'express';
import os from 'node:os';
import { execSync } from 'node:child_process';
import rateLimit from 'express-rate-limit';

const router = express.Router();

// 헬스 체크 엔드포인트를 위한 전용 레이트 리미터 설정
// OS 명령(execSync) 실행에 따른 부하를 방지하기 위해 호출 횟수를 제한합니다.
const healthRateLimiter = rateLimit({
  windowMs: 1 * 60 * 1000, // 1분
  max: 60,                // IP당 1분에 최대 60번 요청 허용
  message: JSON.stringify({
    status: 'error',
    message: 'Too many health check requests, please try again later.'
  }),
  standardHeaders: true, // `RateLimit-*` 헤더 포함
  legacyHeaders: false,  // `X-RateLimit-*` 헤더 비활성화
});

/**
 * GET /health
 * 서버 상태를 확인하는 헬스 체크 엔드포인트
 * 
 * @returns {Object} 서버 상태 정보 (status, uptime, memory, disk)
 */
router.get('/', healthRateLimiter, (req: Request, res: Response) => {
  try {
    // 서버 가동 시간
    const uptime = process.uptime();
    const uptimeMinutes = Math.floor(uptime / 60);
    const uptimeSeconds = Math.floor(uptime % 60);

    // 메모리 사용량
    const memoryUsage = process.memoryUsage();
    const heapUsedMB = (memoryUsage.heapUsed / 1024 / 1024).toFixed(2);
    const heapTotalMB = (memoryUsage.heapTotal / 1024 / 1024).toFixed(2);

    // 디스크 여유 공간 (Windows/Linux 호환)
    let diskAvailableGB = 'N/A';
    try {
      if (os.platform() === 'win32') {
        // Windows: wmic 명령어 사용
        const output = execSync('wmic logicaldisk where "DeviceID=\'C:\'" get FreeSpace', { encoding: 'utf8' });
        const lines = output.trim().split('\n');
        if (lines.length > 1) {
          const rawLine = lines[1];
          if (rawLine) {
            const freeSpaceBytes = Number.parseInt(rawLine.trim());
            diskAvailableGB = (freeSpaceBytes / 1024 / 1024 / 1024).toFixed(2) + ' GB';
          }
        }
      } else {
        // Linux/Mac: df 명령어 사용
        const output = execSync('df -k / | tail -1 | awk \'{print $4}\'', { encoding: 'utf8' });
        const freeSpaceKB = Number.parseInt(output.trim());
        diskAvailableGB = (freeSpaceKB / 1024 / 1024).toFixed(2) + ' GB';
      }
    } catch {
      // 디스크 체크 실패 시 'N/A'로 표시 (비핵심 정보이므로 무시)
      diskAvailableGB = 'N/A';
    }

    // 런타임 모드 정보 — 배포 후 실제 동작 모드를 즉시 확인할 수 있도록 노출
    const nodeEnv = process.env.NODE_ENV ?? 'development';
    const adminProfileKeySet = Boolean(process.env.ADMIN_PROFILE_KEY);
    const keepImages = process.env.KEEP_IMAGES === 'true';

    res.json({
      status: 'healthy',
      timestamp: new Date().toISOString(),
      uptime: `${uptimeMinutes} minutes ${uptimeSeconds} seconds`,
      memory: {
        used: `${heapUsedMB} MB`,
        total: `${heapTotalMB} MB`
      },
      disk: {
        available: diskAvailableGB
      },
      // 운영 모드 정보 (배포 검증용)
      mode: {
        env: nodeEnv,
        admin_profiling_enabled: adminProfileKeySet,
        keep_images: keepImages,
        preprocess_url: process.env.PREPROCESS_SERVER_URL ?? 'http://127.0.0.1:8081',
        ai_url: process.env.AI_SERVER_URL ?? 'http://127.0.0.1:8082',
      }
    });
  } catch (error: unknown) {
    res.status(503).json({
      status: 'unhealthy',
      error: error instanceof Error ? error.message : 'Unknown error',
      timestamp: new Date().toISOString()
    });
  }
});

export default router;
