from typing import Optional

from fastapi import FastAPI

from src.config import ModelConfig
from src.infra.model_loader import ModelState, load_models
from src.routes.health import router as health_router


def create_app(model_config: Optional[ModelConfig] = None) -> FastAPI:
    """FastAPI 앱 팩토리. 테스트 격리를 위해 팩토리 패턴 사용."""
    config = model_config or ModelConfig()
    app = FastAPI(title="Mind Palette AI Server")

    model_state = load_models(config)
    app.state.model_state = model_state

    app.include_router(health_router)
    return app


app = create_app()
