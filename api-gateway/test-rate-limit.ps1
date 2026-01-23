# /health 레이트 리미팅 테스트 스크립트 (PowerShell)

echo "의도적으로 65번의 요청을 보냅니다 (제한 수치: 60)..."

for ($i=1; $i -le 65; $i++) {
    $response = Invoke-WebRequest -Uri "http://localhost:3000/health" -SkipHttpErrorCheck
    $status = $response.StatusCode
    echo "요청 ${i}: 응답 코드 $status"
    
    if ($status -eq 429) {
        echo ">>> 성공: 429 Too Many Requests 발생!"
        $content = $response.Content | ConvertFrom-Json
        echo "메시지: $($content.message)"
        break
    }
}
