#!/bin/bash
# PreToolUse: Write 전 민감한 파일 또는 시크릿 패턴 감지
# 트리거: .env 파일 또는 시크릿이 포함된 파일 쓰기 시

INPUT=$(cat)

# .env 파일 쓰기 감지
if echo "$INPUT" | grep -qiE '"file_path":"[^"]*\.env[^"]*"'; then
    echo "🔒 [secret-guard] .env 파일 쓰기 감지됨"
    echo "   확인 사항:"
    echo "   • .gitignore에 .env 파일이 포함되어 있는가?"
    echo "   • 실제 시크릿 값이 하드코딩되어 있지 않은가?"
fi

# 내용에 시크릿 패턴 감지 (API 키, 비밀번호 등)
if echo "$INPUT" | grep -qiE '(API_KEY|SECRET_KEY|PASSWORD|PRIVATE_KEY|ACCESS_TOKEN)\s*=\s*[^$\{][^\s]{8,}'; then
    echo "🚨 [secret-guard] 잠재적 시크릿 값이 감지됨"
    echo "   환경 변수 또는 별도 설정 파일로 분리하는 것을 권장합니다"
    echo "   예: os.environ.get('API_KEY') 또는 process.env.API_KEY"
fi
