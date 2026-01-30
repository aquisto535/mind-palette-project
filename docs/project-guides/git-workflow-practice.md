# Git 워크플로우 실습 가이드

> **목표**: 실제로 Feature Branch + Pull Request 워크플로우를 연습해보기

## 🎯 이 가이드의 목적

이론은 충분히 봤으니, 이제 실제로 **손으로 직접 해보면서** 익히는 시간입니다!

---

## 📝 실습 1: 첫 번째 Feature Branch 만들어보기

### 목표
간단한 README 수정을 Feature Branch로 작업하고 PR을 만들어보기

### 단계별 실습

#### Step 1: 현재 상태 확인

```bash
# 현재 어느 브랜치에 있는지 확인
git branch

# main 브랜치로 이동
git checkout main

# 최신 상태로 업데이트
git pull origin main
```

**✅ 체크포인트:**
- `* main` (별표가 main 옆에 있어야 함)
- "Already up to date" 또는 최신 변경사항 받기

#### Step 2: 새 브랜치 생성

```bash
# feature 브랜치 생성 및 이동
git checkout -b feature/update-readme

# 브랜치 확인
git branch
```

**✅ 체크포인트:**
- `* feature/update-readme` (별표가 새 브랜치에 있어야 함)

#### Step 3: 파일 수정

**README.md 파일 열기**

기존:
```markdown
# Mind Palette Project
```

수정:
```markdown
# Mind Palette Project

> AI 기반 아동 인물화 지능측정 시스템

## 주요 기능
- 이미지 전처리 (C++ + OpenCV)
- API Gateway (Node.js + Express)
- AI 분석 (Python - 예정)
```

**저장하기** (Ctrl + S)

#### Step 4: 변경사항 확인

```bash
# 무엇이 바뀌었는지 확인
git status

# 자세한 변경 내용 보기
git diff
```

**✅ 체크포인트:**
- `modified: README.md` 표시됨
- 빨간색으로 변경사항 보임

#### Step 5: 커밋

```bash
# 파일 스테이징
git add README.md

# 상태 확인 (색깔이 초록색으로 바뀜)
git status

# 커밋
git commit -m "docs: update README with project description"
```

**✅ 체크포인트:**
- "1 file changed, X insertions(+)" 메시지 확인

#### Step 6: GitHub에 푸시

```bash
# 처음 푸시 (upstream 설정)
git push -u origin feature/update-readme
```

**✅ 체크포인트:**
- "Branch 'feature/update-readme' set up to track remote branch" 메시지
- GitHub 링크 출력됨

#### Step 7: GitHub에서 PR 생성

**웹 브라우저로 이동:**

1. GitHub 저장소 페이지 열기
2. 상단에 노란색 배너가 나타남:
   ```
   feature/update-readme had recent pushes
   [Compare & pull request]
   ```
3. "Compare & pull request" 버튼 클릭
4. PR 템플릿 작성:

```markdown
## 📝 변경사항 요약
README에 프로젝트 설명과 주요 기능 추가

## 🔍 변경 이유
프로젝트 overview가 필요하다고 판단

## 🧪 테스트 방법
- [x] 수동 테스트 완료 (README 읽어보기)

## ✅ 체크리스트
- [x] 모든 테스트가 통과했습니다
- [x] 린트 오류가 없습니다
- [x] 커밋 메시지가 Conventional Commits 규칙을 따릅니다
```

5. "Create pull request" 클릭

#### Step 8: Self-Review

1. **"Files changed" 탭 클릭**
2. 변경사항 확인:
   - 빨간색 줄: 삭제
   - 초록색 줄: 추가
3. 코드 줄에 마우스 올리면 **+ (코멘트 추가)** 버튼 나타남
4. 문제가 없다면 넘어가기

#### Step 9: Merge

1. **"Conversation" 탭으로 돌아가기**
2. 아래로 스크롤
3. **"Merge pull request"** 버튼 클릭
4. **"Confirm merge"** 클릭
5. **"Delete branch"** 클릭 (선택사항, 권장)

**✅ 체크포인트:**
- 보라색 "Merged" 뱃지 표시
- "Pull request successfully merged and closed" 메시지

#### Step 10: 로컬 정리

```bash
# main 브랜치로 이동
git checkout main

# 최신 변경사항 가져오기
git pull origin main

# 불필요한 브랜치 삭제
git branch -d feature/update-readme
```

**✅ 체크포인트:**
- README.md에 새로운 내용이 반영되어 있음
- `git branch`로 확인하면 feature 브랜치가 사라짐

---

## 🎯 실습 2: 코드 변경 + 테스트

### 목표
실제 코드를 수정하고 테스트까지 포함한 PR 만들기

### 시나리오
API Gateway에 새로운 엔드포인트 `/version` 추가

#### Step 1: 브랜치 생성

```bash
git checkout main
git pull origin main
git checkout -b feature/add-version-endpoint
```

#### Step 2: 코드 작성

**`api-gateway/server.js`에 추가:**

```javascript
// 기존 코드 아래에 추가
app.get('/version', (req, res) => {
  res.json({
    version: '1.0.0',
    name: 'Mind Palette API Gateway',
    uptime: process.uptime()
  });
});
```

#### Step 3: 테스트 작성

**`api-gateway/tests/version.test.js` 생성:**

```javascript
const request = require('supertest');
const express = require('express');

describe('GET /version', () => {
  let app;

  beforeAll(() => {
    app = express();
    app.get('/version', (req, res) => {
      res.json({
        version: '1.0.0',
        name: 'Mind Palette API Gateway',
        uptime: process.uptime()
      });
    });
  });

  it('should return version information', async () => {
    const response = await request(app).get('/version');
    
    expect(response.statusCode).toBe(200);
    expect(response.body).toHaveProperty('version');
    expect(response.body).toHaveProperty('name');
    expect(response.body).toHaveProperty('uptime');
  });
});
```

#### Step 4: 테스트 실행

```bash
cd api-gateway
npm test
cd ..
```

**✅ 체크포인트:**
- 모든 테스트 통과 (PASS)

#### Step 5: 커밋 (여러 개로 나누기)

```bash
# 1번째 커밋: 코드 추가
git add api-gateway/server.js
git commit -m "feat(api-gateway): add /version endpoint"

# 2번째 커밋: 테스트 추가
git add api-gateway/tests/version.test.js
git commit -m "test(api-gateway): add tests for /version endpoint"
```

#### Step 6: 푸시 & PR

```bash
git push -u origin feature/add-version-endpoint
```

GitHub에서 PR 생성 (실습 1과 동일)

**PR 템플릿 작성 예시:**

```markdown
## 📝 변경사항 요약
- `/version` 엔드포인트 추가
- 버전 정보, 서버 이름, uptime 반환

## 🔍 변경 이유
서버 상태와 버전 확인을 위해

## 🧪 테스트 방법
- [x] 단위 테스트 실행 (`npm test`)
- [x] 수동 테스트 완료

## ✅ 체크리스트
- [x] 모든 테스트가 통과했습니다
- [x] 린트 오류가 없습니다
- [x] 커밋 메시지가 Conventional Commits 규칙을 따릅니다
- [x] 코드에 주석을 추가했습니다
```

---

## 🚨 일반적인 실수와 해결법

### 실수 1: main 브랜치에 직접 커밋

**증상:**
```bash
git branch
# * main  ← 아차! main에 있었네!
```

**해결:**
```bash
# 1. 커밋 취소 (변경사항은 유지)
git reset HEAD~1

# 2. 새 브랜치 생성
git checkout -b feature/my-feature

# 3. 다시 커밋
git add .
git commit -m "feat: add my feature"
```

### 실수 2: 커밋 메시지를 잘못 씀

**증상:**
```bash
git commit -m "update code"  # ← 너무 모호함!
```

**해결:**
```bash
# 마지막 커밋 메시지 수정
git commit --amend -m "feat: add user authentication"
```

### 실수 3: 잘못된 파일까지 커밋

**증상:**
```bash
git status
# modified: src/app.js
# modified: .env  ← 이건 커밋하면 안 되는데!
```

**해결:**
```bash
# 특정 파일만 스테이징
git add src/app.js

# .env는 .gitignore에 추가
echo ".env" >> .gitignore
```

### 실수 4: 브랜치 이름을 잘못 지음

**증상:**
```bash
git branch
# * feature-logging  ← 하이픈(-) 대신 언더스코어(_)를 써야 했는데!
```

**해결:**
```bash
# 브랜치 이름 변경
git branch -m feature/logging-system
```

### 실수 5: 충돌(Conflict) 발생

**증상:**
```bash
git merge main
# CONFLICT (content): Merge conflict in server.js
```

**해결:**
```bash
# 1. 충돌 파일 열기
# <<<<<<< HEAD
# 내 코드
# =======
# main의 코드
# >>>>>>> main

# 2. 수동으로 해결 (원하는 대로 편집)

# 3. 해결 완료 표시
git add server.js
git commit -m "merge: resolve conflict in server.js"
```

---

## 📊 진도 체크리스트

완료한 항목에 ✅ 표시하세요:

- [ ] 실습 1: README 수정 PR 완료
- [ ] 실습 2: 코드 + 테스트 PR 완료
- [ ] Self-Review를 통해 실수 발견 경험
- [ ] Conventional Commits 규칙 3번 이상 사용
- [ ] 브랜치 생성/삭제 5번 이상 반복
- [ ] PR Merge 성공 경험
- [ ] 충돌 해결 경험 (선택사항)

---

## 🎓 다음 단계

이제 워크플로우에 익숙해졌다면:

1. **실제 개발에 적용하기**
   - 모든 새 기능은 Feature Branch로
   - PR 없이는 main에 합치지 않기

2. **고급 기능 배우기**
   - Git Rebase
   - Cherry-pick
   - Stash

3. **팀 협업 연습**
   - Code Review 주고받기
   - PR에 코멘트 달기
   - CI/CD 연동

---

## 💡 팁

### 빠른 워크플로우 (익숙해진 후)

```bash
# 1줄로 브랜치 생성 + 파일 수정 + 커밋 + 푸시
git checkout -b feature/quick && \
  # (파일 수정) && \
  git add . && \
  git commit -m "feat: quick feature" && \
  git push -u origin feature/quick
```

### Git 별칭(Alias) 설정

```bash
# 자주 쓰는 명령어를 짧게
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.cm commit
git config --global alias.st status

# 이제 이렇게 사용 가능
git co main
git br
git cm -m "feat: add feature"
git st
```

---

## 🎉 축하합니다!

이제 당신은:
- ✅ Feature Branch를 만들 수 있습니다
- ✅ Pull Request를 생성할 수 있습니다
- ✅ Self-Review를 할 수 있습니다
- ✅ Conventional Commits를 사용할 수 있습니다
- ✅ 실무와 동일한 워크플로우를 연습했습니다

**다음 커밋부터 바로 적용해보세요!** 🚀
