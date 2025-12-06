# Netlify 배포 트러블슈팅 및 가이드

이 문서는 프로젝트 배포 과정에서 발생한 이슈와 해결 방법, 그리고 향후 안정적인 배포를 위한 가이드를 담고 있습니다.

## 1. 경로 설정 이슈 (Path Configuration)

### 증상
- 배포 시 `Publish directory`를 찾을 수 없다는 오류 발생.
- `Base directory`를 `frontend`로 설정했음에도 빌드 결과물을 찾지 못함.

### 원인
- Netlify의 `Base directory` 설정은 작업의 "시작 위치(Working Directory)"를 변경합니다.
- `Base directory`를 `frontend`로 설정하면, Netlify는 이미 `frontend` 폴더 내부로 진입한 상태가 됩니다.
- 이 상태에서 `Publish directory`를 `frontend/dist`로 설정하면, 실제로는 `frontend/frontend/dist`라는 존재하지 않는 경로를 찾게 됩니다.

### 해결 방법
- **Base directory**: `frontend`
- **Publish directory**: `dist` (현재 위치 기준 상대 경로)

---

## 2. 빌드 명령어 및 의존성 이슈 (`tsc: not found`)

### 증상
- 빌드 로그에 `sh: 1: tsc: not found` 또는 `Command failed with exit code 127` 발생.
- `npm run build` 실행 중 실패.

### 원인
- Netlify 빌드 환경에서 `node_modules`가 설치되지 않은 상태로 빌드 명령(`tsc && vite build`)이 실행됨.
- `tsc`(TypeScript Compiler)는 `devDependencies`에 포함되어 있으므로, `npm install` 과정이 선행되지 않으면 명령어를 찾을 수 없음.

### 해결 방법: `netlify.toml` 설정 파일 사용 (권장)
UI 설정 대신 프로젝트 루트에 `netlify.toml` 파일을 생성하여 명시적으로 설치와 빌드 과정을 정의합니다.

```toml
[build]
  # 작업 디렉토리를 frontend로 지정
  base = "frontend"
  
  # 빌드 완료 후 배포할 폴더 (frontend/dist 가 아님)
  publish = "dist"
  
  # 의존성 설치 후 빌드 실행
  command = "npm install && npm run build"
```

---

## 3. 향후 배포 관련 체크리스트 & 팁

### 설정 관리
- **`netlify.toml` 우선**: Netlify 웹 대시보드(UI)의 설정보다 `netlify.toml` 파일의 설정이 우선순위가 높습니다. 설정이 꼬이는 것을 방지하기 위해 파일로 관리하는 것이 안전합니다.

### Git 및 빌드
- **`.gitignore`와 `dist`**: `dist` 폴더는 `.gitignore`에 포함되어 있어도 됩니다. Netlify는 소스 코드를 받아 서버에서 직접 빌드하여 `dist`를 생성하므로, 로컬의 `dist` 폴더를 깃에 올릴 필요가 없습니다.
- **재배포 트리거**: 설정(`netlify.toml` 등)을 변경한 경우, 자동으로 배포가 되지 않을 수 있습니다. 변경 사항을 Git에 `push` 하거나, Netlify 콘솔에서 **"Trigger deploy"** 버튼을 눌러야 적용됩니다.

### 환경 변수 (Environment Variables)
- `.env` 파일은 보안상 Git에 업로드하지 않습니다.
- API Key 등 환경 변수가 필요한 경우, Netlify 대시보드의 **Site configuration > Environment variables** 메뉴에 직접 키와 값을 등록해야 합니다.

