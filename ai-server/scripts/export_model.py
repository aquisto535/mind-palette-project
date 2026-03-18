"""학습된 .pt 모델을 .onnx로 내보내기.

사용법:
    python scripts/export_model.py --gender male
    python scripts/export_model.py --gender female
"""

import argparse

import torch

from src.config import ModelConfig
from src.core.model import HFDClassifier
from src.core.onnx_converter import export_to_onnx
from src.infra.logger import get_logger

logger = get_logger(__name__)


def main():
    parser = argparse.ArgumentParser(description="HFD .pt → .onnx 변환")
    parser.add_argument("--gender", choices=["male", "female"], default="male")
    parser.add_argument("--device", default="cpu")
    args = parser.parse_args()

    config = ModelConfig(device=args.device)

    pt_path = config.male_model_path if args.gender == "male" else config.female_model_path
    onnx_path = config.male_onnx_path if args.gender == "male" else config.female_onnx_path

    model = HFDClassifier(config)
    model.load_state_dict(torch.load(pt_path, map_location=args.device))
    model.eval()

    out = export_to_onnx(model, config, onnx_path)
    logger.info(f"ONNX 저장 완료: {out}")
    print(f"Exported: {out}")


if __name__ == "__main__":
    main()
