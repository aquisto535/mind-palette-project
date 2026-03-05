from fastapi import APIRouter, Request

router = APIRouter()


@router.get("/health")
async def health_check(request: Request):
    model_state = getattr(request.app.state, "model_state", None)

    if model_state is None:
        return {
            "status": "ok",
            "models": {"male": False, "female": False},
        }

    return {
        "status": "ok",
        "models": {
            "male": model_state.male_loaded,
            "female": model_state.female_loaded,
        },
    }
