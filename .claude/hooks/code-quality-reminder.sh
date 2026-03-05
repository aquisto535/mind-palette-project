#!/bin/bash
# PostToolUse: Write/Edit 후 파일 유형별 테스트 리마인더
# 트리거: C++/Python/TypeScript 파일 수정 시

INPUT=$(cat)

# C++ 파일 감지
if echo "$INPUT" | grep -qE '"file_path":"[^"]*\.(cpp|h|hpp|cc)"'; then
    echo "⚠️ [code-quality-reminder] C++ 파일 수정됨"
    echo "   빌드 및 테스트를 확인하세요:"
    echo "   • cmake --build build/"
    echo "   • cd build && ctest --output-on-failure"
fi

# Python 파일 감지
if echo "$INPUT" | grep -qE '"file_path":"[^"]*\.py"'; then
    echo "⚠️ [code-quality-reminder] Python 파일 수정됨"
    echo "   테스트를 확인하세요:"
    echo "   • pytest -v"
fi

# TypeScript 파일 감지
if echo "$INPUT" | grep -qE '"file_path":"[^"]*\.(ts|tsx)"'; then
    echo "⚠️ [code-quality-reminder] TypeScript 파일 수정됨"
    echo "   테스트를 확인하세요:"
    echo "   • npm test"
fi
