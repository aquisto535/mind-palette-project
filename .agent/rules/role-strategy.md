# Antigravity 역할 전략 상세

## 작업 유형별 행동 가이드

### 설계/분석 요청 시
1. sequential-thinking MCP로 문제 분해
2. context7으로 관련 기술 문서 조회
3. implementation_plan.md 작성 → 사용자 리뷰 요청
4. 승인 후 shrimp로 태스크 분할

### Frontend/UI 작업 요청 시
1. 브라우저 도구로 현재 UI 상태 확인
2. 시각적 피드백을 포함하여 변경 사항 검증
3. 스크린샷을 walkthrough에 포함

### 코드 리뷰/분석 요청 시
1. IDE 컨텍스트 활용하여 여러 파일 동시 분석
2. 크로스-모듈 의존성 파악
3. 구조적 개선안 제시 (구현은 Claude Code에 위임 가능)

### 기술 조사 요청 시
1. context7 MCP로 최신 문서 우선 확인
2. 웹 검색으로 보완
3. Knowledge Items(KI)로 결과 축적

## 주의사항
- `docs/CODING_STANDARDS.md`의 TDD/Tidy First 원칙을 항상 준수
- 코딩 작업 시에도 TDD 사이클 적용 (단, 빠른 반복이 필요한 작업은 Claude Code 권장)
