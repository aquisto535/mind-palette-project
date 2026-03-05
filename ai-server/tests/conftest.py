import pytest

from src.config import ModelConfig


@pytest.fixture(scope="session")
def config():
    """테스트용 ModelConfig (기본값 사용)."""
    return ModelConfig()
