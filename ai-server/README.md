# 🧠 Mind Palette AI Server

HFD(인물화 검사) 스케치 이미지를 분석하여 아동의 지능(IQ)과 백분위를 산출하는 Python 기반 딥러닝 추론 서버입니다.

## 🚀 개요 (Overview)
- **Framework**: FastAPI (비동기 HTTP 서버)
- **Model**: EfficientNet-B2 기반 Multi-Head 분류기 (60문항 다중분류)
- **Inference Engine**: TensorRT **FP16 양자화** (GPU 가속) 지원 및 ONNX Runtime Fallback
- **핵심 목표**: 
  - 빠르고 안정적인 단일/배치 이미지 추론 제공 (TensorRT 기준 **15ms 이내 응답**, **300+ QPS 달성**)
  - **무중단 폴백(Graceful Degradation)**: 모델 파일 의존성을 낮추어, 엔진/가중치 로드 실패 시에도 서버 생존 보장
  - **데이터 강건성(Robustness)**: Channel Dropout Augmentation 및 도메인 특화 정규화 (Hybrid Input Normalization)
  - 남아/여아 분리된 HFD 채점 규준표 자동 적용 및 변환

---

## 📂 디렉터리 구조 및 역할 (Structure)
`ai-server`의 로직은 중앙 집중식 팩토리(`main.py`)를 통해 조립되는 구조를 가집니다.

```
ai-server/
├── src/
│   ├── main.py              ← FastAPI 앱 팩토리 (서버 객체 및 미들웨어 조립)
│   ├── config.py            ← 서버 구동 및 추론에 필요한 환경/하이퍼파라미터 정의
│   ├── routes/
│   │   ├── health.py        ← 서버 헬스체크 및 시스템/모델 로드 상태 반환 API
│   │   └── analyze.py       ← 이미지 분석, 추론 실행 및 IQ 산출 메인 API
│   ├── core/                ← HFD AI 도메인 종속 로직
│   │   ├── model.py         ← PyTorch HFDClassifier 아키텍처 정의
│   │   ├── preprocessing.py ← 입력 이미지 전처리 파이프라인 (Resize, ToTensor, Normalize)
│   │   ├── iq_scorer.py     ← 전국 규준을 바탕으로 원점수 → IQ/백분위 변환 공식
│   │   └── item_mapping.py  ← 60개 HFD 문항 번호 매핑 (Head A,B,C,D)
│   └── infra/               ← 인프라스트럭처 및 엔진 인터페이스 시스템
│       ├── engine_protocol.py ← Engine 구동을 위한 Protocol(덕 타이핑) 스펙
│       ├── model_loader.py    ← TensorRT 우선 적용 및 ONNX Fallback 로딩 전략 구현
│       ├── onnx_inference.py  ← ONNX Runtime 기반 실행기
│       └── tensorrt_*.py      ← TensorRT 기반 실행기 (성능 최적화)
└── tests/                   ← L1(구조), L2(로직), L3(예외 방어) TDD 검증용 테스트 코드
```

---

## 📖 추천 소스코드 분석 순서 (Study Guide)

이 모듈의 아키텍처를 온전히 이해하려면 **"데이터와 의존성의 흐름"**대로 코드를 읽어가는 것이 좋습니다. 제1원칙(First Principles)과 L1/L2/L3 프레임워크 기반의 추천 읽기 순서입니다.

### Step 1. 뼈대 파악하기 (진입점과 설정)
가장 먼저 서버의 기초 체력과 인테리어가 어떻게 구성되는지 확인합니다.
1. **`src/config.py` (L1: 구조의 기준점)**
   - **Focus:** `input_size`, `normalize_mean` 등 모든 추론의 통계적 기준이 어떻게 하드코딩을 피해 Pydantic 환경 변수로 묶여 있는지 확인합니다.
2. **`src/main.py` (L1: 전체 조립도)**
   - **Focus:** 설정(`config`)을 주입받아 FastAPI 앱으로 찍어내는 팩토리 패턴(`create_app`)의 이유와, 모든 요청에 ID 꼬리표를 달아주는 미들웨어(`@app.middleware`)의 존재 의의를 확인합니다.
3. **`src/routes/health.py` (L3: 생존 확인)**
   - **Focus:** 서버와 엔진 메모리 상태를 점검하는, 가장 의존성 없고 가벼운 API 코드로 워밍업합니다.

### Step 2. AI 코어 파헤치기 (모델 구조와 입력)
실제 딥러닝 모델의 물리적 형태와 데이터 가공법을 뜯어봅니다.
4. **`src/core/model.py` (L1/L2: 모델의 물리적 형태)**
   - **Focus:** HFD(인물화) 도메인 최적화를 위해 EfficientNet-B2 백본을 어떻게 '얼리고(`requires_grad=False`)', 끝단에 4개의 머리(`heads`)를 재조립하는지 추적합니다. (Transfer Learning 원리)
5. **`src/core/preprocessing.py` (L1/L2: 데이터의 변환)**
   - **Focus:** 네트워크(HTTP)를 탄 날것의 이미지가 `config.py`에서 정의한 '스케치별 특수 통계치'를 거치며 어떻게 AI가 이해할 텐서로 변하는지(`transforms.Compose`) 추적합니다.

### Step 3. 메인 비즈니스 로직 (요청 처리와 채점)
본질적 목표인 이미지-결과 변환 API의 동작을 확인합니다.
6. **`src/routes/analyze.py` (L2/L3: 엔진 구동 및 예외 방어)**
   - **Focus:** 전처리된 이미지를 엔진(`engine.run`)에 밀어 넣는 본선 로직입니다. 쏟아져 나온 `Logits` 결과를 60문항 `Dict`로 변환(`_logits_to_item_results`)하는 과정과, 메모리 고갈 등 비정상 상황 방어 로직(L3)을 집중 분석합니다.
7. **`src/core/item_mapping.py` & `src/core/iq_scorer.py` (L2: 결과 해석)**
   - **Focus:** 모델이 뽑아낸 60문항 점수가 전국 규준 데이터(연령, 성별)에 맞추어 어떻게 한국 아동의 IQ와 백분위로 변환 계산되는지 확인합니다.

### Step 4. 인프라와 엔진 (실전 구동 체계)
프레임워크나 최적화 계기에 모델이 어떻게 끼워 맞춰지는지 알아봅니다.
8. **`src/infra/engine_protocol.py` (L1: 공통 규약)**
   - **Focus:** TensorRT와 ONNX가 통신할 때 지켜야 할 파이썬 규약(Protocol)의 형태를 봅니다.
9. **`src/infra/onnx_inference.py` (L2: 엔진의 단일 구현)**
   - **Focus:** PyTorch의 무거운 로직을 벗어던지고 `onnxruntime`만으로 가볍게 추론하는 최적화 패러다임을 확인합니다.
10. **`src/infra/model_loader.py` (L3: 실전 방어와 폴백)**
    - **Focus:** 모델 가중치 파일(`.pt`, `.onnx` 등)이 없거나 로드에 실패하면 어떻게 서버를 보호하고 ONNX로 Fallback(우회)하는지, 그 생존 로직을 분석합니다.

---

## 🏃 실행 및 테스트 (Run & Test)

### 1. 테스트 실행 (PyTest)
전처리, 모델 구조, 서버 헬스체크 및 추론 E2E 테스트를 TDD 기반으로 검증합니다.
```bash
# ai-server 디렉토리에서 실행
pytest tests/ -v
```

### 2. 서버 구동
개발 모드로 활성화하려면:
```bash
# uvicorn 사용 (기본 포트 8000)
uvicorn src.main:app --reload
```
서버 구동 후 `http://localhost:8000/docs` 로 접속하여 Swagger UI 기반으로 엔드포인트를 손쉽게 탐색 및 테스트할 수 있습니다.
