# CI/CD 파이프라인 및 통합 가이드

> **작성일**: 2024-12-06  
> **프로젝트**: Mind Palette - 아동 인물화 지능측정 AI 시스템  
> **목적**: 프론트엔드-백엔드 연동, CI/CD 파이프라인 구축, 보안 의존성 관리에 대한 종합 가이드

---

## 📋 목차

1. [Frontend-Backend 연동](#1-frontend-backend-연동)
2. [CI/CD 파이프라인 구축](#2-cicd-파이프라인-구축)
3. [GitHub Actions 버전 관리](#3-github-actions-버전-관리)
4. [Node.js 의존성 보안 관리](#4-nodejs-의존성-보안-관리)
5. [로컬 개발 환경 설정](#5-로컬-개발-환경-설정)
6. [트러블슈팅](#6-트러블슈팅)

---

## 1. Frontend-Backend 연동

### 1.1 문제 상황
- Netlify에 배포된 프론트엔드는 백엔드 없이 Mock 데이터로만 작동
- 로컬 개발 시에는 실제 API와 연동하여 테스트 필요
- 환경에 따라 다른 동작 방식이 요구됨

### 1.2 해결 방법: 환경변수 기반 분기

**`frontend/src/api/uploadApi.ts` 수정**

```typescript
export const uploadImage = async (file: File, childInfo: ChildInfo | null): Promise<AnalysisResult> => {
  // 환경 변수로 Mock 모드 제어 (기본값: true)
  const useMock = import.meta.env.VITE_USE_MOCK !== 'false';

  if (!useMock) {
    // 실제 API 호출
    const formData = new FormData();
    formData.append('image', file);
    if (childInfo) {
      formData.append('childInfo', JSON.stringify(childInfo));
    }
    const response = await client.post<AnalysisResult>('/analyze', formData, {
      headers: { 'Content-Type': 'multipart/form-data' },
    });
    return response.data;
  }

  // Mock 응답 (기본값)
  return new Promise((resolve) => {
    setTimeout(() => resolve(generateMockResult(childInfo?.name || '아이')), 3000);
  });
};
```

### 1.3 환경별 설정

**로컬 개발 환경** (`frontend/.env`):
```bash
VITE_API_URL=http://localhost:3000
VITE_USE_MOCK=false
```

**프로덕션 (Netlify)**: `.env` 파일 없음 → 자동으로 Mock 모드

### 1.4 검증 방법

**터미널 1 (Backend)**:
```bash
cd api-gateway
npm start
```

**터미널 2 (Frontend)**:
```bash
cd frontend
echo "VITE_USE_MOCK=false" > .env
npm run dev
```

**브라우저**: `http://localhost:5173` 접속 후 이미지 업로드 → `shared_volume/uploads` 확인

---

## 2. CI/CD 파이프라인 구축

### 2.1 GitHub Actions 구조

**`.github/workflows/main.yml`**

```yaml
name: Mind Palette CI/CD

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]

jobs:
  # 1. Backend CI (Node.js)
  backend-ci:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20.x'
      - working-directory: ./api-gateway
        run: |
          npm ci
          npm test

  # 2. Frontend CI (React + Vite)
  frontend-ci:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20.x'
      - working-directory: ./frontend
        run: |
          npm ci
          npm test
          npm run build  # 빌드 단계 추가 (중요!)

  # 3. Integration Test
  integration-test:
    needs: [backend-ci, frontend-ci]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
      - working-directory: ./api-gateway
        run: |
          npm ci
          npm start &
          sleep 5
          npm test tests/integration.test.js

  # 4. Security Analysis (CodeQL)
  security-analysis:
    runs-on: ubuntu-latest
    permissions:
      security-events: write
      actions: read
      contents: read
    steps:
      - uses: actions/checkout@v4
      - uses: github/codeql-action/init@v4
        with:
          languages: javascript
      - uses: github/codeql-action/analyze@v4
```

### 2.2 CI vs CD 비교

| 항목 | GitHub Actions (CI) | Netlify (CD) |
|:---|:---|:---|
| **주요 목적** | 코드 품질 검증 (Quality Gate) | 프로덕션 배포 (Deployment) |
| **실행 명령어** | `npm test` + `npm run build` | `npm run build` |
| **타입 체크** | ✅ (빌드 중 자동) | ✅ (빌드 중 자동) |
| **유닛 테스트** | ✅ 실행 | ❌ 실행 안 함 (선택) |
| **빌드 생성** | ✅ (검증용) | ✅ (배포용) |
| **실제 배포** | ❌ | ✅ (CDN에 배포) |
| **실패 시 영향** | PR Merge 차단 가능 | 배포 중단 (이전 버전 유지) |

**핵심**: GitHub Actions에서 **빌드까지 검증**하여 Netlify에서 빌드 실패하는 불상사를 사전에 방지합니다.

---

## 3. GitHub Actions 버전 관리

### 3.1 Deprecated 경고 해결 히스토리

**2024-12-06 업데이트**:
- `github/codeql-action/init@v2` → `v3` → `v4`
- `github/codeql-action/analyze@v2` → `v3` → `v4`
- `actions/checkout@v3` → `v4`
- `actions/setup-node@v3` → `v4`

### 3.2 버전 업데이트 이유

| 버전 | 상태 | Deprecated 날짜 |
|:---:|:---|:---|
| `v1` | ❌ Deprecated | 2025-01-10 |
| `v2` | ❌ Deprecated | 2025-01-10 |
| `v3` | ⚠️ 예정 (Upcoming) | 2026-12 |
| `v4` | ✅ Current | - |

**권장사항**: 항상 `v4`를 사용하여 향후 2년간 안정성 확보

### 3.3 업데이트 명령어

```bash
# 워크플로우 파일에서 일괄 치환
sed -i 's/@v3/@v4/g' .github/workflows/main.yml
```

---

## 4. Node.js 의존성 보안 관리

### 4.1 주요 패키지 업데이트 (2024-12-06)

**API Gateway (`api-gateway/package.json`)**:

| 패키지 | 이전 버전 | 현재 버전 | 업데이트 이유 |
|:---|:---|:---|:---|
| `multer` | `1.4.5-lts.1` | `2.0.2` | v1.x 보안 취약점 해결 |
| `supertest` | `6.3.3` | `7.1.4` | v6.x Deprecated 해소 |
| `superagent` | - | `10.2.3` | supertest 의존성 최신화 |
| `rimraf` | - | `6.1.2` | glob 의존성 정리 |

### 4.2 업데이트 절차

```bash
cd api-gateway

# 1. 패키지 업데이트
npm install multer@latest supertest@latest superagent@latest rimraf@latest

# 2. 취약점 자동 수정
npm audit fix

# 3. 의존성 트리 정리
npm dedupe

# 4. 테스트 실행 (회귀 테스트)
npm test

# 결과: found 0 vulnerabilities ✅
```

### 4.3 남은 경고 (무시 가능)

**`inflight` & `glob@7.x` 경고**:
```
npm warn deprecated inflight@1.0.6: This module is not supported, and leaks memory.
npm warn deprecated glob@7.2.3: Glob versions prior to v9 are no longer supported
```

**원인**: `jest@29.7.0`이 내부적으로 `glob@7.x` 사용  
**해결**: Jest 팀이 v30에서 해결 예정 (우리가 강제 수정 시 테스트 프레임워크 손상 위험)  
**결론**: **기능상 문제 없음, 무시해도 안전**

---

## 5. 로컬 개발 환경 설정

### 5.1 프로젝트 구조

```
mind-palette-project/
├── frontend/              # React (Vite)
├── api-gateway/           # Node.js (Express)
├── shared_volume/         # 파일 저장소
│   ├── uploads/          # 업로드된 이미지
│   └── results/          # 분석 결과 JSON
└── .github/workflows/     # CI/CD 파이프라인
```

### 5.2 초기 설정

**1. 의존성 설치**:
```bash
# Frontend
cd frontend
npm install

# Backend
cd ../api-gateway
npm install
```

**2. 환경 변수 설정** (`frontend/.env`):
```bash
VITE_API_URL=http://localhost:3000
VITE_USE_MOCK=false
```

**3. 서버 실행** (2개 터미널 필요):
```bash
# Terminal 1 - Backend
cd api-gateway
npm start  # Port 3000

# Terminal 2 - Frontend
cd frontend
npm run dev  # Port 5173
```

### 5.3 로컬 테스트

**CLI 레벨 (curl 사용)**:
```bash
cd api-gateway

# 더미 이미지 생성
echo "fake image content" > tests/dummy.jpg

# API 요청
curl.exe -X POST -F "image=@tests/dummy.jpg" http://localhost:3000/analyze

# 파일 저장 확인
dir ..\shared_volume\uploads
dir ..\shared_volume\results
```

**UI 레벨 (브라우저)**:
1. `http://localhost:5173` 접속
2. 이미지 업로드
3. 개발자 도구(F12) → Network 탭에서 `/analyze` 요청 확인
4. `shared_volume/uploads` 폴더에 파일 생성 확인

---

## 6. 트러블슈팅

### 6.1 Frontend CI 실패: `Unknown option --watchAll`

**에러**:
```
CACError: Unknown option `--watchAll`
```

**원인**: Vitest는 `--watchAll` 옵션을 지원하지 않음 (Jest와 다름)

**해결**:
```yaml
# .github/workflows/main.yml
- name: Run Tests (Frontend)
  working-directory: ./frontend
  run: npm test  # --watchAll=false 제거
```

### 6.2 Frontend CI 실패: `Cannot read properties of undefined (reading 'get')`

**에러**:
```
TypeError: Cannot read properties of undefined (reading 'get')
at webidl-conversions/lib/index.js:325:94
```

**원인**: Node.js 18.x와 jsdom/webidl-conversions 호환성 문제

**해결**: Node.js 버전을 20.x로 업그레이드
```yaml
strategy:
  matrix:
    node-version: [20.x]  # 18.x → 20.x
```

### 6.3 Netlify 빌드 실패: TypeScript 에러

**에러 1**: `Cannot find module 'axios'`
```bash
cd frontend
npm install axios
```

**에러 2**: `Parameter 'config' implicitly has an 'any' type`
```typescript
// frontend/src/api/client.ts
import { InternalAxiosRequestConfig } from 'axios';

apiClient.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  // ...
});
```

**에러 3**: `'client' is declared but its value is never read`
```typescript
// frontend/src/api/uploadApi.ts
// import client from './client';  // 제거
```

### 6.4 PowerShell에서 curl 명령어 실패

**에러**:
```
Invoke-WebRequest : 매개 변수 이름 'X'과(와) 일치하는 매개 변수를 찾을 수 없습니다.
```

**원인**: PowerShell의 `curl`은 `Invoke-WebRequest`의 별칭

**해결**: `curl.exe`를 명시적으로 호출
```powershell
curl.exe -X POST -F "image=@tests/dummy.jpg" http://localhost:3000/analyze
```

### 6.5 GitHub Actions에서 CodeQL v3 Deprecated 경고

**경고**:
```
Warning: CodeQL Action v3 will be deprecated in December 2026.
```

**해결**: 즉시 v4로 업그레이드
```yaml
- uses: github/codeql-action/init@v4
- uses: github/codeql-action/analyze@v4
```

---

## 📚 참고 자료

- [GitHub Actions 공식 문서](https://docs.github.com/en/actions)
- [CodeQL Action v4 마이그레이션 가이드](https://github.blog/changelog/2025-10-28-upcoming-deprecation-of-codeql-action-v3/)
- [Netlify 빌드 설정](https://docs.netlify.com/configure-builds/overview/)
- [Vitest 설정](https://vitest.dev/config/)
- [Multer v2 릴리스 노트](https://github.com/expressjs/multer/releases)

---

## ✅ 체크리스트

프로젝트 설정 시 아래 항목을 확인하세요:

- [ ] GitHub Actions 워크플로우 파일에서 모든 액션이 `v4` 사용 중
- [ ] Frontend CI에 `npm run build` 단계 포함
- [ ] `api-gateway/package.json`의 모든 패키지가 최신 버전 (`npm audit` 결과 0 vulnerabilities)
- [ ] `frontend/.env`가 `.gitignore`에 포함되어 있음 (로컬 전용)
- [ ] `shared_volume/` 폴더가 생성되어 있음
- [ ] 로컬 연동 테스트 성공 (curl 또는 브라우저)
- [ ] Netlify에서 자동 배포가 정상 작동 중

---

**문서 버전**: 1.0  
**최종 업데이트**: 2024-12-06  
**작성자**: Mind Palette Development Team

