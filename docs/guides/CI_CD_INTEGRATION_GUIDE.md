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

### 2.3 (선택) CI 통과 후에만 배포되게 하기 (Branch Protection)

> **목표**: “CI가 깨진 커밋이 `main`에 들어가고 → Netlify가 배포해버리는” 상황을 구조적으로 차단합니다.  
> **핵심 아이디어**: Netlify는 GitHub에 push/merge가 되면 배포합니다. 따라서 **`main`에 merge 자체를 CI 통과로 게이트**하면 됩니다.

#### 권장 설정 (GitHub Repository Settings)

1. GitHub에서 `Settings → Branches → Branch protection rules`로 이동
2. `main` 브랜치에 protection rule 추가
3. 아래 옵션을 체크:
   - **Require a pull request before merging**
   - **Require status checks to pass before merging**
     - 필수 체크로 GitHub Actions 워크플로우의 CI job들을 선택 (예: `backend-ci`, `frontend-ci`, `integration-test`, `security-analysis`)
   - (권장) **Require branches to be up to date before merging**
   - (권장) **Restrict who can push to matching branches** (직접 push 금지)

#### 효과
- PR에서 CI가 실패하면 **Merge가 막히고**
- `main`에 들어가지 않으니 **Netlify 자동 배포도 자연스럽게 막힙니다.**

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

### 6.6 Visual Studio(CMake) + vcpkg + OpenCV(Windows Pack) 빌드/구성 실패

> **배경**: `preprocess-server`(C++ 전처리 서버) 개발을 Visual Studio의 **Folder Open + CMake** 방식으로 시작할 때 발생한 이슈 정리

#### 증상 A: OpenCV를 “찾았다”면서도 `OpenCV_FOUND=FALSE`

**메시지 (요약)**:
- `C:/opencv/build/OpenCVConfig.cmake ... set OpenCV_FOUND to FALSE so package "OpenCV" is considered NOT FOUND`

**원인**:
- `opencv-4.xx-windows.exe`로 설치한 **OpenCV Windows Pack**은 `C:\opencv\build\OpenCVConfig.cmake`가 “래퍼(wrapper)” 역할을 합니다.
- 최신/프리뷰 MSVC 버전에서는 이 래퍼가 런타임 폴더(vc16/vc17 등)를 제대로 추론 못해 `OpenCV_FOUND=FALSE`로 떨어질 수 있습니다.

**해결(로컬 OpenCV를 쓸 때)**:
- `OpenCV_DIR`을 래퍼가 아닌 **실제 config 디렉토리**로 지정합니다.
  - 예: `C:\opencv\build\x64\vc16\lib`
- 실행 시에는 DLL 경로가 필요할 수 있으니 PATH에 아래를 추가합니다.
  - 예: `C:\opencv\build\x64\vc16\bin`

#### 증상 B: vcpkg가 `vcpkg install failed`로 멈춤 (opencv4 빌드 실패)

**메시지 (요약)**:
- `CMake Error at C:/vcpkg/scripts/buildsystems/vcpkg.cmake:... (message): vcpkg install failed`
- 로그에서 `opencv4:x64-windows BUILD_FAILED`

**원인**:
- Visual Studio가 `vcpkg.json`을 감지하면 **manifest install**을 자동 수행합니다.
- 이 때 `opencv4`는 의존성 그래프가 크고, 환경/포트 상태에 따라 **소스 빌드가 실패**할 수 있습니다.

**해결(이번 케이스 / TDD Minimal 적용)**:
- 현재 단계는 “헬스 체크/라우팅 테스트”였기 때문에 OpenCV가 필수 의존성이 아니었습니다.
- 따라서 빌드 안정성을 위해 **OpenCV를 임시로 의존성에서 제거**했습니다.
  - `preprocess-server/vcpkg.json`: `opencv` 제거
  - `preprocess-server/CMakeLists.txt`: `find_package(OpenCV ...)` 및 `${OpenCV_LIBS}` 링크 제거
- Crow 사용을 위해 `asio`를 명시적으로 링크합니다.
  - `find_package(asio CONFIG REQUIRED)`
  - `target_link_libraries(... asio::asio ...)`

> **재도입 원칙**: 이미지 전처리 기능을 TDD로 시작(“전처리 테스트” 추가)하는 시점에만 OpenCV를 다시 추가합니다.

#### 증상 C: `Failed to take the filesystem lock` (vcpkg 잠금)

**메시지 (요약)**:
- `C:\vcpkg\.vcpkg-root: error: Failed to take the filesystem lock`

**원인**:
- Visual Studio 백그라운드 vcpkg 프로세스가 실행 중인 상태에서, 별도의 터미널에서 vcpkg를 동시에 실행하면 발생합니다.

**해결**:
- `vcpkg.exe` 프로세스가 남아있는지 확인 후 종료(또는 VS에서 install이 끝날 때까지 대기)합니다.
  - 예: PowerShell에서 `Get-Process vcpkg`로 확인 후 필요 시 종료

#### 로그 위치(빠른 진단)
- Visual Studio 기준:
  - `preprocess-server/out/build/<preset>/vcpkg-manifest-install.log`
- vcpkg 포트 빌드 로그:
  - `C:\vcpkg\buildtrees\opencv4\*`

#### 권장 설정 체크리스트 (vcpkg 통일 기준)

- **Windows (Visual Studio / CMake)**
  - [ ] 환경변수에 `OPENCV_DIR`가 없다. (로컬 OpenCV로 “새는” 원인 제거)
  - [ ] `PATH`에 `C:\opencv\...` 또는 `%OPENCV_DIR%\...` 경로가 없다.
  - [ ] Visual Studio를 완전히 종료 후 재실행했다. (백그라운드 vcpkg 잠금 해제)
  - [ ] `preprocess-server/out/build/`를 삭제했거나, VS에서 **CMake 캐시 삭제 및 재구성**을 수행했다.
  - [ ] 의존성은 `vcpkg.json`만을 “진실의 근원(Source of Truth)”으로 관리한다. (로컬 경로 하드코딩 금지)
  - [ ] OpenCV는 “전처리 테스트”가 추가되는 시점에만 재도입한다. (Minimal / 실패 확률 감소)

- **CI (GitHub Actions)**
  - [ ] vcpkg 버전(포트)을 고정한다. (`vcpkg-configuration.json`의 `baseline` 또는 `builtin-baseline`)
  - [ ] 로컬과 동일한 triplet/툴체인 정책을 유지한다. (예: Windows=`x64-windows`, Linux=`x64-linux`)
  - [ ] (선택) 바이너리 캐시를 사용해 빌드 시간을 안정화한다.

- **Docker**
  - [ ] 컨테이너 내부에서 vcpkg로 의존성을 설치한다. (호스트 `C:\opencv`는 컨테이너에서 사용 불가)
  - [ ] Linux 컨테이너 기준 `x64-linux` triplet을 사용한다.

### 6.7 CI (또는 로컬) 빌드 실패: `spdlog/spdlog.h: No such file or directory`

**에러**:
```
fatal error: spdlog/spdlog.h: No such file or directory
#include <spdlog/spdlog.h>
```

**원인**:
- CMake는 철저하게 **타겟(Target) 중심**입니다. 최상단에서 `find_package(spdlog CONFIG REQUIRED)`를 호출했더라도, 해당 라이브러리를 실제로 사용하는 소스코드(예: `image_processor.cpp`)를 컴파일하는 **모든 개별 실행 파일/테스트 타겟**에 의존성을 명시적으로 주입(Link)하지 않으면 컴파일러가 헤더 경로를 알 수 없습니다.

**해결**:
- 문제가 발생한 소스코드를 공유하여 빌드하는 모든 타겟(테스트 예제 등)의 `target_link_libraries`에 `spdlog::spdlog`를 추가합니다.

```cmake
# CMakeLists.txt 예시
add_executable(test_canny_pipeline examples/test_canny_pipeline.cpp src/core/image_processor.cpp)
- target_link_libraries(test_canny_pipeline PRIVATE ${OpenCV_LIBS})
+ target_link_libraries(test_canny_pipeline PRIVATE ${OpenCV_LIBS} spdlog::spdlog)
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

**문서 버전**: 1.1  
**최종 업데이트**: 2026-01-14  
**작성자**: Mind Palette Development Team

