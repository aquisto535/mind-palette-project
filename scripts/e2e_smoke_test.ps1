param (
    [switch]$ProfileMode,          #프로파일링 모드
    [int]$Concurrency = 1,          #동시 요청 수
    [int]$Requests = 10,            #총 요청 수
    [int]$IntervalMs = 7000         #요청 간격
)

# Mind Palette E2E Smoke Test
# V16: TrafficBot integration — 서버 3개 기동 + TrafficBot N발 분산사격 + Server-Timing 집계 + 클린 종료
$LOG_FILE = "e2e_final_log.txt"
if (Test-Path $LOG_FILE) { Remove-Item $LOG_FILE }

function Write-LogInfo($msg) {
    $ts = Get-Date -Format "HH:mm:ss"
    Write-Host "[$ts] $msg"
    "[$ts] $msg" | Out-File $LOG_FILE -Append
}

function Wait-ForServer($url, $timeoutSeconds = 60) {
    Write-LogInfo "Waiting for server at $url (Timeout: ${timeoutSeconds}s)..."
    $start = Get-Date
    while (((Get-Date) - $start).TotalSeconds -lt $timeoutSeconds) {
        try {
            $resp = & curl.exe -s -o /dev/null -w "%{http_code}" $url
            if ($resp -eq "200") {
                Write-Host ""
                Write-LogInfo "Server at $url is READY."
                return $true
            }
        } catch { }
        Write-Host "." -NoNewline
        Start-Sleep -Seconds 2
    }
    Write-Host ""
    Write-LogInfo "Error: Timeout waiting for server at $url"
    return $false
}

Write-LogInfo "Cleaning up old processes..."
Get-Process | Where-Object { $_.ProcessName -match "preprocess_server|node|python" } | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

$PROJECT_ROOT = Split-Path -Parent $PSScriptRoot

# ─────────────────────────────────────────────────────────
# 1. 서버 구동
# ─────────────────────────────────────────────────────────
Write-LogInfo "Starting servers..."

$preprocessProc = Start-Process -FilePath "$PROJECT_ROOT\preprocess-server\build\bin\preprocess_server.exe" -WorkingDirectory "$PROJECT_ROOT\preprocess-server" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:8081/health" 10)) { exit 1 }

$aiProc = Start-Process -FilePath "python.exe" -ArgumentList "-m uvicorn src.main:app --host 127.0.0.1 --port 8082" -WorkingDirectory "$PROJECT_ROOT\ai-server" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:8082/health" 60)) { exit 1 }

$gatewayProc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c npm run dev" -WorkingDirectory "$PROJECT_ROOT\api-gateway" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:3000/health" 30)) { exit 1 }

# ─────────────────────────────────────────────────────────
# 2. TrafficBot 실행 (분산 사격)
# ─────────────────────────────────────────────────────────
$adminKey = $env:ADMIN_PROFILE_KEY
if ($ProfileMode -and (-not $adminKey)) {
    Write-LogInfo "Warning: ProfileMode is ON but ADMIN_PROFILE_KEY is not set. Server-Timing will not be collected."
}

Write-LogInfo "=== TrafficBot 시작 (requests=$Requests, interval=${IntervalMs}ms, concurrency=$Concurrency, profileMode=$ProfileMode) ==="

$botArgs = @(
    "src/tools/runTrafficBot.ts",
    "--requests", "$Requests",
    "--interval", "$IntervalMs",
    "--concurrency", "$Concurrency",
    "--url", "http://127.0.0.1:3000",
    "--endpoint", "/analyze"
)
if ($ProfileMode -and $adminKey) {
    $botArgs += "--profile-key", $adminKey
}

Push-Location "$PROJECT_ROOT\api-gateway"
try {
    $tsNode = ".\node_modules\.bin\ts-node.cmd"
    $prevEncoding = [Console]::OutputEncoding
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $botOutput = & $tsNode $botArgs 2>&1
    [Console]::OutputEncoding = $prevEncoding
    $botOutput | ForEach-Object { Write-LogInfo $_ }
    $botOutput | Out-File $LOG_FILE -Append -Encoding utf8
} finally {
    Pop-Location
}

# ─────────────────────────────────────────────────────────
# 3. 종료 및 정리
# ─────────────────────────────────────────────────────────
Write-LogInfo "Cleaning up..."
Stop-Process -Id $preprocessProc.Id, $aiProc.Id, $gatewayProc.Id -Force -ErrorAction SilentlyContinue
Get-Process | Where-Object { $_.ProcessName -match "preprocess_server|node|python" } | Stop-Process -Force -ErrorAction SilentlyContinue
Write-LogInfo "E2E Smoke Test completed."
