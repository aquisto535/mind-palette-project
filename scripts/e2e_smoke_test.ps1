# Mind Palette E2E Smoke Test
# V14: Dynamic health checks and literal path handling for Korean directories

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

Write-LogInfo "Selected Image: $imgPath"

# Analysis Request
Write-LogInfo "Sending analysis request..."
try {
    $response = & curl.exe -X POST "http://127.0.0.1:3000/analyze" -F "image=@$imgPath" --silent
    if (-not $response) { throw "No response received" }
    
    $json = $response | ConvertFrom-Json
    $json | ConvertTo-Json -Depth 5 | Out-File $LOG_FILE -Append
    
    if ($null -ne $json.score -and $json.score -gt 0) {
        Write-LogInfo "TEST STATUS: PASSED (OK) - Real AI Result Received"
        Write-LogInfo "IQ: $($json.score), Percentile: $($json.percentile)%"
    } else {
        Write-LogInfo "TEST STATUS: FAILED (ERR) - Response mismatch"
        Write-LogInfo "Response: $response"
    }
} catch {
    Write-LogInfo "Error during analysis: $($_.Exception.Message)"
}

# ─────────────────────────────────────────────────────────
# 4. 종료 및 정리
# ─────────────────────────────────────────────────────────
Write-LogInfo "Cleaning up..."
Stop-Process -Id $preprocessProc.Id, $aiProc.Id, $gatewayProc.Id -Force -ErrorAction SilentlyContinue
Get-Process | Where-Object { $_.ProcessName -match "preprocess_server|node|python" } | Stop-Process -Force -ErrorAction SilentlyContinue
Write-LogInfo "E2E Smoke Test completed."
