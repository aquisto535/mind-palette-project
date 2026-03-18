# AI Server 기술 스택 가이드 (AI Server Tech Stack Guide)

본 문서는 `ai-server`의 핵심 아키텍처를 구성하는 주요 프레임워크와 라이브러리들을 **제1원칙 사고(First Principles Thinking)**와 **데이터 흐름 기반 3단계 프레임워크**를 바탕으로 정리한 학습 가이드입니다.

---

## 1. 제1원칙 사고 기반 기술 분석 (First Principles)

AI 서버의 본질적 진실은 **"업로드된 원격 데이터를 안전하게 수신하고(Network), 이를 수치 데이터로 변환하여(Validation), 최적화된 지능형 연산을 거쳐(Inference), 신뢰할 수 있는 결과로 반환하는 것(Response)"**입니다.

### [Deconstruct & Optimize]
1.  **Network & Async (사고의 분해)**: 동시 요청 처리를 위한 Non-blocking I/O 최적화 -> **FastAPI + Uvicorn** 선정.
2.  **Inference Intelligence (본질적 지능)**: 모델 정의의 유연성과 추론 성능의 균형 -> **PyTorch + ONNX Runtime** 조합.
3.  **Data Integrity (가정 제거)**: "올바른 이미지가 올 것이다"라는 가정을 배제하고 엄격한 검증 수행 -> **Pydantic + Pillow** 기반 L1 검증.
4.  **Observability (투명성)**: 분산 환경에서 요청의 흐름을 추적할 수 있는 구조화된 데이터 생성 -> **structlog**를 이용한 Request ID 전파.

---

## 2. 데이터 흐름 기반 3단계 기술 스택

| Level | 구성 요소 | 주요 라이브러리 | 역할 |
| :--- | :--- | :--- | :--- |
| **L1: 데이터 구조 (What)** | 입력 검증 및 스키마 | `Pydantic`, `Pillow`, `NumPy` | 이미지 형식 확인, Shape(260x260x3) 검증, Tensor 변환 전 전처리 |
| **L2: 변환 로직 (How)** | AI 모델 및 추론 | `PyTorch`, `ONNX`, `ONNX Runtime` | MobileNetV3 기반 Backbone 연사, ONNX 최적화 추론 (Latency 최적화) |
| **L3: 제약과 검증 (Why)** | 안정성 및 성능 | `pytest`, `structlog`, `psutil` | OOM 대응(503), Request ID 추적, 리소스 모니터링, 테스트 자동화 |

---

## 3. 상세 기술 명세 (Detailed Tech Stack)

### 🌐 Web & API Architecture
- **FastAPI**: Python의 타입 힌트를 활용한 고성능 웹 프레임워크. 비동기(ASync) 처리를 기본으로 하며 자동 OpenAPI(Swagger) 문서를 제공합니다.
- **Uvicorn**: FastAPI를 실행하는 고성능 ASGI(Asynchronous Server Gateway Interface) 서버입니다.

### 🧠 AI / ML Inference
- **PyTorch**: 동적 그래프 방식의 딥러닝 프레임워크로, 모델 설계 및 `HFDClassifier` 아키텍처 정의에 사용됩니다.
- **Torchvision**: 컴퓨터 비전 특화 라이브러리로, MobileNetV3 등 사전 학습된 가중치와 이미지 변환 도구를 제공합니다.
- **ONNX (Open Neural Network Exchange)**: 모델의 상호 운용성을 위한 표준 포맷입니다. PyTorch 모델을 최적화된 추론용 파일로 변환합니다.
- **ONNX Runtime**: CPU 환경에서도 높은 추론 속도를 보장하는 고성능 추론 엔진입니다. (AI Inference Optimizer의 핵심 도구)

### 📊 Data Processing & Validation
- **NumPy**: 다차원 배열 연산을 위한 핵심 라이브러리입니다. PIL과 PyTorch/ONNX 간의 데이터 교량 역할을 합니다.
- **Pillow (PIL)**: 이미지 데이터의 로드, 리사이징, 픽셀 검증 및 코덱 처리를 담당합니다.
- **Pydantic**: 데이터 유효성 검사 라이브러리입니다. `/analyze` 엔드포인트의 입력 데이터 형식을 강제합니다.
- **Pydantic-settings**: 환경 변수(`.env`)와 설정값(`config.py`)을 계층적으로 관리합니다.

### 🛡️ Quality & Observability
- **structlog**: "누가(Request ID), 언제, 무엇을" 했는지 기계가 읽기 쉬운 JSON 포맷으로 기록합니다.
- **psutil**: 서버의 CPU, 메모리 실시간 점유율을 측정하여 `/health` 체크 시 제공합니다.
- **pytest & pytest-asyncio**: TDD 기반 개발을 지원하며, 비동기 API 엔드포인트와 추론 정합성을 자동 검증합니다.
- **HTTPX**: 비동기 HTTP 클라이언트로, 통합 테스트 및 E2E 테스트에서 서버를 호출할 때 사용됩니다.

---

## 4. 도구 활용 지침 (Usage Guidelines)

1.  **신규 기능 추가 시**: `Pydantic` 모델을 먼저 정의하여 L1 데이터 구조를 확정하세요.
2.  **추론 속도 개선 필요 시**: `ONNX Runtime`의 `ExecutionProvider` 설정을 조정하거나 `ONNX Script`를 통한 노드 최적화를 수행하세요.
3.  **오류 추적 시**: `structlog`의 `contextvars`를 활용하여 로그에 항상 `request_id`가 포함되도록 유지하세요.

---
> **Reference**: 본 스택은 AI Pipeline Architect 에이전트에 의해 설계되었으며, Mind Palette 프로젝트의 성능과 안정성을 최우선으로 고려하여 선정되었습니다.
