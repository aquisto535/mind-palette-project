#!/bin/bash
# Stop: Claude 응답 종료 시 세션 체크포인트 제안
# 트리거: 각 응답 종료 후 (중요한 작업 이후 컨텍스트 보존용)

echo "📋 [session-checkpoint] 세션 체크포인트"
echo "   • 중요한 결정사항 → memory/MEMORY.md에 기록했나요?"
echo "   • plan.md 진행상황이 업데이트되었나요?"
echo "   • 컨텍스트 한계 근접 시 /compact 실행을 고려하세요"
