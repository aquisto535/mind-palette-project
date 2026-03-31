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
    async def add_request_id_to_logs(request: Request, call_next): # http 요청이 들어올 때마다 실행되는 함수
        request_id = request.headers.get("X-Request-ID", "unknown") # 요청 헤더에서 request_id를 가져옴
        structlog.contextvars.clear_contextvars() # 기존 로그의 contextvars를 초기화
        structlog.contextvars.bind_contextvars(request_id=request_id) # 새로운 request_id를 contextvars에 바인딩
        
        response: Response = await call_next(request) # 다음 미들웨어 또는 라우터 함수 호출
        response.headers["X-Request-ID"] = request_id # 응답 헤더에 request_id를 추가
        return response

    model_state = load_models(config) # 실제 모델 파일을 하드디스크에서 읽어오는 과정
    app.state.model_state = model_state # 모델 상태를 앱 상태에 저장
    
    logger.info("Server initialized", device=config.device)

    app.include_router(health_router) # health_router를 앱에 포함
    app.include_router(analyze_router) # analyze_router를 앱에 포함
    return app


app = create_app()
