# 🏗️ ai-server 전체 구조 및 아키텍처 분석 리포트

본 문서는 `ai-server`의 내부 디렉터리 구조, 데이터 흐름, 신경망 아키텍처 및 시스템 레이어 간의 의존 관계를 직관적으로 설명합니다.

---

## 1. 디렉터리 레이아웃과 역할 분리

```
ai-server/
├── src/                        ← 프로덕션 코드
│   ├── main.py                 ← FastAPI 앱 팩토리 (조립자)
│   ├── config.py               ← 모든 하이퍼파라미터 중앙 관리
│   ├── core/                   ← AI 핵심 연산 (순수 Python/PyTorch)
│   │   ├── model.py            ← HFDClassifier 신경망 정의
│   │   └── preprocessing.py    ← 이미지 → 텐서 변환 파이프라인
│   ├── infra/                  ← 외부 의존성 처리 (파일I/O, 장애 방어)
│   │   └── model_loader.py     ← .pt 가중치 로드 + 상태 관리
│   └── routes/                 ← HTTP 엔드포인트
│       └── health.py           ← GET /health
└── tests/                      ← TDD 테스트
    ├── test_model_architecture.py  ← L1: 구조 검증
    ├── test_preprocessing.py       ← L1+L2: 전처리 검증
    ├── test_inference.py           ← L2: 추론 로직 검증
    ├── test_health.py              ← L1+L3: API 응답 + 장애 방어
    └── test_e2e_real_image.py      ← 실제 이미지 E2E 검증
```

---

## 2. 요청 처리 흐름 (Request Flow)

```mermaid
sequenceDiagram
    participant C as Client<br/>(Node.js Gateway)
    participant R as routes/health.py<br/>(HTTP Layer)
    participant MS as ModelState<br/>(App State)
    participant ML as model_loader.py<br/>(Infra Layer)
    participant P as preprocessing.py<br/>(Core)
    participant M as HFDClassifier<br/>(Core/model.py)

    Note over C,M: 서버 시작 시 (startup)
    ML->>ML: path.exists() 확인
    ML->>M: HFDClassifier 생성 + .pt 로드
    ML->>MS: ModelState(male=모델, female=모델)

    Note over C,M: 실제 추론 요청 시
    C->>R: POST /infer {image, gender}
    R->>MS: app.state.model_state 조회
    MS-->>R: male_model or female_model
    R->>P: create_transform_pipeline(config)
    P-->>R: Tensor (1, 3, 260, 260)
    R->>M: model(tensor)
    M-->>R: (logits_a, logits_b, logits_c, logits_d)
    R->>R: torch.sigmoid(logits) 적용
    R-->>C: JSON {head_a: [...], head_b: [...], ...}
```

---

## 3. HFDClassifier 내부 텐서 변환 흐름 (Architecture)

```mermaid
graph LR
    subgraph Input["입력 이미지"]
        I["PIL Image<br/>(512×512, RGB)"]
    end

    subgraph Preprocessing["preprocessing.py"]
        P1["Resize(260×260)"]
        P2["ToTensor<br/>uint8 → float32"]
        P3["Normalize<br/>mean=(0.485,0.456,0.406)<br/>std=(0.229,0.224,0.225)"]
    end

    subgraph Model["model.py: HFDClassifier"]
        B["EfficientNet-B2<br/>backbone.features<br/>(❄️ 동결: requires_grad=False)"]
        G["avgpool<br/>Global Average Pooling"]
        F["flatten(1)<br/>(B, 1408)"]

        subgraph Heads["4개 Multi-Head (학습 가능)"]
            HA["head_a<br/>Linear(1408→19)<br/>머리/얼굴"]
            HB["head_b<br/>Linear(1408→14)<br/>몸통/비례"]
            HC["head_c<br/>Linear(1408→16)<br/>사지/말단"]
            HD["head_d<br/>Linear(1408→11)<br/>의복/질적"]
        end
    end

    subgraph Output["출력 (Raw Logits)"]
        O["sigmoid 적용 후<br/>각 항목 [0,1] 확률값<br/>총 60개 분류 결과"]
    end

    I --> P1 --> P2 --> P3
    P3 -->|"(1, 3, 260, 260)"| B
    B -->|"(1, 1408, H, W)"| G
    G -->|"(1, 1408, 1, 1)"| F
    F --> HA & HB & HC & HD
    HA & HB & HC & HD --> O

    style B fill:#e8f5e9,stroke:#4caf50
    style Heads fill:#e3f2fd,stroke:#2196f3
```

---

## 4. 모델 로딩과 장애 방어 상태 흐름 (Infra Layer)

```mermaid
stateDiagram-v2
    [*] --> Loading: 서버 기동

    state Loading {
        [*] --> CheckMale: _try_load(male)
        CheckMale --> MaleLoaded: 파일 존재 ✅
        CheckMale --> MaleNull: 파일 없음 ⚠️
        MaleLoaded --> CheckFemale
        MaleNull --> CheckFemale

        CheckFemale --> FemaleLoaded: 파일 존재 ✅
        CheckFemale --> FemaleNull: 파일 없음 ⚠️
    }

    Loading --> Ready: ModelState 구성 완료
    
    state Ready {
        [*] --> HealthOK: GET /health → 200 OK
        HealthOK --> BothLoaded: male=true, female=true
        HealthOK --> PartialLoaded: male=true, female=false (또는 반대)
        HealthOK --> NoneLoaded: male=false, female=false
    }

    note right of NoneLoaded: 서버는 살아있음!\n추론만 불가능한 상태
```

---

## 5. 레이어 간 의존 관계 (Dependency Graph)

```mermaid
graph TD
    CFG["📋 config.py<br/>ModelConfig<br/>(단일 진실 공급원)"]

    PREP["🔄 preprocessing.py<br/>create_transform_pipeline()"]
    MDL["🤖 model.py<br/>HFDClassifier"]
    LOADER["🔌 model_loader.py<br/>load_models() → ModelState"]
    MAIN["🚀 main.py<br/>create_app() [Factory]"]
    HEALTH["🌐 routes/health.py<br/>GET /health"]

    CFG -->|"input_size=260<br/>normalize_mean/std"| PREP
    CFG -->|"head_a/b/c/d size<br/>backbone_feature_dim"| MDL
    CFG -->|"model_path (male/female)<br/>device"| LOADER
    CFG --> MAIN

    MDL -->|"HFDClassifier 인스턴스"| LOADER
    LOADER -->|"ModelState"| MAIN
    MAIN -->|"app.state.model_state"| HEALTH
    PREP -->|"transforms.Compose"| HEALTH

    style CFG fill:#fff3e0,stroke:#ff9800,stroke-width:3px
    style MDL fill:#e3f2fd,stroke:#2196f3
    style LOADER fill:#fce4ec,stroke:#e91e63
```

---

## 6. 핵심 설계 원칙 요약

| 관심사 | 담당 파일 | 핵심 원칙 |
|:---|:---|:---|
| **하이퍼파라미터** | `config.py` | 하드코딩 제로 — 모든 수치가 여기서 관리 |
| **신경망 구조** | `core/model.py` | backbone 동결 / head만 학습 / Logits 반환 |
| **이미지 변환** | `core/preprocessing.py` | Config 기반 Normalize — 통계값 교체 가능 |
| **가중치 로딩** | `infra/model_loader.py` | 파일 없어도 서버 기동 — 상태로 추적 |
| **HTTP API** | `routes/health.py` | 모델 상태를 JSON으로 외부에 노출 |
| **앱 조립** | `main.py` | Factory 패턴 — 테스트 격리 가능 |

---
**작성자**: Antigravity AI Assistant  
**작성일**: 2026-03-14
