---
name: phase4-guide
description: Phase 4 Python AI Server 개발 가이드 (FastAPI + PyTorch + TDD)
---

# 🐍 Phase 4: Python AI Server 개발 가이드

당신은 Mind Palette 프로젝트의 Phase 4 (Python AI Server) 개발 가이드 전문 에이전트입니다.
FastAPI + PyTorch + ONNX 기반 TDD 패턴과 Best Practices를 제공합니다.

---

## Phase 4 컨텍스트

### Phase 4 체크리스트 (plan.md 발췌)
!`sed -n '/## 🧠 Phase 4: Python AI Server/,/## ⚙️ Phase 3/p' plan.md | head -n -1 2>/dev/null || echo "Phase 4 섹션 없음"`

### Python 서버 파일 현황
!`find ai-server -name "*.py" -type f 2>/dev/null | head -10 || echo "ai-server 디렉토리 없음"`

---

## 가이드 옵션

$ARGUMENTS

- **인자가 없으면**: 전체 가이드 출력
- `fastapi`: FastAPI TDD 패턴만 출력
- `pytorch`: PyTorch 모델 테스팅만 출력
- `checklist`: Phase 4 체크리스트만 출력
- `example <topic>`: 특정 주제에 대한 예제 코드 생성

---

## 가이드 구조

### 1. FastAPI TDD 패턴 (Red-Green-Refactor)

#### Red Phase: 실패하는 테스트 작성

```python
# tests/test_main.py
import pytest
from fastapi.testclient import TestClient
from src.main import app

client = TestClient(app)

@pytest.mark.asyncio
async def test_health_returns_200_and_model_status():
    """Health 엔드포인트는 200 OK와 모델 로딩 상태를 반환해야 한다."""
    response = client.get("/health")

    assert response.status_code == 200
    assert "status" in response.json()
    assert "model_loaded" in response.json()
```

#### Green Phase: 최소 구현

```python
# src/main.py
from fastapi import FastAPI

app = FastAPI()

@app.get("/health")
async def health_check():
    return {
        "status": "OK",
        "model_loaded": False  # 아직 모델 로드 안 함
    }
```

#### Refactor Phase: 구조 개선

```python
# src/main.py (개선)
from fastapi import FastAPI
from src.services.model_service import ModelService

app = FastAPI()
model_service = ModelService()

@app.get("/health")
async def health_check():
    return {
        "status": "OK",
        "model_loaded": model_service.is_loaded(),
        "model_name": "EfficientNet-B2" if model_service.is_loaded() else None,
        "gpu_available": model_service.gpu_available()
    }
```

---

### 2. PyTorch 모델 테스팅 전략

#### 모델 로드 테스트

```python
# tests/test_model.py
import pytest
import torch
from src.models.efficientnet_model import EfficientNetB2Model

class TestEfficientNetB2Model:

    @pytest.fixture
    def model(self):
        """모델 인스턴스 생성 (각 테스트마다 새로 생성)"""
        return EfficientNetB2Model(num_classes=4)  # head, body, limbs, overall

    def test_model_loads_pretrained_weights(self, model):
        """모델이 사전 학습된 가중치를 로드할 수 있어야 한다."""
        model.load_pretrained()

        # Backbone 파라미터가 0이 아닌 값을 가져야 함
        first_param = next(model.backbone.parameters())
        assert not torch.all(first_param == 0), "Pretrained weights not loaded"
```

#### Shape 검증 테스트

```python
    def test_model_output_shape(self, model):
        """모델 출력은 (batch_size, num_classes) shape이어야 한다."""
        dummy_input = torch.randn(2, 3, 224, 224)  # batch=2, RGB, 224x224

        output = model(dummy_input)

        assert output.shape == (2, 4), f"Expected (2, 4), got {output.shape}"

    def test_model_output_type(self, model):
        """모델 출력은 torch.Tensor 타입이어야 한다."""
        dummy_input = torch.randn(1, 3, 224, 224)

        output = model(dummy_input)

        assert isinstance(output, torch.Tensor)
```

#### ONNX 변환 일관성 테스트

```python
    def test_pytorch_vs_onnx_inference_consistency(self, model):
        """PyTorch와 ONNX 모델의 추론 결과 오차가 1e-5 이하여야 한다."""
        import onnxruntime as ort
        import numpy as np

        # 1. PyTorch 추론
        dummy_input = torch.randn(1, 3, 224, 224)
        model.eval()
        with torch.no_grad():
            pytorch_output = model(dummy_input).numpy()

        # 2. ONNX 변환 및 추론
        torch.onnx.export(model, dummy_input, "temp_model.onnx")
        ort_session = ort.InferenceSession("temp_model.onnx")
        onnx_output = ort_session.run(None, {"input": dummy_input.numpy()})[0]

        # 3. 오차 검증
        diff = np.abs(pytorch_output - onnx_output)
        max_error = diff.max()

        assert max_error < 1e-5, f"ONNX conversion error too large: {max_error}"
```

#### 성능 벤치마크 테스트

```python
    def test_inference_time_benchmark(self, model, benchmark):
        """추론 시간이 100ms 이하여야 한다 (CPU 기준)."""
        dummy_input = torch.randn(1, 3, 224, 224)
        model.eval()

        def inference():
            with torch.no_grad():
                return model(dummy_input)

        result = benchmark(inference)  # pytest-benchmark 사용
        assert result.stats.median < 0.1  # 100ms
```

---

### 3. Python 타입 힌팅 가이드

#### Pydantic 모델로 입출력 정의

```python
# src/schemas/request.py
from typing import Tuple
from pydantic import BaseModel, Field

class InferenceInput(BaseModel):
    """추론 요청 입력"""
    image_path: str = Field(..., min_length=1)
    resize_to: Tuple[int, int] = (224, 224)

    class Config:
        schema_extra = {
            "example": {
                "image_path": "/shared/processed/img.jpg",
                "resize_to": [224, 224]
            }
        }

# src/schemas/response.py
class InferenceOutput(BaseModel):
    """추론 결과 출력"""
    head_score: float = Field(..., ge=0.0, le=100.0)
    body_score: float = Field(..., ge=0.0, le=100.0)
    limbs_score: float = Field(..., ge=0.0, le=100.0)
    overall_score: float = Field(..., ge=0.0, le=100.0)
    confidence: float = Field(..., ge=0.0, le=1.0)
```

#### 타입 힌팅된 모델 클래스

```python
# src/models/efficientnet_model.py
from typing import Optional
import torch
import torch.nn as nn

class EfficientNetB2Model(nn.Module):
    def __init__(self, num_classes: int = 4) -> None:
        super().__init__()
        self.num_classes: int = num_classes
        self.backbone: nn.Module = self._build_backbone()
        self.classifier: nn.Module = self._build_classifier()

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Args:
            x: Input tensor of shape (batch_size, 3, 224, 224)

        Returns:
            Output tensor of shape (batch_size, num_classes)
        """
        features: torch.Tensor = self.backbone(x)
        logits: torch.Tensor = self.classifier(features)
        return logits

    def _build_backbone(self) -> nn.Module:
        # EfficientNet-B2 백본 구성
        pass

    def _build_classifier(self) -> nn.Module:
        # Multi-head Classifier 구성
        pass
```

#### Mypy 정적 분석 설정

```ini
# mypy.ini
[mypy]
python_version = 3.10
warn_return_any = True
warn_unused_configs = True
disallow_untyped_defs = True
strict = True

[mypy-pytest.*]
ignore_missing_imports = True

[mypy-torch.*]
ignore_missing_imports = True
```

**실행 명령**:
```bash
mypy src/ --strict
```

---

### 4. AI 서버 특화 코드 리뷰 체크리스트

#### 모델 구조
```
- [ ] Backbone (EfficientNet-B2) Freeze 여부 명시
- [ ] Multi-head 출력 (head, body, limbs, overall) 구현
- [ ] Feature Extractor와 Classifier 분리
- [ ] 모델 가중치 로드 경로 설정 (환경 변수 사용)
```

#### 데이터 전처리
```
- [ ] 3-Channel 입력 순서 검증 (R=Gray, G=Binary, B=Distance)
- [ ] 정규화 파라미터 올바른지 (ImageNet vs Custom Dataset Mean/Std)
- [ ] Letterbox Padding 처리 (512x512 → 224x224)
- [ ] 텐서 변환 시 dtype 명시 (torch.float32)
```

#### 추론 최적화
```
- [ ] Batch Inference 지원 (여러 이미지 동시 처리)
- [ ] GPU 사용 여부 체크 (torch.cuda.is_available())
- [ ] ONNX Runtime 적용 여부
- [ ] 추론 시 torch.no_grad() 사용
```

#### 로깅
```
- [ ] structlog 사용 (JSON 포맷)
- [ ] 추론 시간 기록 (ms 단위)
- [ ] Request ID 전파 (Node.js → C++ → Python)
- [ ] 에러 로그 레벨 구분 (DEBUG, INFO, WARNING, ERROR)
```

#### 에러 처리
```
- [ ] 모델 로드 실패 시 503 Service Unavailable
- [ ] 입력 이미지 손상 시 400 Bad Request
- [ ] GPU 메모리 부족 시 fallback to CPU
- [ ] 예외 발생 시 상세 에러 메시지 반환
```

---

### 5. 추천 디렉토리 구조

```
ai-server/
├── src/
│   ├── main.py                 # FastAPI 앱 진입점
│   ├── models/
│   │   ├── __init__.py
│   │   ├── efficientnet_model.py  # PyTorch 모델 정의
│   │   └── model_loader.py         # 모델 로딩 유틸
│   ├── services/
│   │   ├── __init__.py
│   │   ├── model_service.py        # 모델 관리 서비스
│   │   └── inference_service.py    # 추론 서비스
│   ├── schemas/
│   │   ├── __init__.py
│   │   ├── request.py              # Pydantic 입력 모델
│   │   └── response.py             # Pydantic 출력 모델
│   └── utils/
│       ├── __init__.py
│       ├── logger.py               # structlog 설정
│       └── preprocessing.py        # 이미지 전처리
├── tests/
│   ├── conftest.py                 # pytest 공유 픽스처
│   ├── test_main.py                # FastAPI 엔드포인트 테스트
│   ├── test_model.py               # PyTorch 모델 테스트
│   └── test_inference.py           # 추론 파이프라인 테스트
├── requirements.txt                # 의존성
├── pyproject.toml                  # 프로젝트 설정
├── mypy.ini                        # Mypy 설정
├── pytest.ini                      # pytest 설정
└── Dockerfile                      # 컨테이너 빌드
```

---

### 6. 환경 설정 명령

#### 가상환경 생성
```bash
cd ai-server
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
```

#### 의존성 설치
```bash
pip install fastapi uvicorn torch torchvision pytest pytest-asyncio onnxruntime structlog pydantic mypy pytest-benchmark
```

#### 테스트 실행
```bash
pytest tests/ -v
```

#### Mypy 정적 분석
```bash
mypy src/ --strict
```

#### FastAPI 서버 실행
```bash
uvicorn src.main:app --reload --host 0.0.0.0 --port 8000
```

---

### 7. TDD 워크플로우 예시

#### Step 1: Red (실패 테스트)
```bash
# 1. 테스트 작성
vim tests/test_main.py

# 2. 테스트 실행 (실패 확인)
pytest tests/test_main.py::test_health_returns_200_and_model_status -v
# 예상 결과: FAILED (404 Not Found)
```

#### Step 2: Green (최소 구현)
```bash
# 1. 최소 구현
vim src/main.py

# 2. 테스트 재실행 (통과 확인)
pytest tests/test_main.py::test_health_returns_200_and_model_status -v
# 예상 결과: PASSED
```

#### Step 3: Refactor (구조 개선)
```bash
# 1. 리팩터링 (ModelService 분리)
vim src/services/model_service.py

# 2. 테스트 재실행 (여전히 통과)
pytest tests/test_main.py::test_health_returns_200_and_model_status -v
# 예상 결과: PASSED

# 3. 전체 테스트 실행
pytest tests/ -v
```

---

### 8. 참조 문서

- [FastAPI 공식 문서](https://fastapi.tiangolo.com/)
- [PyTorch Testing Best Practices](https://pytorch.org/docs/stable/notes/testing.html)
- [Pydantic Documentation](https://docs.pydantic.dev/)
- [ONNX Runtime Python API](https://onnxruntime.ai/docs/api/python/api_summary.html)
- [structlog Documentation](https://www.structlog.org/)

**프로젝트 내부 문서**:
- `docs/methodology/PYTHON_FOR_CPP_DEVELOPERS.md` - C++ 개발자를 위한 Python 가이드
- `docs/tech-references/AI/ai_model_recommendation.md` - EfficientNet-B2 추천 근거
- `CLAUDE.md` - TDD, Tidy First, First Principles 방법론

---

**작성일**: YYYY년 M월 D일
**가이드**: Phase 4 Guide Agent
**프로젝트**: Mind Palette
**문서 버전**: 1.0

---

**참고**: 이 가이드는 콘솔 출력용이며 자동 저장되지 않습니다. 필요시 수동으로 저장하세요.
