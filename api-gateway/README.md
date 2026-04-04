# Mind Palette: API Gateway

Mind Palette 프로젝트의 중앙 진입점(Entry Point) 역할을 수행하는 Node.js 기반의 마이크로서비스입니다. 불특정 다수의 트래픽 방어, 보안 검열, 그리고 비동기 워크플로우 오케스트레이션을 담당합니다.

## 🚀 아키텍처 역할 (Why Node.js?)

API Gateway를 Python이나 C++ 대신 **Node.js(Express)**로 분리하여 설계한 이유는 명확합니다.

1. **극단적인 I/O 비동기 처리 성능:** 이미지 업로드 수신, C++ 서버로의 전달, Python AI 서버로의 전달 등 게이트웨이의 작업은 100% **I/O Bound** 작업입니다. Node.js 특유의 태생적 비동기 이벤트 루프(Event Loop) 아키텍처는 이 대규모 트래픽을 처리하는 데 최적화되어 있습니다.
2. **풀스택 TypeScript 생태계:** Frontend(React)와 동일하게 TypeScript를 채택함으로써, 데이터 타입과 인터페이스를 손쉽게 공유하고 컨텍스트 스위칭 비용을 최소화했습니다.
3. **비용 효율적 트래픽 댐 (Traffic Dam):** 무거운 CPU 연산 코어(C++, Python)가 외부 트래픽에 직격으로 노출되지 않도록 막아주는 방어벽 역할을 합니다.

## ✨ 핵심 기능 (Core Features)

- **6-Layer 파일 업로드 보안 & 방어:**
  - 단순 확장자 비교를 넘어, 메모리 상의 **Magic Byte 검증(L2)** 및 샌드박스 경로 제어(Path Traversal 방어)를 통해 악성 파일 투입을 원천 차단합니다 (ADR-020).
- **성능 오케스트레이션 및 무결성 제어:**
  - **Auto-Cleanup (Stateless):** 클라우드 디스크 용량 고갈 방지를 위해, 분석이 끝나면 원본 및 `.tmp` 버퍼 파일을 원자적(`try-finally`)으로 파쇄합니다.
  - **SHA-256 해시 무결성:** 분석된 결과 JSON에 원본 이미지의 해시를 첨부하여 위변조를 막고, 동일 요청 시 **10ms 이내 응답을 보장하는 스마트 캐싱 레이어**의 기반으로 활용합니다.
- **분산 추적 (Distributed Tracing) 및 무중단 프로파일링:**
  - **X-Request-ID 전파:** 게이트웨이가 발급한 UUID가 C++(전처리)와 Python(AI) 로그로 일관되게 전파되어, MSA 환경의 디버깅 가시성을 확보합니다.
  - **W3C Server-Timing API (ADR-026):** 병목 구간 튜닝을 위해 데이터베이스 기록(I/O Overhead) 없이, 클라이언트 HTTP 응답 헤더에 각 마이크로서비스의 소요 시간(ms)을 동적으로 삽입합니다.

## 🛠️ 기술 스택

- **Runtime:** Node.js, TypeScript
- **Web Framework:** Express.js
- **Upload Handling:** Multer
- **HTTP Client:** Axios (마이크로서비스 간 통신)
- **Test Framework:** Jest, Supertest
- **Logging:** Winston + Morgan

## 📂 폴더 구조

```text
api-gateway/
├── src/
│   ├── routes/        # API 엔드포인트 정의 (e.g., /analyze)
│   ├── services/      # 비즈니스 로직 및 외부 서버 통신 로직 (analysisService)
│   ├── utils/         # 파일 저장, 무결성 검증, 로거 등 유틸리티
│   ├── middlewares/   # 에러 핸들링, Request ID 발급 등
│   └── server.ts      # Express 서버 진입점
├── tests/             # 단위 테스트 및 E2E 테스트 (TDD Red-Green 원칙)
└── shared_volume/     # (Docker Runtime) C++ 및 Python 서버와 파일 교환을 위한 공유 디스크 (휘발성)
```

## 💻 실행 방법

```bash
# 의존성 설치
npm install

# 개발 환경 실행 (nodemon 모니터링)
npm run dev

# 프로덕션 빌드
npm run build
npm start

# 테스트 실행 (Jest)
npm run test
```

## 🛡️ 의존성 서비스
이 서비스가 정상 동작하려면 아래의 서버 라우팅 설정이 필요합니다. (또는 `docker-compose.yml` 사용)
- `PREPROCESS_SERVER_URL` (기본값: `http://localhost:8081`) - C++ 전처리 서버
- `AI_SERVER_URL` (기본값: `http://localhost:8082`) - Python AI 모델 추론 서버
