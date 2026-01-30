# Git 워크플로우 가이드

> **목표**: main 브랜치에 직접 커밋하지 않고, Feature Branch + Pull Request 방식으로 협업 연습하기

## 📚 목차

1. [기본 개념](#기본-개념)
2. [브랜치 전략](#브랜치-전략)
3. [전체 워크플로우](#전체-워크플로우)
4. [Conventional Commits](#conventional-commits)
5. [Self-Review 체크리스트](#self-review-체크리스트)
6. [실습 예제](#실습-예제)
7. [자주 묻는 질문](#자주-묻는-질문)

---

## 기본 개념

### Pull Request(PR)란?

```
당신의 브랜치 (feature/logging)  ─── "이 코드 합쳐줘!" ───>  main 브랜치
                                   (Pull Request)
```

**왜 필요한가?**
- ✅ **자기 검토**: 변경사항을 한눈에 확인
- ✅ **기록 관리**: "왜 이렇게 바꿨는지" 문서화
- ✅ **실수 방지**: 합치기 전에 문제 발견
- ✅ **실무 연습**: 회사에서 쓰는 방식 그대로

---

## 브랜치 전략

### GitHub Flow (단순화 버전)

```
main (항상 배포 가능한 상태)
  ↑
  │ Pull Request & Merge
  │
feature/logging-system (개발 중)
```

### 브랜치 네이밍 규칙

| 타입 | 형식 | 예시 |
|------|------|------|
| 새 기능 | `feature/기능명` | `feature/logging-system` |
| 버그 수정 | `fix/버그명` | `fix/image-upload-error` |
| 문서 | `docs/문서명` | `docs/update-readme` |
| 리팩터링 | `refactor/대상` | `refactor/api-routes` |
| 테스트 | `test/테스트명` | `test/add-unit-tests` |

**네이밍 규칙:**
- 소문자만 사용
- 단어는 하이픈(`-`)으로 구분
- 영어 또는 영어+한글 혼용 가능

---

## 전체 워크플로우

### 0️⃣ 시작 전 준비

```bash
# main 브랜치가 최신 상태인지 확인
git checkout main
git pull origin main
```

### 1️⃣ Feature 브랜치 생성

```bash
# 새 브랜치 생성 및 이동
git checkout -b feature/logging-system

# 현재 브랜치 확인
git branch
# * feature/logging-system
#   main
```

### 2️⃣ 코드 작성 & 커밋

```bash
# 변경사항 확인
git status

# 파일 스테이징
git add .

# 또는 특정 파일만
git add src/utils/logger.js

# 커밋 (Conventional Commits 규칙 준수)
git commit -m "feat: add winston logger utility"

# 여러 번 커밋 가능
git commit -m "test: add logger unit tests"
git commit -m "docs: update logging documentation"
```

### 3️⃣ GitHub에 푸시

```bash
# 처음 푸시할 때
git push -u origin feature/logging-system

# 이후부터는
git push
```

### 4️⃣ Pull Request 생성

**GitHub 웹사이트에서:**

1. 저장소 페이지로 이동
2. "Compare & pull request" 버튼 클릭
3. PR 템플릿 작성:
   - 변경사항 요약
   - 테스트 방법
   - 체크리스트 확인
4. "Create pull request" 클릭

### 5️⃣ Self-Review

**'Files changed' 탭에서 확인:**

```diff
# 빨간색 (-): 삭제된 코드
- console.log('Server started');

# 초록색 (+): 추가된 코드
+ logger.info('Server started');
```

**체크 포인트:**
- [ ] 불필요한 변경사항 없는지 (공백, 디버그 코드 등)
- [ ] 모든 테스트 통과했는지
- [ ] 커밋 메시지가 명확한지
- [ ] 코드에 주석이 충분한지

### 6️⃣ Merge

**문제가 없다면:**

1. "Merge pull request" 버튼 클릭
2. Merge 방식 선택:
   - **Merge commit** (기본) - 모든 커밋 히스토리 유지
   - **Squash and merge** - 여러 커밋을 하나로 합치기
   - **Rebase and merge** - 깔끔한 히스토리
3. "Confirm merge" 클릭

### 7️⃣ 브랜치 정리

```bash
# 로컬에서 main으로 이동
git checkout main

# 최신 변경사항 가져오기
git pull origin main

# 작업 완료한 브랜치 삭제
git branch -d feature/logging-system

# 원격 브랜치도 삭제 (GitHub에서 자동 삭제 안 된 경우)
git push origin --delete feature/logging-system
```

---

## Conventional Commits

### 기본 형식

```
<타입>(<범위>): <제목>

<본문> (선택사항)

<푸터> (선택사항)
```

### 타입별 사용법

| 타입 | 사용 시기 | 예시 |
|------|----------|------|
| `feat` | 새 기능 추가 | `feat: add winston logger` |
| `fix` | 버그 수정 | `fix: resolve image upload error` |
| `docs` | 문서 변경 | `docs: update README with setup guide` |
| `style` | 코드 포맷팅 (동작 변경 X) | `style: format code with prettier` |
| `refactor` | 리팩터링 | `refactor: extract validation logic` |
| `test` | 테스트 추가/수정 | `test: add unit tests for logger` |
| `chore` | 빌드/설정 변경 | `chore: update dependencies` |
| `perf` | 성능 개선 | `perf: optimize image processing` |

### 예시

```bash
# 기본
git commit -m "feat: add logging system"

# 범위 지정
git commit -m "feat(api-gateway): add winston logger"

# 한글 (프로젝트 내부용)
git commit -m "feat: 로깅 시스템 추가"

# 본문 포함
git commit -m "feat: add winston logger

- JSON 형식으로 파일 저장
- 콘솔 출력은 컬러 포맷
- 에러 로그는 별도 파일 분리"

# Breaking Change
git commit -m "feat!: change API response format

BREAKING CHANGE: response now returns { data, error } instead of { result }"
```

### 커밋 메시지 작성 팁

✅ **좋은 예시:**
```bash
feat: add user authentication
fix: resolve null pointer in image processor
docs: add API documentation for /analyze endpoint
```

❌ **나쁜 예시:**
```bash
update code
fix bug
asdf
WIP
```

---

## Self-Review 체크리스트

### 📋 코드 품질

- [ ] 모든 테스트가 통과하는가?
- [ ] 린트 경고/에러가 없는가?
- [ ] 코드에 불필요한 console.log가 남아있지 않은가?
- [ ] 주석이 충분한가? (특히 복잡한 로직)

### 📋 커밋 히스토리

- [ ] 커밋 메시지가 Conventional Commits 규칙을 따르는가?
- [ ] 각 커밋이 논리적 단위로 분리되어 있는가?
- [ ] 불필요한 커밋이 없는가? (예: "fix typo", "oops")

### 📋 변경사항

- [ ] 의도하지 않은 파일 변경이 없는가?
- [ ] 공백 변경만 있는 라인이 많지 않은가?
- [ ] .gitignore에 추가해야 할 파일은 없는가?

### 📋 문서화

- [ ] README가 업데이트되었는가? (필요한 경우)
- [ ] API 문서가 업데이트되었는가? (필요한 경우)
- [ ] 주요 변경사항이 문서화되었는가?

---

## 실습 예제

### 시나리오: 로깅 시스템 추가하기

```bash
# 1. main 브랜치 최신화
git checkout main
git pull origin main

# 2. 새 브랜치 생성
git checkout -b feature/logging-system

# 3. 코드 작성
# (winston 설치, logger.js 작성 등)

# 4. 커밋
git add api-gateway/src/utils/logger.js
git commit -m "feat(api-gateway): add winston logger utility"

git add api-gateway/server.js
git commit -m "feat(api-gateway): integrate logger into server"

git add api-gateway/package.json
git commit -m "chore: add winston and morgan dependencies"

# 5. 푸시
git push -u origin feature/logging-system

# 6. GitHub에서 PR 생성
# 7. Self-Review 진행
# 8. Merge
# 9. 브랜치 정리
git checkout main
git pull origin main
git branch -d feature/logging-system
```

---

## 자주 묻는 질문

### Q1: PR을 만든 후에 코드를 더 수정하고 싶어요

```bash
# 같은 브랜치에서 계속 작업
git add .
git commit -m "fix: resolve lint errors"
git push

# PR이 자동으로 업데이트됩니다!
```

### Q2: main 브랜치가 업데이트되어서 충돌이 났어요

```bash
# 방법 1: Merge
git checkout feature/my-feature
git merge main
# 충돌 해결
git push

# 방법 2: Rebase (깔끔한 히스토리)
git checkout feature/my-feature
git rebase main
# 충돌 해결
git push -f  # 주의: force push 필요
```

### Q3: 실수로 main에 커밋했어요

```bash
# 커밋 취소 (변경사항은 유지)
git reset HEAD~1

# 새 브랜치 생성
git checkout -b feature/my-feature

# 다시 커밋
git add .
git commit -m "feat: add new feature"
git push -u origin feature/my-feature
```

### Q4: 커밋 메시지를 잘못 썼어요

```bash
# 마지막 커밋 메시지 수정 (아직 push 안 한 경우)
git commit --amend -m "feat: correct commit message"

# 이미 push한 경우 (주의: force push)
git commit --amend -m "feat: correct commit message"
git push -f
```

### Q5: 여러 커밋을 하나로 합치고 싶어요

```bash
# 최근 3개 커밋을 하나로
git rebase -i HEAD~3

# 에디터에서:
# pick → squash로 변경 (첫 번째는 pick 유지)
# 저장 후 종료

git push -f  # force push 필요
```

---

## 🎯 핵심 요약

1. **절대 main에 직접 커밋하지 말 것**
2. **브랜치 이름은 명확하게** (`feature/logging-system`)
3. **커밋 메시지는 Conventional Commits** (`feat:`, `fix:` 등)
4. **PR은 자기 검토의 기회**
5. **작업 완료 후 브랜치 정리**

---

## 📚 참고 자료

- [GitHub Flow 공식 문서](https://docs.github.com/en/get-started/quickstart/github-flow)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Git 브랜치 전략](https://git-scm.com/book/ko/v2/Git-%EB%B8%8C%EB%9E%9C%EC%B9%98-%EB%B8%8C%EB%9E%9C%EC%B9%98%EB%9E%80-%EB%AC%B4%EC%97%87%EC%9D%B8%EA%B0%80)
