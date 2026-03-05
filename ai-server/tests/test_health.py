"""GET /health 엔드포인트 응답 구조 테스트 (L1 + 통합)."""

import pytest
from httpx import ASGITransport, AsyncClient

from src.main import create_app
from src.config import ModelConfig


@pytest.fixture
def app():
    return create_app()


@pytest.fixture
def app_no_models():
    """모델 파일이 존재하지 않는 상태의 앱."""
    config = ModelConfig(
        male_model_path="nonexistent/male.pt",
        female_model_path="nonexistent/female.pt",
    )
    return create_app(model_config=config)


@pytest.mark.asyncio
async def test_health_returns_200(app):
    """GET /health → 200 OK."""
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        response = await client.get("/health")
    assert response.status_code == 200


@pytest.mark.asyncio
async def test_health_has_status_field(app):
    """응답에 'status' 필드(str)가 존재해야 한다."""
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        response = await client.get("/health")
    data = response.json()
    assert "status" in data
    assert isinstance(data["status"], str)


@pytest.mark.asyncio
async def test_health_has_models_field(app):
    """응답에 'models' 필드가 존재하고 male/female bool을 포함해야 한다."""
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        response = await client.get("/health")
    data = response.json()
    assert "models" in data
    models = data["models"]
    assert "male" in models
    assert "female" in models
    assert isinstance(models["male"], bool)
    assert isinstance(models["female"], bool)


# --- Cycle 8: 모델 상태 연동 ---


@pytest.mark.asyncio
async def test_health_model_loaded_false_when_no_weights(app_no_models):
    """모델 가중치 파일이 없으면 models.male/female == False여야 한다."""
    transport = ASGITransport(app=app_no_models)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        response = await client.get("/health")
    data = response.json()
    assert data["models"]["male"] is False
    assert data["models"]["female"] is False


@pytest.mark.asyncio
async def test_health_server_runs_without_model(app_no_models):
    """모델이 없어도 서버는 200 OK로 정상 응답해야 한다."""
    transport = ASGITransport(app=app_no_models)
    async with AsyncClient(transport=transport, base_url="http://test") as client:
        response = await client.get("/health")
    assert response.status_code == 200
    assert response.json()["status"] == "ok"
