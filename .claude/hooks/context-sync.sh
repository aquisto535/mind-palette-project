#!/bin/bash
# PostToolUse: Bash 명령 실행 후 빌드/테스트 완료 감지 → MEMORY.md 업데이트 제안
# 트리거: cmake, ctest, pytest, npm test 명령 실행 후

INPUT=$(cat)

# 빌드/테스트 명령 감지
if echo "$INPUT" | grep -qE '"command":"[^"]*(cmake|ctest|pytest|npm test|make)[^"]*"'; then
    # 성공 또는 실패 여부 감지 (tool_response에 포함된 경우)
    if echo "$INPUT" | grep -q '"exit_code":0'; then
        echo "✅ [context-sync] 빌드/테스트 성공"
    else
        echo "ℹ️ [context-sync] 빌드/테스트 실행 완료"
    fi
    echo "   중요한 결과나 변경사항이 있으면 다음을 업데이트하세요:"
    echo "   • memory/MEMORY.md — 주요 학습 내용이나 결정사항"
    echo "   • plan.md — 완료된 항목 체크"
fi
