# 🧠 AI Server 구조 분석 리포트

> 작성일: 2026-03-25  
> 분석 대상: `ai-server/src/` (FastAPI + ONNX/TensorRT 추론 서버)

---

## 1. 디렉터리 구조

```
ai-server/
├── src/
│   ├── main.py              ← FastAPI 앱 팩토리 (진입점)
│   ├── config.py            ← 모든 하이퍼파라미터 중앙 관리
│   ├── routes/
│   │   ├── health.py        ← GET /health (서버 상태 확인)
│   │   └── analyze.py       ← POST /analyze (핵심: 이미지 → 결과)
│   ├── core/                ← AI 도메인 로직
│   │   ├── model.py         ← HFDClassifier (EfficientNet-B2 + 4 Heads)
│   │   ├── preprocessing.py ← 이미지 → 텐서 변환 파이프라인
│   │   ├── augmentation.py  ← 학습/추론용 이미지 변환
│   │   ├── iq_scorer.py     ← 원점수 → IQ + 백분위 계산
│   │   ├── item_mapping.py  ← 60문항 번호 매핑 테이블
│   │   └── onnx_converter.py← PyTorch → ONNX 변환 스크립트
│   └── infra/               ← 인프라/시스템 계층
│       ├── engine_protocol.py   ← InferenceEngine Protocol (공통 인터페이스)
│       ├── model_loader.py      ← TensorRT → ONNX Fallback 로더
│       ├── onnx_inference.py    ← ONNX Runtime 엔진
│       ├── tensorrt_engine.py   ← TensorRT Native 엔진 (GPU 최적화)
│       ├── tensorrt_ort_engine.py ← TensorRT + ORT 혼합 엔진
│       └── logger.py            ← structlog 기반 JSON 로거
└── tests/
    ├── test_model_architecture.py ← L1: Shape 검증
    ├── test_preprocessing.py      ← L1/L2: 전처리 검증
    ├── test_inference.py          ← L2: 동결/추론 검증
    ├── test_health.py             ← L3: Graceful Degradation
    └── test_e2e_real_image.py     ← E2E: 실제 이미지 통합
```

---

## 2. HTTP 요청 처리 흐름 (POST /analyze)

```mermaid
sequenceDiagram
    participant C as Client<br/>(Node.js API Gateway)
    participant R as routes/analyze.py
    participant V as _validate_image_file()
    participant P as core/augmentation.py<br/>get_val_transform()
    participant E as infra/InferenceEngine<br/>(ONNX or TensorRT)
    participant S as core/iq_scorer.py

    C->>R: POST /analyze<br/>multipart: file + age + child_gender + figure_gender

    R->>V: 파일 유효성 검사
    Note over V: ① 0바이트 체크<br/>② Content-Type 허용 목록<br/>③ 매직 바이트 (JPEG/PNG/BMP/WebP)<br/>④ PIL.verify()
    V-->>R: 통과 또는 400 Bad Request

    R->>R: figure_gender로 엔진 선택<br/>male_engine or female_engine
    Note over R: 엔진 None이면 503 반환

    R->>P: PIL Image
    P-->>R: Tensor (1, 3, 260, 260)<br/>float32 numpy

    R->>E: engine.run(img_np)
    Note over E: ONNX Session.run()<br/>or TensorRT execute()
    E-->>R: (head_a, head_b, head_c, head_d)<br/>각각 numpy 배열

    R->>R: _logits_to_item_results()<br/>Sigmoid → 60문항 0/1 결과
    R->>R: _compute_head_scores()<br/>4개 그룹 원점수 합산

    R->>S: raw_score + age + gender
    Note over S: IQ = 100 + 15×((점수-M)/SD)<br/>전국 규준 데이터 적용
    S-->>R: iq, percentile

    R-->>C: JSON 응답<br/>items(60문항) + head_scores + raw_score + iq + percentile
```

---

## 3. 서버 부팅 시 엔진 로딩 전략 (Fallback Chain)

```mermaid
flowchart TD
    Start([서버 시작<br/>main.py: create_app]) --> LC[load_models 호출]

    LC --> GPU{CUDA 사용 가능?}

    GPU -->|Yes| TRT{.engine 파일<br/>남녀 모두 존재?}
    TRT -->|Yes| LoadTRT[TensorRtNativeEngine 로드<br/>engine_type = 'tensorrt']
    TRT -->|No| ONNX_FB[ONNX로 Fallback]
    GPU -->|No| ONNX_FB

    ONNX_FB --> ONN{.onnx 파일<br/>남녀 모두 존재?}
    ONN -->|Yes| LoadONNX[OnnxInferenceEngine 로드<br/>engine_type = 'onnx']
    ONN -->|No| NoModel[ModelState 빈 상태<br/>engine_type = 'none']

    LoadTRT --> Ready([서버 정상 기동<br/>모든 요청 처리 가능])
    LoadONNX --> Ready
    NoModel --> Degraded([서버 기동은 됨<br/>/analyze 요청 시 503])

    style LoadTRT fill:#e8f5e9,stroke:#4caf50
    style LoadONNX fill:#fff3e0,stroke:#ff9800
    style NoModel fill:#fce4ec,stroke:#e91e63
    style Ready fill:#e3f2fd,stroke:#2196f3
    style Degraded fill:#fafafa,stroke:#9e9e9e
```

> **핵심**: 모델 파일이 없어도 서버는 **절대 죽지 않습니다**. `/health`는 항상 200 OK를 반환하고, `/analyze` 호출 시에만 503을 반환합니다. (Graceful Degradation)

---

## 4. 추론 엔진 계층 구조 (Strategy Pattern)

```mermaid
classDiagram
    class InferenceEngine {
        <<Protocol>>
        +run(image: ndarray) tuple
        +output_names list[str]
    }

    class OnnxInferenceEngine {
        -_session: InferenceSession
        -_input_name: str
        -_output_names: list
        +run(image) tuple
        +providers list
    }

    class TensorRtNativeEngine {
        +run(image) tuple
        +output_names list
    }

    class TensorRtOrtEngine {
        +run(image) tuple
        +output_names list
    }

    InferenceEngine <|.. OnnxInferenceEngine : implements
    InferenceEngine <|.. TensorRtNativeEngine : implements
    InferenceEngine <|.. TensorRtOrtEngine : implements

    class ModelState {
        +male_engine: InferenceEngine
        +female_engine: InferenceEngine
        +engine_type: str
        +male_loaded bool
        +female_loaded bool
    }

    ModelState o-- InferenceEngine
```

> **핵심**: `analyze.py`는 `engine.run(img_np)` 한 줄만 호출합니다. 엔진이 ONNX든 TensorRT든 **동일 인터페이스**이므로 라우터 코드는 전혀 바뀌지 않습니다. (Protocol 기반 duck typing)

---

## 5. 데이터 변환 흐름 요약

```
[클라이언트 이미지 bytes]
       │  (multipart/form-data)
       ▼
[_validate_image_file()]
  ✓ 0바이트 거부
  ✓ Content-Type 허용 목록
  ✓ 매직 바이트 (JPEG/PNG/BMP/WebP)
  ✓ PIL.verify() 이중 검증
       │
       ▼
[PIL Image.open().convert("RGB")]
       │
       ▼
[get_val_transform(260)]
  Resize(260, 260)
  ToTensor  → [0.0, 1.0] float32
  Normalize → 채널별 ImageNet 통계 적용
       │ Tensor shape: (1, 3, 260, 260)
       ▼
[engine.run(img_np)]
  EfficientNet-B2 Backbone (frozen)
  → GlobalAvgPool → flatten → [1408]
  → head_a: Linear(1408→19)  ← 머리/얼굴
  → head_b: Linear(1408→14)  ← 몸통/비례
  → head_c: Linear(1408→16)  ← 사지/말단
  → head_d: Linear(1408→11)  ← 의복/질적
       │ Raw Logits: (1,19), (1,14), (1,16), (1,11)
       ▼
[_logits_to_item_results()]
  Sigmoid 적용 → 확률
  ≥ 0.5 → 1 (통과), < 0.5 → 0 (미통과)
       │ 60문항 dict {"1": 1, "2": 0, ...}
       ▼
[score_to_result()]
  raw_score = sum(items)  ← 0~60점
  IQ = 100 + 15 × ((점수 - M) / SD)  ← 전국 규준
  백분위 = IQ_TO_PERCENTILE[iq]
       │
       ▼
[최종 JSON 응답]
  items: {60문항 결과}
  head_scores: {a,b,c,d 원점수}
  raw_score, iq, percentile
  child_info: {age, child_gender, figure_gender}
```

---

## 6. 현재 구현 상태 요약표

| 컴포넌트 | 파일 | 상태 | 역할 |
|:---|:---|:---:|:---|
| FastAPI 앱 진입점 | `src/main.py` | ✅ | 앱 팩토리, 라우터 등록 |
| 설정 중앙 관리 | `src/config.py` | ✅ | 하이퍼파라미터 하드코딩 방지 |
| 이미지 분석 API | `src/routes/analyze.py` | ✅ | 입력 검증 + 추론 + IQ 산출 |
| 헬스 체크 API | `src/routes/health.py` | ✅ | CPU/메모리/모델 상태 반환 |
| HFD 모델 | `src/core/model.py` | ✅ | EfficientNet-B2 + 4 Linear Heads |
| 전처리 파이프라인 | `src/core/preprocessing.py` | ✅ | Resize → ToTensor → Normalize |
| IQ 점수 계산 | `src/core/iq_scorer.py` | ✅ | 전국 규준 기반 IQ + 백분위 |
| 엔진 공통 인터페이스 | `src/infra/engine_protocol.py` | ✅ | Protocol (duck typing) |
| ONNX 추론 엔진 | `src/infra/onnx_inference.py` | ✅ | CPU 기본 추론 |
| TensorRT 엔진 | `src/infra/tensorrt_engine.py` | ✅ | GPU 최적화 추론 |
| 모델 로더 | `src/infra/model_loader.py` | ✅ | TRT → ONNX Fallback 전략 |
| 구조화 로거 | `src/infra/logger.py` | ✅ | structlog JSON 포맷 |
| ONNX 변환기 | `src/core/onnx_converter.py` | ✅ | PyTorch → ONNX 스크립트 |
