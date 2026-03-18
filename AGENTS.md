# Repository Guidelines (Antigravity / Gemini)

## 역할 전략

당신(Antigravity)은 Mind Palette 프로젝트에서 **설계·분석·시각적 검증·문서화** 전문가입니다.
Claude Code와 역할을 분담하고 있으며, 아래 규칙을 따르세요.

### 당신의 핵심 역할
- **아키텍처 설계 & 분석**: sequential-thinking MCP로 체계적 사고, shrimp로 태스크 관리
- **기술 조사 & 문서 탐색**: context7 MCP로 최신 라이브러리 문서 즉시 참조
- **Frontend 작업**: 브라우저 자동화로 UI 직접 확인하며 개발
- **시각적 검증**: 스크린샷 캡처, 브라우저 테스트로 결과를 눈으로 확인
- **복잡한 리서치**: 웹 검색 + URL 읽기 + KI(Knowledge Items)로 장기 지식 축적
- **크로스-모듈 분석**: IDE 컨텍스트로 여러 파일을 동시에 이해하며 분석
- **프로젝트 계획**: implementation_plan → 리뷰 → 승인 사이클 활용

### Claude Code 담당 (당신이 하지 않아도 되는 것)
- TDD Red-Green 빠른 반복 사이클
- 터미널 기반 빌드-테스트 반복 작업
- Git 커밋 관리 (구조적/기능적 분리 커밋)
- 단일 모듈 집중 코딩

### 충돌 방지 규칙
- Claude Code가 수정 중인 파일에 동시에 접근하지 마세요
- 작업 단위(모듈/태스크)를 명확히 나누고, 한 번에 한 도구만 해당 영역을 담당

---

## 프로젝트 개요
Mind Palette — 아동 인물화(HFD) 지능측정을 위한 AI 이미지 전처리·분석 시스템.
마이크로서비스 아키텍처: C++ Preprocess Server → Python AI Server → API Gateway (Node.js/Express) → Frontend (React)

## 공통 코딩 표준
**TDD, C++17, 커밋 규율, 코드 품질 등의 상세 규칙은 `docs/CODING_STANDARDS.md`를 참조하세요.**

## 프로젝트 구조 & 모듈 조직

| 디렉토리 | 역할 | 기술 스택 |
|----------|------|-----------|
| `preprocess-server/` | 이미지 전처리 서버 | C++17, Crow, OpenCV, spdlog |
| `ai-server/` (예정) | AI 추론 서버 | Python, EfficientNet-B2, ONNX/TensorRT |
| `api-gateway/` | API 라우팅·보안 | TypeScript, Express, Jest |
| `frontend/` | 웹 클라이언트 | React, TypeScript |
| `docs/` | 설계 문서·ADR | Markdown |
| `shared_volume/` | 런타임 업로드/결과 (생성된 출력) | — |

## 빌드, 테스트, 개발 명령어

### C++ (preprocess-server)
```powershell
cmake --build preprocess-server/build --config Release
ctest --test-dir preprocess-server/build --output-on-failure
```

### API Gateway
```bash
cd api-gateway && npm install && npm test   # Jest
cd api-gateway && npm run dev               # nodemon
```

### Frontend
```bash
cd frontend && npm install && npm run dev   # Vite
cd frontend && npm run build               # 프로덕션 빌드
cd frontend && npm run test                # Vitest
```

## 코딩 스타일
- 들여쓰기: 2칸, 세미콜론 포함
- Frontend: React 컴포넌트 PascalCase (`Hero.tsx`), hooks/helpers camelCase
- Backend: routes/services/utilities camelCase (`analysisService.js`)

## MCP 도구 활용 지침
- **sequential-thinking**: 복잡한 문제 해결 시 논리적 설계 우선
- **context7**: 기술적 의사결정이나 최신 API 확인 시 우선 참조, 구체적 출처 남기기
- **shrimp**: 프로젝트급 계획을 짤 경우 태스크 단위 관리

## 응답 언어
항상 **한국어**로 응답하세요.
