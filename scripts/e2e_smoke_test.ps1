param (
    [switch]$ProfileMode,
    [int]$Concurrency = 1,
    [string]$FixedImage = ""
)

# Mind Palette E2E Smoke Test
# V15: Dynamic health checks, concurrency, and Server-Timing profiling
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

$PROJECT_ROOT = (Get-Item .).FullName

# ─────────────────────────────────────────────────────────
# 1. 경로 탐색 (인코딩 우회를 위해 오브젝트 파이프라인 사용)
# ─────────────────────────────────────────────────────────
$DOC_PATH = [Environment]::GetFolderPath("MyDocuments")
$KAKAO_DIR = Get-ChildItem -LiteralPath $DOC_PATH -Directory | Where-Object { $_.Name -like "*카카오톡*" -or $_.Name -like "*Kakao*" } | Select-Object -First 1

if ($null -eq $KAKAO_DIR) {
    $KAKAO_DIR = Get-ChildItem -LiteralPath $DOC_PATH -Directory -Recurse -Depth 1 | Where-Object { 
        (Get-ChildItem -LiteralPath $_.FullName -Directory -Filter "VS_*") 
    } | Select-Object -First 1
}

if ($null -eq $KAKAO_DIR) {
    Write-LogInfo "Error: Could not find image directories."
    exit 1
}

# VS_* 폴더 탐색 (Pipeline 사용)
$TARGET_DIRS = $KAKAO_DIR | Get-ChildItem -Directory -Filter "VS_*"
if ($TARGET_DIRS.Count -eq 0) {
    Write-LogInfo "Error: No folders matching 'VS_*' found."
    exit 1
}

Write-LogInfo "Target directories found successfully."

# ─────────────────────────────────────────────────────────
# 2. 서버 구동
# ─────────────────────────────────────────────────────────
Write-LogInfo "Starting servers..."

$preprocessProc = Start-Process -FilePath "$PROJECT_ROOT\preprocess-server\build\bin\preprocess_server.exe" -WorkingDirectory "$PROJECT_ROOT\preprocess-server" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:8081/health" 10)) { exit 1 }

$aiProc = Start-Process -FilePath "python.exe" -ArgumentList "-m uvicorn src.main:app --host 127.0.0.1 --port 8082" -WorkingDirectory "$PROJECT_ROOT\ai-server" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:8082/health" 60)) { exit 1 }

$gatewayProc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c npm run dev" -WorkingDirectory "$PROJECT_ROOT\api-gateway" -NoNewWindow -PassThru
if (-not (Wait-ForServer "http://127.0.0.1:3000/health" 30)) { exit 1 }

# ─────────────────────────────────────────────────────────
# 3. 이미지 선택 (LiteralPath 사용)
# ─────────────────────────────────────────────────────────
if ($FixedImage -ne "") {
    # 고정 이미지 모드: 성능 비교 테스트에 사용
    if (-not (Test-Path -LiteralPath $FixedImage)) {
        Write-LogInfo "Error: FixedImage not found: $FixedImage"
        exit 1
    }
    $imgPath = $FixedImage
    Write-LogInfo "[Fixed] Selected Image: $imgPath"
} else {
    # 랜덤 이미지 모드 (기본)
    $selectedSubDir = $TARGET_DIRS | Get-Random
    $selectedImage = $selectedSubDir | Get-ChildItem -File | Where-Object { $_.Extension -match "jpg|png" } | Get-Random

    if ($null -eq $selectedImage) {
        Write-LogInfo "Error: No images found in selected directory."
        exit 1
    }

    # 8.3 Short Path를 사용하여 인코딩 문제 원천 차단 (최후의 수단)
    $imgPath = $selectedImage.FullName
    try {
        $fso = New-Object -ComObject Scripting.FileSystemObject
        $shortPath = $fso.GetFile($imgPath).ShortPath
        if ($shortPath) { $imgPath = $shortPath }
    } catch { }

    Write-LogInfo "[Random] Selected Image: $imgPath"
}

# ─────────────────────────────────────────────────────────
# 4. Analysis Request (With Concurrency & Profiling)
# ─────────────────────────────────────────────────────────
Write-LogInfo "Sending analysis requests (Concurrency: $Concurrency, ProfileMode: $ProfileMode)..."

$adminKey = $env:ADMIN_PROFILE_KEY
if ($ProfileMode -and (-not $adminKey)) {
    Write-LogInfo "Warning: ProfileMode is switched ON but ADMIN_PROFILE_KEY environment variable is not set."
}

$scriptBlock = {
    param($imgPath, $ProfileMode, $adminKey)
    $curlArgs = @("-X", "POST", "http://127.0.0.1:3000/analyze", "-F", "image=@$imgPath", "-s")
    if ($ProfileMode -and $adminKey) {
        $curlArgs += "-H", "X-Admin-Profile-Key: $adminKey", "-i"
    }
    
    $startTime = Get-Date
    $rawResponse = & curl.exe $curlArgs 2>&1
    $endTime = Get-Date
    $totalMs = ($endTime - $startTime).TotalMilliseconds

    return [PSCustomObject]@{
        RawOutput = $rawResponse -join "`n"
        TotalMs = $totalMs
    }
}

$jobs = @()
for ($i = 1; $i -le $Concurrency; $i++) {
    $jobs += Start-Job -ScriptBlock $scriptBlock -ArgumentList $imgPath, $ProfileMode, $adminKey
}

Write-LogInfo "Waiting for $($jobs.Count) requests to complete..."
$completedJobs = $jobs | Wait-Job | Receive-Job
Remove-Job -State Completed

$metrics = @()

foreach ($result in $completedJobs) {
    $out = $result.RawOutput
    
    if ($ProfileMode) {
        $parts = $out -split "\r?\n\r?\n", 2
        $headers = if ($parts.Count -gt 0) { $parts[0] } else { "" }
        # $body logic removed as it's not used in ProfileMode summary table
        
        $gatewayDur = 0; $preprocessDur = 0; $aiDur = 0; $totalE2EDur = [math]::Round($result.TotalMs, 1)
        
        if ($headers -match "Server-Timing:\s*(.*?)(?=\r?\n|$)") {
            $timingHeader = $matches[1]
            if ($timingHeader -match "gateway;dur=([\d\.]+)") { $gatewayDur = [math]::Round([double]$matches[1], 1) }
            if ($timingHeader -match "preprocess;dur=([\d\.]+)") { $preprocessDur = [math]::Round([double]$matches[1], 1) }
            if ($timingHeader -match "ai_inference;dur=([\d\.]+)") { $aiDur = [math]::Round([double]$matches[1], 1) }
        }
        
        $metrics += [PSCustomObject]@{
            Total_E2E_Ms = $totalE2EDur
            Gateway_Ms   = $gatewayDur
            CPP_Pre_Ms   = $preprocessDur
            Python_AI_Ms = $aiDur
        }
    } else {
        try {
            $json = $out | ConvertFrom-Json
            if ($null -ne $json.score -and $json.score -gt 0) {
                Write-LogInfo "TEST STATUS: PASSED (OK) - Real AI Result Received. IQ: $($json.score)"
            } else {
                Write-LogInfo "TEST STATUS: FAILED (ERR) - Response mismatch"
            }
        } catch {
            Write-LogInfo "Error parsing JSON: $out"
        }
    }
}

if ($ProfileMode -and ($metrics.Count -gt 0)) {
    Write-LogInfo "=== PROFILING REPORT ($Concurrency Concurrent Requests) ==="
    $tableString = $metrics | Format-Table -AutoSize | Out-String
    Write-Host $tableString
    $tableString | Out-File $LOG_FILE -Append
}

# ─────────────────────────────────────────────────────────
# 5. 종료 및 정리
# ─────────────────────────────────────────────────────────
Write-LogInfo "Cleaning up..."
Stop-Process -Id $preprocessProc.Id, $aiProc.Id, $gatewayProc.Id -Force -ErrorAction SilentlyContinue
Get-Process | Where-Object { $_.ProcessName -match "preprocess_server|node|python" } | Stop-Process -Force -ErrorAction SilentlyContinue
Write-LogInfo "E2E Smoke Test completed."
