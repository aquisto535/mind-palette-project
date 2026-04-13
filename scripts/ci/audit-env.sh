#!/usr/bin/env bash
# =============================================================================
# audit-env.sh — 환경 변수 설정 감사 스크립트
#
# 목적: PR/빌드 시 .env.example과 실제 코드의 env 키 사용이 일치하는지 검사
#       + 위험 설정(Mock 모드 등)이 번들에 포함되지 않았는지 검증
#
# 종료 코드:
#   0 — 모든 검사 통과
#   1 — 위반 사항 발견
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VIOLATIONS=0
WARNINGS=0

red()    { echo -e "\033[0;31m$*\033[0m"; }
yellow() { echo -e "\033[0;33m$*\033[0m"; }
green()  { echo -e "\033[0;32m$*\033[0m"; }

echo "============================================================"
echo " Mind Palette — Config Audit"
echo "============================================================"

# ─────────────────────────────────────────────────────────────────────────────
# 검사 1: frontend/.env.example 키 vs 코드에서 사용하는 VITE_* 키 일치 여부
# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "[1/4] Frontend VITE_* key coverage check..."

# 코드에서 실제로 사용하는 VITE_ 키 추출
CODE_KEYS=$(grep -r 'import\.meta\.env\.VITE_' "$REPO_ROOT/frontend/src" \
  --include="*.ts" --include="*.tsx" \
  | grep -oP 'VITE_[A-Z_]+' | sort -u)

# .env.example 에서 선언된 키 추출 (주석 제외)
EXAMPLE_KEYS=$(grep -v '^\s*#' "$REPO_ROOT/frontend/.env.example" \
  | grep -oP 'VITE_[A-Z_]+' | sort -u)

for key in $CODE_KEYS; do
  if ! echo "$EXAMPLE_KEYS" | grep -qx "$key"; then
    red "  ❌ MISSING in frontend/.env.example: $key (used in code)"
    VIOLATIONS=$((VIOLATIONS + 1))
  else
    green "  ✅ $key"
  fi
done

# ─────────────────────────────────────────────────────────────────────────────
# 검사 2: api-gateway/.env.example 키 vs 코드에서 사용하는 process.env.* 키
# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "[2/4] API Gateway process.env key coverage check..."

CODE_KEYS=$(grep -r 'process\.env\.' "$REPO_ROOT/api-gateway/src" \
  --include="*.ts" \
  | grep -oP 'process\.env\.[A-Z_]+' \
  | grep -oP '[A-Z][A-Z_]+' | sort -u)

EXAMPLE_KEYS=$(grep -v '^\s*#' "$REPO_ROOT/api-gateway/.env.example" \
  | grep -oP '^[A-Z][A-Z_]+' | sort -u)

for key in $CODE_KEYS; do
  # NODE_ENV, PORT 같은 표준 Node 변수는 검사 제외
  case "$key" in NODE_ENV|PORT) continue ;; esac

  if ! echo "$EXAMPLE_KEYS" | grep -qx "$key"; then
    yellow "  ⚠️  NOT in api-gateway/.env.example: $key (may need documentation)"
    WARNINGS=$((WARNINGS + 1))
  else
    green "  ✅ $key"
  fi
done

# ─────────────────────────────────────────────────────────────────────────────
# 검사 3: 빌드 아티팩트에서 Mock 모드 잔재 확인
# (frontend/dist 가 존재할 때만 실행)
# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "[3/4] Frontend build artifact — Mock mode check..."

DIST_DIR="$REPO_ROOT/frontend/dist"
if [ -d "$DIST_DIR" ]; then
  # generateMockResult 함수가 번들에 포함되어 있으면서 useMock 조건이 'true'인지 확인
  MOCK_TRUE_COUNT=$(grep -r 'VITE_USE_MOCK.*true\|useMock.*=.*true' "$DIST_DIR" \
    --include="*.js" 2>/dev/null | grep -v '\.map' | wc -l || true)

  if [ "$MOCK_TRUE_COUNT" -gt 0 ]; then
    red "  ❌ Build artifact contains VITE_USE_MOCK=true reference ($MOCK_TRUE_COUNT occurrence(s))"
    red "     Production build must NOT include Mock mode!"
    VIOLATIONS=$((VIOLATIONS + 1))
  else
    green "  ✅ No Mock=true in build artifact"
  fi
else
  yellow "  ⚠️  frontend/dist not found — skipping artifact check (run 'npm run build' first)"
  WARNINGS=$((WARNINGS + 1))
fi

# ─────────────────────────────────────────────────────────────────────────────
# 검사 4: .env.example 파일 존재 여부
# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "[4/4] .env.example existence check..."

for service in frontend api-gateway ai-server; do
  EXAMPLE_FILE="$REPO_ROOT/$service/.env.example"
  if [ -f "$EXAMPLE_FILE" ]; then
    green "  ✅ $service/.env.example exists"
  else
    red "  ❌ MISSING: $service/.env.example"
    VIOLATIONS=$((VIOLATIONS + 1))
  fi
done

# ─────────────────────────────────────────────────────────────────────────────
# 결과 요약
# ─────────────────────────────────────────────────────────────────────────────
echo ""
echo "============================================================"
if [ "$VIOLATIONS" -gt 0 ]; then
  red " AUDIT FAILED: $VIOLATIONS violation(s), $WARNINGS warning(s)"
  echo "============================================================"
  exit 1
else
  green " AUDIT PASSED: 0 violations, $WARNINGS warning(s)"
  echo "============================================================"
  exit 0
fi
