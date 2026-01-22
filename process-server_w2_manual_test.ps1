# Manual Verification Script for Mind Palette Preprocess Server

# 1. Configuration
$serverUrl = "http://localhost:8081/preprocess"
$baseDir = Get-Location
$inputImage = "$baseDir\shared_volume\uploads\wrtFileImageView.jpg"

# 2. Check Input File
if (-not (Test-Path $inputImage)) {
    Write-Host "Error: Input file not found at $inputImage" -ForegroundColor Red
    exit 1
}

Write-Host "Input Image: $inputImage" -ForegroundColor Gray

# 3. Send API Request
$body = @{
    imagePath = $inputImage
} | ConvertTo-Json

Write-Host "Calling API ($serverUrl)..." -ForegroundColor Yellow

try {
    $response = Invoke-RestMethod -Method Post -Uri $serverUrl -Body $body -ContentType "application/json"
    
    Write-Host "API Response Received:" -ForegroundColor Green
    $response | Format-List
    
    # 4. Verify Output File
    $outputPath = $response.processedPath
    
    if (Test-Path $outputPath) {
        Write-Host "✅ SUCCESS: Output file generated at $outputPath" -ForegroundColor Cyan
        $item = Get-Item $outputPath
        Write-Host "   Size: $($item.Length) bytes"
        Write-Host "   Time: $($item.LastWriteTime)"
    } else {
        Write-Host "❌ FAILURE: Output file verification failed!" -ForegroundColor Red
        Write-Host "   Expected path: $outputPath"
    }
} catch {
    Write-Host "❌ API Request Failed. Is the server running?" -ForegroundColor Red
    Write-Host "   Error: $_"
}
