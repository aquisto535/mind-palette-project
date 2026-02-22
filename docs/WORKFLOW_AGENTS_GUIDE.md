# Mind Palette 워크플로우 자동화 에이전트 사용 가이드

**작성일**: 2026년 2월 21일
**버전**: 1.0
**프로젝트**: Mind Palette

---

## 📋 개요

Mind Palette 프로젝트는 4가지 워크플로우 자동화 에이전트를 제공하여 개발 생산성을 향상시킵니다:

1. **daily-progress**: 매일 진행도 자동 리포트 생성
2. **refactor-validator**: Tidy First 원칙 준수 검증
3. **phase4-guide**: Phase 4 Python AI Server 개발 가이드
4. **benchmark-reporter**: 벤치마크 결과 분석 및 리포트 생성

모든 에이전트는 Claude Code 스킬 시스템(`.claude/skills/`)으로 구현되었습니다.

---

## 🚀 에이전트 사용 방법

### 1. daily-progress (일일 진행도 리포트)

#### 목적
- `plan.md`의 130개 TDD 체크리스트 진행률 자동 집계
- Phase별 진행 현황 분석
- 다음 우선 과제 추출
- 최근 커밋 히스토리 정리

#### 실행 방법

```bash
# 기본 실행 (오늘 날짜)
/project:daily-progress

# 특정 날짜 기준 리포트
/project:daily-progress --date 2026-02-21

# 상세 모드 (미완료 항목 전체 출력)
/project:daily-progress --verbose
```

#### 출력 위치
- **저장 경로**: `docs/project-status/PROGRESS/YYYY-MM-DD_daily_progress.md`
- **자동 저장**: 예
- **파일명 예시**: `2026-02-21_daily_progress.md`

#### 출력 내용
- 전체 진행률 (완료/미완료 개수 및 퍼센트)
- Phase별 진행 현황 테이블
- 최근 5개 커밋 테이블
- 다음 우선 과제 Top 5
- 권장 작업 순서 (오늘/이번 주/이번 달)

#### 권장 실행 시점
- **매일 저녁** (하루 작업 종료 시)
- 주요 마일스톤 완료 후
- 작업 우선순위를 확인하고 싶을 때

---

### 2. refactor-validator (리팩터링 검증)

#### 목적
- **Tidy First 원칙** 준수 검증 (구조적 변경 ≠ 기능적 변경)
- **TDD Red-Green-Refactor** 사이클 체크
- 언어별 코딩 스타일 검증 (C++17, TypeScript, Python)
- 디자인 패턴 적용 확인

#### 실행 방법

```bash
# Staged 변경사항 검증 (가장 일반적)
git add .
/project:refactor-validator

# 최근 커밋 검증
/project:refactor-validator HEAD~1..HEAD

# 최근 3개 커밋 범위 검증
/project:refactor-validator HEAD~3..HEAD

# 특정 커밋 검증
/project:refactor-validator a1b2c3d

# 테스트 자동 실행 (옵션)
/project:refactor-validator --run-tests
```

#### 출력 위치
- **저장 경로**: `docs/project-status/REFACTOR_VALIDATION/YYYY-MM-DD_refactor_validation.md`
- **자동 저장**: 예
- **파일명 예시**: `2026-02-21_refactor_validation.md`

#### 출력 내용
- TDD 준수도 (Red-Green-Refactor 단계별 체크)
- 변경 유형 분류 (구조적/기능적)
- Tidy First 위반 여부
- 테스트 실행 명령 (C++, Node.js, React)
- 언어별 스타일 체크 결과
- 디자인 패턴 적용 확인
- 종합 평가 및 권장사항

#### 권장 실행 시점
- **리팩터링 커밋 전** (Tidy First 원칙 확인)
- PR 생성 전
- 주요 구조 변경 후

---

### 3. phase4-guide (Phase 4 개발 가이드)

#### 목적
- FastAPI TDD 패턴 제공 (Red-Green-Refactor 예제)
- PyTorch/ONNX 모델 테스팅 전략
- Python 타입 힌팅 가이드 (Pydantic, Mypy)
- AI 서버 특화 코드 리뷰 체크리스트

#### 실행 방법

```bash
# 전체 가이드 보기
/project:phase4-guide

# FastAPI TDD 패턴만
/project:phase4-guide fastapi

# PyTorch 모델 테스팅만
/project:phase4-guide pytorch

# Phase 4 체크리스트만
/project:phase4-guide checklist
```

#### 출력 위치
- **저장 경로**: 없음 (콘솔 출력만)
- **자동 저장**: 아니오
- **이유**: 참고 문서 성격이므로 필요시 수동 저장

#### 출력 내용
- FastAPI TDD 패턴 예제 코드
- PyTorch 모델 테스팅 전략 (Shape 검증, ONNX 일관성, 벤치마크)
- Python 타입 힌팅 예제 (Pydantic 모델, Mypy 설정)
- AI 서버 특화 체크리스트 (모델 구조, 전처리, 최적화, 로깅)
- 추천 디렉토리 구조
- 환경 설정 명령
- TDD 워크플로우 예시

#### 권장 실행 시점
- **Phase 4 개발 시작 전** (패턴 학습)
- Python 코드 작성 중 (참고용)
- FastAPI/PyTorch 테스트 작성 시

---

### 4. benchmark-reporter (벤치마크 리포트)

#### 목적
- C++ 전처리 서버 벤치마크 결과 분석
- 성능 비교 테이블 생성
- 최적 파라미터 추천 (속도/품질/균형)
- 통계 요약 (평균, 표준편차, 변동 계수)

#### 실행 방법

```bash
# 1. 벤치마크 실행 (예시: Canny)
cd preprocess-server/build
./canny_benchmark.exe > ../../benchmark_result.txt

# 2. 리포트 생성 (파일 입력)
/project:benchmark-reporter benchmark_result.txt

# 3. JSON 직접 입력
/project:benchmark-reporter '{"benchmark_name":"canny","measurements":[...]}'

# 4. 테이블 형식 입력 (파이프 구분자)
/project:benchmark-reporter "| Low | High | Edges | ... |"
```

#### 출력 위치
- **저장 경로**: `docs/project-status/BENCHMARKS/YYYY-MM-DD_<benchmark_name>.md`
- **자동 저장**: 예
- **파일명 예시**: `2026-02-21_canny_parameter_tuning.md`

#### 출력 내용
- 성능 비교 테이블 (파라미터별)
- 최적화 추천 (최고 속도/최적 균형/품질 우선)
- 통계 요약 (평균, 표준편차, 변동 계수)
- 시각화 정보 (생성된 이미지 파일 목록)
- 결론 및 액션 아이템
- 참고 자료 (알고리즘 이론, 프로젝트 파일)

#### 권장 실행 시점
- **OpenCV 파라미터 실험 후**
- 성능 최적화 작업 후
- 벤치마크 결과 비교가 필요할 때

---

## 📋 추천 워크플로우

### 매일 저녁 루틴

```bash
# 1. 오늘 진행도 확인
/project:daily-progress

# 2. 오늘 작업한 코드 리뷰
/project:code-review staged
```

**목적**: 하루 작업 정리 및 다음 날 계획 수립

---

### 리팩터링 워크플로우

```bash
# 1. 변경사항 Stage
git add .

# 2. Tidy First 원칙 검증
/project:refactor-validator

# 3. 테스트 수동 실행
cd preprocess-server/build && ctest --output-on-failure
cd api-gateway && npm test
cd frontend && npm test

# 4. 통과하면 커밋
git commit -m "refactor: Extract ImageProcessor logic to PipelineFactory"
```

**목적**: 안전한 리팩터링 보장 (행동 변경 없음 검증)

---

### Phase 4 개발 워크플로우

```bash
# 1. 개발 가이드 확인
/project:phase4-guide

# 2. 개발 진행
# ... FastAPI 코드 작성 ...

# 3. 진행도 업데이트 확인
/project:daily-progress

# 4. 코드 리뷰
/project:code-review staged
```

**목적**: Phase 4 개발 가이드라인 준수

---

### 벤치마크 실험 워크플로우

```bash
# 1. 벤치마크 실행
cd preprocess-server/build
./canny_benchmark.exe > ../../benchmark_result.txt

# 2. 결과 리포트 생성
/project:benchmark-reporter benchmark_result.txt

# 3. 최적 파라미터 적용
# (리포트의 추천에 따라 코드 수정)

# 4. 결과 문서화 (리포트가 자동 저장됨)
```

**목적**: 파라미터 실험 결과 체계적 관리

---

## 🎯 에이전트 활용 팁

### 1. daily-progress 활용
- **매일 저녁 습관화**: 루틴으로 만들어 진행도 추적
- **주간 회고**: 일주일간의 리포트를 비교하여 생산성 분석
- **우선순위 조정**: "다음 우선 과제" 섹션을 참고하여 다음 날 계획

### 2. refactor-validator 활용
- **커밋 전 필수 체크**: 리팩터링 시 Tidy First 위반 방지
- **코드 리뷰 준비**: PR 생성 전 검증 리포트 첨부
- **팀 협업**: 일관된 코딩 스타일 유지

### 3. phase4-guide 활용
- **개발 시작 전 학습**: FastAPI/PyTorch 패턴을 먼저 이해
- **코드 작성 중 참고**: TDD 예제를 보며 구현
- **테스트 작성 가이드**: PyTorch 모델 테스팅 전략 참고

### 4. benchmark-reporter 활용
- **실험 결과 체계화**: 콘솔 출력을 자동으로 마크다운 리포트화
- **파라미터 선택 근거**: 최적 파라미터 추천으로 의사결정 지원
- **성능 회귀 방지**: 과거 리포트와 비교하여 성능 저하 감지

---

## 📁 파일 구조

### 생성된 스킬 파일
```
.claude/
└── skills/
    ├── daily-progress/
    │   └── SKILL.md
    ├── refactor-validator/
    │   └── SKILL.md
    ├── phase4-guide/
    │   └── SKILL.md
    └── benchmark-reporter/
        └── SKILL.md
```

### 리포트 저장 위치
```
docs/
└── project-status/
    ├── PROGRESS/
    │   └── 2026-02-21_daily_progress.md
    ├── REFACTOR_VALIDATION/
    │   └── 2026-02-21_refactor_validation.md
    └── BENCHMARKS/
        └── 2026-02-21_canny_parameter_tuning.md
```

---

## 🔧 문제 해결

### 스킬이 인식되지 않을 때
1. **VSCode 재시작**: Claude Code가 스킬을 다시 로드
2. **파일 경로 확인**: `.claude/skills/<name>/SKILL.md` 존재 확인
3. **YAML 포맷 확인**: frontmatter가 올바른지 확인

### 자동 저장이 안 될 때
1. **디렉토리 존재 확인**: `docs/project-status/PROGRESS/` 등 생성 확인
2. **권한 확인**: 쓰기 권한 확인
3. **경로 오류 확인**: 스킬 출력 마지막의 저장 메시지 확인

### shell preprocessing 오류
1. **Git 상태 확인**: `git status` 실행하여 리포지토리 상태 확인
2. **파일 존재 확인**: `plan.md` 등 필요한 파일 존재 확인
3. **명령 호환성**: Windows에서 `sed`, `find` 등이 작동하는지 확인 (Git Bash 사용 권장)

---

## 📚 참고 자료

### 프로젝트 방법론
- [CLAUDE.md](../CLAUDE.md) - TDD, Tidy First, First Principles 원칙
- [docs/methodology/CODE_REVIEW_GUIDELINES.md](methodology/CODE_REVIEW_GUIDELINES.md) - 코드 리뷰 가이드라인
- [docs/refactoring_strategy.md](refactoring_strategy.md) - Tidy First 리팩터링 전략

### 관련 문서
- [plan.md](../plan.md) - 130개 TDD 체크리스트
- [docs/methodology/PYTHON_FOR_CPP_DEVELOPERS.md](methodology/PYTHON_FOR_CPP_DEVELOPERS.md) - C++ 개발자를 위한 Python 가이드
- [docs/tech-references/AI/ai_model_recommendation.md](tech-references/AI/ai_model_recommendation.md) - EfficientNet-B2 추천 근거

---

## 🎓 학습 경로

### 초급 (프로젝트 시작)
1. `/project:daily-progress` 실행해보기
2. 리포트 형식 이해하기
3. 매일 저녁 루틴으로 습관화

### 중급 (리팩터링 시작)
1. `/project:refactor-validator` 사용법 익히기
2. Tidy First 원칙 이해하기
3. 리팩터링 워크플로우 적용

### 고급 (Phase 4 개발)
1. `/project:phase4-guide` 가이드 학습
2. FastAPI TDD 패턴 적용
3. PyTorch 모델 테스팅 전략 적용

### 전문가 (최적화)
1. `/project:benchmark-reporter` 활용
2. 파라미터 실험 체계화
3. 성능 회귀 방지 시스템 구축

---

**문서 버전**: 1.0
**최종 수정일**: 2026년 2월 21일
**작성자**: Claude Sonnet 4.5
**프로젝트**: Mind Palette
