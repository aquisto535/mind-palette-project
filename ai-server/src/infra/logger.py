import sys
import os
import structlog
import logging.handlers
from typing import Any, Dict
from pathlib import Path

def setup_logging():
    """structlog를 사용한 구조화된 로깅 설정."""
    log_dir = Path("logs")
    log_dir.mkdir(exist_ok=True)
    log_file = log_dir / "ai-server.json.log"

    # 1. 파일 핸들러 설정 (JSON 포맷)
    file_handler = logging.handlers.RotatingFileHandler(
        log_file, maxBytes=10 * 1024 * 1024, backupCount=5, encoding="utf-8"
    )
    
    # 2. 콘솔 핸들러 설정
    console_handler = logging.StreamHandler(sys.stdout)

    # 3. 루트 로거 설정 (Multi-Handler)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[file_handler, console_handler]
    )

    structlog.configure(
        processors=[
            structlog.contextvars.merge_contextvars,
            structlog.processors.add_log_level,
            structlog.processors.StackInfoRenderer(),
            structlog.processors.format_exc_info,
            structlog.processors.TimeStamper(fmt="iso"),
            structlog.processors.JSONRenderer(), # JSON 포맷 로그 출력
        ],
        logger_factory=structlog.stdlib.LoggerFactory(),
        wrapper_class=structlog.stdlib.BoundLogger,
        cache_logger_on_first_use=True,
    )

def get_logger(name: str) -> Any:
    """이름 기반 로거 생성."""
    return structlog.get_logger(name)
