# Mind Palette 프로젝트: Git 워크플로우 적용 가이드

> **목표**: 현재 진행 중인 개발 작업에 Feature Branch + PR 워크플로우를 실제로 적용하기

## 📍 현재 프로젝트 상황

### ✅ 완료된 작업
- Phase 1-2: API Gateway, Frontend, CI/CD 구축
- Phase 3 (Week 1-2): C++ 전처리 서버 기본 구현
- **Logging System 완료** (Winston + spdlog)

### 🚧 진행 중인 작업
- Phase 3 (Week 3): GrabCut 배경 제거 구현
- Phase 4: Traffic & Load Testing 준비

### 📋 다음 작업
- GrabCut 알고리즘 적용
- Canny 엣지 검출
- 멀티스레딩 최적화

---

## 🎯 적용 전략

### 원칙
1. **과거 작업은 그대로 둔다** (이미 main에 있는 코드)
2. **새로운 작업부터 적용** (GrabCut, Traffic Testing 등)
3. **작은 단위로 시작** (큰 기능을 작은 PR로 나누기)

---

## 📝 실전 적용 시나리오

### 시나리오 1: 다음 작업 (Traffic Bot 구현)

현재 `task.md`에 따르면:
```markdown
## Phase 4: Traffic & Load Testing Setup
- [ ] Node.js 기반 트래픽 봇 스크립트 작성
- [ ] k6 설치 및 부하 테스트 스크립트 작성
```

#### Step-by-Step 적용:

**1. 브랜치 생성**
```bash
# main 브랜치 최신화
git checkout main
git pull origin main

# 새 브랜치 생성
git checkout -b feature/traffic-bot
```

**2. TDD 사이클 시작**

**Red (실패하는 테스트 작성):**
```bash
# api-gateway/tests/traffic-bot.test.js 생성
# ... 테스트 코드 작성 ...

git add api-gateway/tests/traffic-bot.test.js
git commit -m "test(api-gateway): add traffic bot tests (failing)"
git push -u origin feature/traffic-bot
```

**3. GitHub에서 Draft PR 생성**

웹 브라우저에서:
- "Compare & pull request" 클릭
- **"Create draft pull request" 선택** (아직 작업 중이므로)
- PR 템플릿 작성:

```markdown
## 📝 변경사항 요약
Node.js 기반 트래픽 생성 봇 구현

## 🔍 변경 이유
- 로깅 시스템 테스트 (파일 로테이션, Request ID 전파)
- 성능 벤치마크 데이터 수집

## 🧪 테스트 방법
- [x] 단위 테스트 작성 (현재 실패)
- [ ] 구현 완료 후 테스트 통과 확인
- [ ] 실제 트래픽 생성 테스트

## ✅ 체크리스트
TDD 진행 상황:
- [x] Red: 테스트 작성 (failing)
- [ ] Green: 최소 구현
- [ ] Refactor: 리팩터링
- [ ] 문서 업데이트
```

**Green (테스트 통과하는 코드):**
```bash
# api-gateway/scripts/traffic-bot.js 생성
# ... 구현 코드 작성 ...

git add api-gateway/scripts/traffic-bot.js
git commit -m "feat(api-gateway): implement basic traffic bot"
git push
```

**Refactor (리팩터링):**
```bash
# 코드 정리, 중복 제거 등
git add .
git commit -m "refactor(api-gateway): extract http client logic"
git push
```

**4. Draft → Ready for Review**

모든 체크리스트가 완료되면:
- GitHub PR 페이지에서 "Ready for review" 클릭
- Self-Review 진행
- 문제 없으면 Merge

**5. 정리**
```bash
git checkout main
git pull origin main
git branch -d feature/traffic-bot
```

---

### 시나리오 2: 현재 작업 (GrabCut 구현)

만약 지금 GrabCut 작업 중이라면:

#### 옵션 A: 지금 바로 브랜치로 옮기기

```bash
# 1. 현재 변경사항 확인
git status

# 2. 변경사항이 있다면 stash
git stash

# 3. 새 브랜치 생성
git checkout -b feature/grabcut-background-removal

# 4. stash한 변경사항 복원
git stash pop

# 5. 커밋
git add .
git commit -m "feat(preprocess): add GrabCut initial implementation"
git push -u origin feature/grabcut-background-removal

# 6. Draft PR 생성 (GitHub 웹사이트에서)
```

#### 옵션 B: 다음 작업부터 적용 (권장)

현재 작업은 main에 그대로 커밋하고, **다음 작업부터** 브랜치 전략 적용

---

## 🗂️ 브랜치 네이밍 예시 (프로젝트별)

### Node.js (API Gateway)
```
feature/traffic-bot           # 트래픽 봇 구현
feature/request-id-tracking   # Request ID 추적
test/load-testing-k6          # k6 부하 테스트
docs/api-documentation        # API 문서화
```

### C++ (Preprocess Server)
```
feature/grabcut-background-removal   # GrabCut 배경 제거
feature/canny-edge-detection         # Canny 엣지 검출
perf/thread-pool-optimization        # 스레드 풀 최적화
refactor/strategy-pattern-filters    # Strategy Pattern 적용
test/performance-benchmarking        # 성능 벤치마크
```

### 공통
```
fix/image-upload-error        # 버그 수정
docs/update-architecture      # 아키텍처 문서 업데이트
chore/update-dependencies     # 의존성 업데이트
```

---

## 🔄 TDD + PR 워크플로우 통합

### 완벽한 사이클:

```
1. 브랜치 생성
   git checkout -b feature/my-feature

2. [RED] 실패하는 테스트 작성
   git commit -m "test: add my-feature tests (failing)"
   git push -u origin feature/my-feature
   
   → GitHub에서 Draft PR 생성
   → Title: "[WIP] My Feature"
   → 체크리스트: [x] Red, [ ] Green, [ ] Refactor

3. [GREEN] 최소 구현
   git commit -m "feat: implement my-feature (minimal)"
   git push
   
   → PR 자동 업데이트
   → 체크리스트: [x] Red, [x] Green, [ ] Refactor

4. [REFACTOR] 리팩터링
   git commit -m "refactor: improve my-feature structure"
   git push
   
   → PR 자동 업데이트
   → 체크리스트: [x] Red, [x] Green, [x] Refactor

5. Ready for Review
   → GitHub에서 "Ready for review" 클릭
   → Self-Review 진행
   → Merge

6. 정리
   git checkout main
   git pull origin main
   git branch -d feature/my-feature
```

---

## 📊 작업 분할 가이드

### ✅ 좋은 PR (작은 단위)

**예시 1: GrabCut 구현을 3개 PR로 나누기**

```
PR #1: feature/grabcut-initial-mask
- GrabCut 초기 마스크 생성 로직
- 테스트: 마스크 크기 검증
- 예상 작업 시간: 2-3시간

PR #2: feature/grabcut-algorithm
- GrabCut 알고리즘 적용
- 테스트: iterCount별 품질 검증
- 예상 작업 시간: 3-4시간

PR #3: feature/grabcut-optimization
- 성능 최적화 (파라미터 튜닝)
- 벤치마크 테스트
- 예상 작업 시간: 2시간
```

**예시 2: Traffic Testing을 2개 PR로 나누기**

```
PR #1: feature/traffic-bot
- Node.js 트래픽 생성 봇
- 테스트 포함
- 예상 작업 시간: 2시간

PR #2: test/k6-load-testing
- k6 스크립트 작성
- 성능 벤치마크 설정
- 예상 작업 시간: 3시간
```

### ❌ 나쁜 PR (너무 큰 단위)

```
PR #1: feature/entire-week3-implementation
- GrabCut 전체
- Canny 엣지 검출
- 모폴로지 연산
- 모든 테스트
- 문서화
→ 너무 큼! 리뷰 어려움!
```

---

## 🎯 첫 번째 실습 추천

### 간단한 작업으로 시작하기

**실습 1: Traffic Bot 구현 (추천!)**

**이유:**
- ✅ 상대적으로 작은 작업 (2-3시간)
- ✅ 독립적인 기능 (다른 코드에 영향 적음)
- ✅ TDD 연습하기 좋음
- ✅ Node.js라서 빌드 시간 짧음

**진행 방법:**
```bash
# 1. 브랜치 생성
git checkout -b feature/traffic-bot

# 2. 테스트 작성
# api-gateway/tests/traffic-bot.test.js

# 3. Draft PR 생성 (GitHub 웹)

# 4. TDD 사이클 진행
# Red → Green → Refactor

# 5. Ready for Review → Merge
```

---

## 📝 Conventional Commits 실전 예시

### 프로젝트별 커밋 메시지

**Node.js (API Gateway):**
```bash
feat(api-gateway): add traffic generation bot
test(api-gateway): add traffic bot unit tests
fix(api-gateway): resolve request timeout issue
docs(api-gateway): update traffic testing guide
chore(api-gateway): update winston to v3.11.0
```

**C++ (Preprocess Server):**
```bash
feat(preprocess): implement GrabCut background removal
test(preprocess): add GrabCut iteration tests
perf(preprocess): optimize thread pool allocation
refactor(preprocess): extract filter strategy pattern
fix(preprocess): resolve memory leak in GrabCut
```

**한글 사용 (팀 내부용):**
```bash
feat(preprocess): GrabCut 배경 제거 구현
test(api-gateway): 트래픽 봇 테스트 추가
docs: Git 워크플로우 가이드 업데이트
```

---

## 🔐 Branch Protection (선택사항)

### GitHub 설정 (나중에 적용 가능)

혼자 개발 중이라면 **당장은 필요 없지만**, 나중에 팀 프로젝트로 확장할 때:

**Settings → Branches → Add rule:**
- Branch name pattern: `main`
- ✅ Require pull request before merging
- ✅ Require status checks to pass (CI 통과 필수)
- ❌ Include administrators (혼자 개발 중이면 체크 안 함)

---

## 🎓 단계별 로드맵

### Week 1: 익숙해지기
- [ ] Git 워크플로우 가이드 읽기
- [ ] 실습 1 따라하기 (README 수정)
- [ ] PR 템플릿 체험하기

### Week 2: 실제 적용
- [ ] Traffic Bot을 Feature Branch로 구현
- [ ] Draft PR 사용해보기
- [ ] TDD 사이클 + PR 통합

### Week 3: 숙련
- [ ] GrabCut 구현을 여러 PR로 나누기
- [ ] Self-Review 습관화
- [ ] Conventional Commits 자연스럽게 사용

### Week 4: 고급
- [ ] Git Rebase 연습
- [ ] Branch Protection 설정
- [ ] CI/CD와 PR 통합

---

## 💡 팁 & 트릭

### 1. PR 크기 조절

```bash
# PR이 너무 커지면 (커밋 10개 이상)
→ 다른 브랜치로 분리

git checkout -b feature/my-feature-part2
git cherry-pick <commit-hash>  # 필요한 커밋만 선택
```

### 2. 커밋 메시지 템플릿

```bash
# .git/commit-template 생성
feat(scope): 

# 왜 이 변경이 필요한가?

# 무엇을 바꿨는가?

# 저장
git config commit.template .git/commit-template

# 이제 git commit만 하면 템플릿이 나타남
git commit
```

### 3. GitHub CLI 활용 (선택)

```bash
# GitHub CLI 설치 후
gh pr create --draft --title "[WIP] Traffic Bot" --body "작업 중..."
gh pr ready  # Draft → Ready
gh pr merge  # PR Merge
```

---

## 🚨 주의사항

### 1. 절대 하지 말 것

```bash
# ❌ main 브랜치에서 직접 작업
git checkout main
git commit -m "add feature"  # 위험!

# ✅ 항상 브랜치 생성
git checkout -b feature/my-feature
```

### 2. 커밋 전 확인

```bash
# 무엇이 변경되었는지 항상 확인
git diff
git status

# 의도하지 않은 파일이 있다면
git reset HEAD <file>  # unstage
```

### 3. PR 생성 전 체크리스트

- [ ] 모든 테스트 통과? (`npm test`, `ctest`)
- [ ] 린트 오류 없음? (`npm run lint`)
- [ ] 커밋 메시지 Conventional Commits 준수?
- [ ] .gitignore 확인? (로그 파일 등)

---

## 🎉 다음 단계

이제 준비가 되었습니다!

**바로 시작하기:**
1. 📖 [`git-workflow-guide.md`](./git-workflow-guide.md) 읽기
2. 🧪 [`git-workflow-practice.md`](./git-workflow-practice.md)의 실습 1 따라하기
3. 🚀 Traffic Bot 구현을 첫 Feature Branch로 시작하기

**질문이 생기면:**
- 가이드 문서 참고
- GitHub PR 템플릿의 안내 따르기
- 막히면 언제든지 물어보기!

---

**Good luck! 🚀**
