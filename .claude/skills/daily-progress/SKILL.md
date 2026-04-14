---
name: daily-progress
description: Mind Palette 프로젝트의 일일 진행도 리포트 생성 (plan.md 기반)
---

# 📊 Daily Progress 에이전트

당신은 Mind Palette 프로젝트의 진행도 추적 전문 에이전트입니다.
`plan.md`의 130개 TDD 체크리스트를 자동으로 집계하고, 매일 진행도 리포트를 생성합니다.

---

## 리포트 컨텍스트

### 최근 5개 커밋
!`git log --pretty=format:"%h|%ad|%s" --date=short -5 2>/dev/null || echo "커밋 이력 없음"`

### 현재 브랜치
!`git branch --show-current 2>/dev/null || echo "unknown"`

### Plan.md 전체 내용
!`cat plan.md 2>/dev/null || echo "plan.md not found"`

---

## 리포트 옵션

$ARGUMENTS

- **인자가 없으면**: 오늘 날짜로 리포트 생성
- `--date YYYY-MM-DD`: 특정 날짜 기준 리포트 생성
- `--verbose`: 미완료 항목 전체 출력 (상위 3-5개가 아닌 전체)

---

## 리포트 생성 절차

### 1단계: plan.md 파싱

다음 패턴을 사용하여 체크리스트를 분석하세요:

```
- [ ] 패턴: 미완료 항목
- [x] 패턴: 완료된 항목
## 🧠 Phase X: 패턴: Phase 구분
```

**주요 계산**:
- 전체 항목 수 = `- [ ]` + `- [x]` 개수
- 완료 항목 수 = `- [x]` 개수
- 미완료 항목 수 = `- [ ]` 개수
- 전체 진행률 = (완료 / 전체) × 100%

**Phase별 집계**:
각 Phase 섹션 내의 체크박스를 별도로 집계하여 Phase별 진행률 계산

---

### 2단계: 현재 Phase 식별

다음 기준으로 "진행 중" Phase 결정:
1. Phase 4가 미완료 항목을 가지고 있으면 → Phase 4 진행 중
2. 그렇지 않으면 미완료 항목이 가장 많은 Phase → 진행 중

---

### 3단계: 다음 우선 과제 추출

미완료 항목(`- [ ]`) 중 다음 기준으로 상위 3-5개 추출:
1. **Phase 4 항목 우선**: Phase 4의 미완료 항목을 최우선으로
2. **주력(80%) 키워드**: `**주력 (80%):**` 라벨이 있는 항목
3. **최우선 마크**: `[최우선]`, `[즉시]` 키워드가 있는 항목
4. 위 기준에 해당 없으면 파일 순서상 가장 위의 미완료 항목

---

### 4단계: 최근 커밋 테이블 생성

Git log에서 추출한 커밋 정보를 다음 포맷의 마크다운 테이블로 변환:

```
| 해시 | 날짜 | 메시지 |
|------|------|--------|
| 8c2cdb2 | 2026-02-16 | Merge pull request #13... |
```

---

## 출력 템플릿

다음 형식으로 리포트를 생성하세요:

```markdown
# 📊 Daily Progress Report - YYYY년 M월 D일

**브랜치**: main
**리포트 생성 시각**: YYYY-MM-DD HH:MM

---

## 전체 진행률

- **총 항목**: 130개
- **완료**: XX개 (XX.X%)
- **미완료**: XX개 (XX.X%)

**Progress Bar**:
```
[████████████░░░░░░░░░░░░░░░░░░░░] 38.5%
```

---

## Phase별 진행 현황

| Phase | 전체 | 완료 | 미완료 | 진행률 | 상태 |
|-------|------|------|--------|--------|------|
| Phase 1 (Frontend) | XX | XX | XX | XX% | ✅ 완료 |
| Phase 2 (API Gateway) | XX | XX | XX | XX% | ✅ 완료 |
| Phase 3 (C++ Server) | XX | XX | XX | XX% | ✅ 완료 |
| **Phase 4 (Python AI)** | XX | XX | XX | XX% | 🚀 진행 중 |
| Phase 5 (통합) | XX | XX | XX | XX% | ⏳ 대기 |
| Phase 6 (Reliability) | XX | XX | XX | XX% | ⏳ 대기 |

---

## 현재 진행 Phase

- **진행 중**: Phase 4 - Python AI Server
- **최근 업데이트**: YYYY-MM-DD
- **목표 완료일**: 2026년 3월 말
- **경쟁력 점수**: 60점 → 85점 (Phase 4 완성 시)

---

## 최근 5개 커밋

| 해시 | 날짜 | 메시지 |
|------|------|--------|
| XXXXXXX | YYYY-MM-DD | Commit message... |
| XXXXXXX | YYYY-MM-DD | Commit message... |
| XXXXXXX | YYYY-MM-DD | Commit message... |
| XXXXXXX | YYYY-MM-DD | Commit message... |
| XXXXXXX | YYYY-MM-DD | Commit message... |

---

## 다음 우선 과제 (Top 5)

1. **[ ] FastAPI 서버 골격 구축** (Phase 4 Step 1)
   - 경로: `ai-server/src/main.py`
   - 우선도: 최우선 (주력 80%)
   - TDD: `/health` 엔드포인트 테스트 → 구현

2. **[ ] EfficientNet-B2 아키텍처 조사**
   - MCP context7 활용
   - 우선도: 최우선
   - 참고: `docs/reference/AI/ai_model_recommendation.md`

3. **[ ] PyTorch 환경 설정 및 Hello World**
   - 더미 텐서 추론 성공
   - 우선도: 최우선

4. **[ ] C++ 전처리 코드 1개 파일 정독** (복습 20%)
   - 경로: `preprocess-server/src/core/filter_pipeline.h`
   - 우선도: 보통

5. **[ ] GaussianBlur 파라미터 실험** (복습 20%)
   - (3,3) vs (5,5) vs (7,7) 비교
   - 우선도: 보통

---

## 권장 작업 순서

### 오늘 (YYYY-MM-DD)
1. **Phase 4 최우선 항목 1개** 착수 (주력 80%)
2. **복습 항목 20% 시간 할당** (기존 C++ 코드 정독)

### 이번 주
- Phase 4 Step 1 완료 목표 (FastAPI 서버 골격)
- C++ 파라미터 실험 1회 수행

### 이번 달
- Phase 4 Base Model 완성 (PyTorch 모델 구현)
- ONNX 변환 준비

---

## 프로젝트 2-Track 전략

```
Track 1 (주력 80%): Phase 4 AI 서버 개발 진행
  - FastAPI 서버 구축
  - EfficientNet-B2 모델 구현
  - ONNX 변환 준비

Track 2 (복습 20%): C++ 디테일 보강
  - 주말/저녁 1-2시간: OpenCV 파라미터 실험
  - 기존 코드 주석 보강 (왜 이 값인지 근거 추가)
  - 벤치마크 결과 문서화
```

---

**생성일**: YYYY년 M월 D일
**리포터**: Daily Progress Agent
**프로젝트**: Mind Palette
**문서 버전**: 1.0

---

## 자동 저장 지시

위 리포트 내용을 다음 경로에 자동 저장하세요:

- **저장 경로**: `docs/status/PROGRESS/YYYY-MM-DD_daily_progress.md`
- **디렉토리 생성**: `docs/status/PROGRESS/` 디렉토리가 없으면 생성
- **파일명 규칙**: `YYYY-MM-DD_daily_progress.md` (예: `2026-02-21_daily_progress.md`)

저장 후 다음 메시지 출력:
```
✅ 진행도 리포트가 저장되었습니다: docs/status/PROGRESS/YYYY-MM-DD_daily_progress.md
```
```
