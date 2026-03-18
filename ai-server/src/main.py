from typing import Optional
import time

from fastapi import FastAPI

from src.config import ModelConfig
from src.infra.model_loader import ModelState, load_models
from src.infra.logger import setup_logging, get_logger
from src.routes.analyze import router as analyze_router
from src.routes.health import router as health_router
import structlog
from starlette.requests import Request
from starlette.responses import Response


def create_app(model_config: Optional[ModelConfig] = None) -> FastAPI:
    """FastAPI 앱 팩토리. 테스트 격리를 위해 팩토리 패턴 사용."""
    setup_logging()
    logger = get_logger("main")
    
    config = model_config or ModelConfig()
    app = FastAPI(title="Mind Palette AI Server")
    app.state.start_time = time.time()

    @app.middleware("http")
    async def add_request_id_to_logs(request: Request, call_next):
        request_id = request.headers.get("X-Request-ID", "unknown")
        structlog.contextvars.clear_contextvars()
        structlog.contextvars.bind_contextvars(request_id=request_id)
        
        response: Response = await call_next(request)
        response.headers["X-Request-ID"] = request_id
        return response

    model_state = load_models(config)
    app.state.model_state = model_state
    
    logger.info("Server initialized", device=config.device)

    app.include_router(health_router)
    app.include_router(analyze_router)
    return app


app = create_app()
