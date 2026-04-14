# Phase 4 Step 1: Python AI Server 기본 모델 구현 (TDD)

## Context

Phase 4 Step 1은 Mind Palette 프로젝트의 핵심인 Python AI 서버를 TDD 방식으로 구축하는 작업입니다.
현재 `ai-server/` 디렉토리가 존재하지 않으며, 처음부터 생성합니다.

**핵심 명세 (ADR-018 + implementation_plan.md)**:
- 분류 방식: Multi-label Binary (Sigmoid + BCEWithLogitsLoss)
- Backbone: EfficientNet-B2 (pretrained, frozen, 1408-dim)
- 입력: (B, 3, 260, 260)
- 4 Heads: A(19) + B(14) + C(16) + D(11) = 60개 이진 판정
- forward() → Tuple 반환 (ONNX 호환), raw logits (sigmoid 미적용)
- **남녀 전략**: 동일 HFDClassifier 클래스 + 별도 가중치 파일 (`male.pt` / `female.pt`)
  - 모델 구조(19/14/16/11)는 동일, 일부 항목 의미만 다름
  - `gender` 파라미터로 로드할 가중치 선택
  - ModelLoader가 양쪽 모델을 모두 관리

---

## 디렉토리 구조

```
ai-server/
├── pyproject.toml
├── src/
│   ├── __init__.py
│   ├── main.py                 # FastAPI app factory + lifespan
│   ├── config.py               # ModelConfig, ServerConfig (pydantic-settings)
│   ├── core/
│   │   ├── __init__.py
│   │   ├── model.py            # HFDClassifier(nn.Module)
│   │   └── preprocessing.py    # Resize→Normalize→ToTensor 파이프라인
│   ├── routes/
│   │   ├── __init__.py
│   │   └── health.py           # GET /health
│   └── infra/
│       ├── __init__.py
│       └── model_loader.py     # 모델 로드/상태 관리
└── tests/
    ├── __init__.py
    ├── conftest.py             # 공유 fixtures
    ├── test_health.py          # /health 엔드포인트
    ├── test_model_architecture.py  # backbone, heads, shapes
    ├── test_preprocessing.py   # 전처리 파이프라인
    └── test_inference.py       # 동결 검증, sigmoid 범위
```

---

## TDD 실행 순서 (8 Cycles)

### Phase A: 구조 설정 (Tidy First)

**Cycle 0** — `chore: ai-server 프로젝트 구조 및 의존성 설정`
- 디렉토리 구조 생성, `pyproject.toml`, `__init__.py` 파일들
- `src/config.py`: ModelConfig (head sizes, input_size=260, backbone_feature_dim=1408, normalize_mean/std, **male_model_path, female_model_path**)
- `tests/conftest.py`: config, model fixtures
- `.gitignore`에 Python 패턴 추가 (`__pycache__/`, `*.pyc`, `.venv/`, `ai-server/models/`)

### Phase B: L1 테스트 (데이터 구조)

**Cycle 1** — FastAPI /health 엔드포인트
- **Red**: `test_health.py` — 200 OK, `{"status": str, "models": {"male": bool, "female": bool}}` 필드/타입 검증
- **Green**: `src/main.py` + `src/routes/health.py` 구현

**Cycle 2** — 모델 아키텍처 구조
- **Red**: `test_model_architecture.py` — backbone 존재, head 4개(nn.Linear), out_features=19/14/16/11, in_features=1408, 총합=60
- **Green**: `src/core/model.py` — HFDClassifier 클래스 구현

**Cycle 3** — 출력 텐서 Shape
- **Red**: `test_model_architecture.py` 추가 — 더미 입력 (1,3,260,260) → (1,19)/(1,14)/(1,16)/(1,11), dtype=float32, Tuple 반환, batch>1 지원
- **Green**: Cycle 2에서 이미 구현됨 (검증 테스트)

**Cycle 4** — 3채널 입력 이미지 구조
- **Red**: `test_preprocessing.py` — shape==(H,W,3), dtype==uint8, 채널 분리 가능
- **Green**: C++ 전처리 결과물 검증 테스트이므로 trivially pass

### Phase C: L2 테스트 (변환 로직)

**Cycle 5** — 전처리 파이프라인
- **Red**: `test_preprocessing.py` 추가 — output shape==(3,260,260), dtype==float32, 값 범위, config의 mean/std 사용 검증
- **Green**: `src/core/preprocessing.py` — `create_transform_pipeline(config)` 구현

**Cycle 6** — Feature Extractor 동결 검증
- **Red**: `test_inference.py` — backbone.parameters() requires_grad==False, head.parameters() requires_grad==True
- **Green**: Cycle 2에서 이미 구현됨 (검증 테스트)

**Cycle 7** — Multi-head sigmoid 출력 범위
- **Red**: `test_inference.py` 추가 — 고정 seed → sigmoid(logits) ∈ [0,1], 결정론적 출력, logits가 확률이 아님(음수 또는 >1 존재) 검증
- **Green**: Cycle 2에서 이미 구현됨 (검증 테스트)

### Phase D: 통합

**Cycle 8** — /health + 모델 상태 연동 (남녀 모델)
- **Red**: `test_health.py` 추가 — 모델 미로드 시 models.male==false/models.female==false, 서버 정상 기동, gender별 독립 로드 확인
- **Green**: `src/infra/model_loader.py` (남녀 모델 독립 관리) + `main.py`/`health.py` 업데이트

---

## 핵심 설계 결정

| 결정 | 선택 | 근거 |
|------|------|------|
| `forward()` 반환 | `Tuple[Tensor x4]` | ONNX 호환 (ADR-018) |
| Sigmoid 위치 | forward() 밖 (추론 시만) | BCEWithLogitsLoss 수치 안정성 |
| Head 구조 | `nn.Linear(1408, N)` | MVP 단순성, 추후 dropout/hidden 추가 가능 |
| 남녀 전략 | 동일 클래스 + 별도 가중치 | 구조 동일(19/14/16/11), 항목 의미만 다름 |
| Config | pydantic-settings | 타입 안전, 환경변수 지원, 하드코딩 방지 |
| App 패턴 | Factory `create_app()` | 테스트 격리 |
| Backbone | `torchvision.models.efficientnet_b2` | 표준 라이브러리, 의존성 최소화 |

---

## 주요 파일 목록

| 파일 | 역할 |
|------|------|
| `ai-server/pyproject.toml` | 의존성 (fastapi, torch, torchvision, pydantic-settings, pytest, httpx) |
| `ai-server/src/config.py` | Head 크기, 입력 차원, 정규화 파라미터 중앙 관리 |
| `ai-server/src/core/model.py` | HFDClassifier: EfficientNet-B2 backbone + 4 Linear heads |
| `ai-server/src/core/preprocessing.py` | Resize(260)→ToTensor→Normalize 파이프라인 |
| `ai-server/src/main.py` | FastAPI app factory, lifespan, 라우트 등록 |
| `ai-server/src/routes/health.py` | GET /health (status + models: {male, female}) |
| `ai-server/src/infra/model_loader.py` | 남녀 모델 독립 로드, 에러 핸들링, 상태 관리 |

---

## 커밋 계획 (Feature Branch: `feature/ai-server-base-model`)

| # | 타입 | 메시지 |
|---|------|--------|
| 1 | `chore` | `chore: ai-server 프로젝트 구조 및 의존성 설정` |
| 2 | `test` | `test(ai-server): /health 응답 구조 테스트 (L1 Red)` |
| 3 | `feat` | `feat(ai-server): FastAPI 기본 골격 및 /health 엔드포인트 구현` |
| 4 | `test` | `test(ai-server): 모델 아키텍처 구조 및 출력 Shape 테스트 (L1 Red)` |
| 5 | `feat` | `feat(ai-server): EfficientNet-B2 기반 HFDClassifier 구현` |
| 6 | `test` | `test(ai-server): 전처리 파이프라인 검증 (L1+L2 Red)` |
| 7 | `feat` | `feat(ai-server): 이미지 전처리 파이프라인 구현` |
| 8 | `test` | `test(ai-server): backbone 동결 및 sigmoid 출력 범위 검증 (L2 Red)` |
| 9 | `test` | `test(ai-server): /health 모델 상태 연동 검증 (Red)` |
| 10 | `feat` | `feat(ai-server): 모델 로더 및 /health 상태 연동 구현` |

---

## 검증 방법

```bash
# 1. 의존성 설치
cd ai-server && pip install -e ".[dev]"

# 2. 전체 테스트 실행
pytest tests/ -v

# 3. 개별 테스트 파일 실행 (TDD 사이클 중)
pytest tests/test_health.py -v
pytest tests/test_model_architecture.py -v
pytest tests/test_preprocessing.py -v
pytest tests/test_inference.py -v

# 4. 서버 로컬 실행 확인
uvicorn src.main:app --reload --port 8082
curl http://localhost:8082/health
```

## 주의 사항

- CI에서 PyTorch는 CPU-only 설치 (`--index-url https://download.pytorch.org/whl/cpu`)
- EfficientNet-B2 pretrained weights 첫 다운로드 ~35MB 소요
- 모델 fixture는 `session` 스코프로 설정하여 테스트 속도 최적화
- Windows 경로는 `pathlib.Path` 사용
