"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const node_os_1 = __importDefault(require("node:os"));
const node_child_process_1 = require("node:child_process");
const express_rate_limit_1 = __importDefault(require("express-rate-limit"));
const router = express_1.default.Router();
// 헬스 체크 엔드포인트를 위한 전용 레이트 리미터 설정
// OS 명령(execSync) 실행에 따른 부하를 방지하기 위해 호출 횟수를 제한합니다.
const healthRateLimiter = (0, express_rate_limit_1.default)({
    windowMs: 1 * 60 * 1000, // 1분
    max: 60, // IP당 1분에 최대 60번 요청 허용
    message: JSON.stringify({
        status: 'error',
        message: 'Too many health check requests, please try again later.'
    }),
    standardHeaders: true, // `RateLimit-*` 헤더 포함
    legacyHeaders: false, // `X-RateLimit-*` 헤더 비활성화
});
/**
 * GET /health
 * 서버 상태를 확인하는 헬스 체크 엔드포인트
 *
 * @returns {Object} 서버 상태 정보 (status, uptime, memory, disk)
 */
router.get('/', healthRateLimiter, (req, res) => {
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
            if (node_os_1.default.platform() === 'win32') {
                // Windows: wmic 명령어 사용
                const output = (0, node_child_process_1.execSync)('wmic logicaldisk where "DeviceID=\'C:\'" get FreeSpace', { encoding: 'utf8' });
                const lines = output.trim().split('\n');
                if (lines.length > 1) {
                    const rawLine = lines[1];
                    if (rawLine) {
                        const freeSpaceBytes = Number.parseInt(rawLine.trim());
                        diskAvailableGB = (freeSpaceBytes / 1024 / 1024 / 1024).toFixed(2) + ' GB';
                    }
                }
            }
            else {
                // Linux/Mac: df 명령어 사용
                const output = (0, node_child_process_1.execSync)('df -k / | tail -1 | awk \'{print $4}\'', { encoding: 'utf8' });
                const freeSpaceKB = Number.parseInt(output.trim());
                diskAvailableGB = (freeSpaceKB / 1024 / 1024).toFixed(2) + ' GB';
            }
        }
        catch {
            // 디스크 체크 실패 시 'N/A'로 표시 (비핵심 정보이므로 무시)
            diskAvailableGB = 'N/A';
        }
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
            }
        });
    }
    catch (error) {
        res.status(503).json({
            status: 'unhealthy',
            error: error instanceof Error ? error.message : 'Unknown error',
            timestamp: new Date().toISOString()
        });
    }
});
exports.default = router;
