from fastapi import APIRouter, Request
import psutil
import time
import os

router = APIRouter()

@router.get("/health")
async def health_check(request: Request):
    model_state = getattr(request.app.state, "model_state", None)
    start_time = getattr(request.app.state, "start_time", time.time())
    
    # 시스템 정보 수집
    process = psutil.Process(os.getpid())
    cpu_usage = process.cpu_percent(interval=None)
    memory_info = process.memory_info()
    
    uptime = time.time() - start_time
    
    health_data = {
        "status": "ok",
        "uptime_seconds": int(uptime),
        "system": {
            "cpu_usage_percent": cpu_usage,
            "memory_usage_bytes": memory_info.rss,
            "memory_usage_mb": round(memory_info.rss / (1024 * 1024), 2)
        },
        "models": {
            "male": False,
            "female": False
        }
    }

    if model_state is not None:
        health_data["models"] = {
            "male": model_state.male_loaded,
            "female": model_state.female_loaded,
        }

    return health_data
