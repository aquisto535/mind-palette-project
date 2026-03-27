# Final Verification Script
$PROJECT_ROOT = (Get-Item .).FullName
Write-LogInfo "Starting Gateway..."
Start-Process -FilePath "cmd.exe" -ArgumentList "/c npm run dev" -WorkingDirectory "$PROJECT_ROOT\api-gateway" -NoNewWindow
Start-Sleep -Seconds 20

Write-LogInfo "Sending Analysis Request..."
$response = & curl.exe -X POST "http://127.0.0.1:3000/analyze" -F "image=@$PROJECT_ROOT\shared_volume\test.jpg" --silent
if ($response) {
    Write-LogInfo "Response Received:"
    $response | ConvertFrom-Json | ConvertTo-Json -Depth 5 | Out-File "final_result.json"
    Write-Host $response
} else {
    Write-LogInfo "No Response."
}

function Write-LogInfo($msg) {
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] $msg"
}
