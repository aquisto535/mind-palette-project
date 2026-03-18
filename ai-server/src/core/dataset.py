"""HFDDataset: HFD 아동화 데이터셋 클래스.

annotations.json을 로드해 이미지-레이블 쌍을 반환한다.
"""

import json
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import torch
from PIL import Image
from torch.utils.data import Dataset

from src.core.augmentation import get_train_transform, get_val_transform


class HFDDataset(Dataset):
    """HFD(Human Figure Drawing) 아동화 데이터셋.

    Args:
        annotations_path: annotations.json 파일 경로
        data_root: 이미지 파일 기준 루트 디렉토리 (기본: annotations 파일과 같은 디렉토리의 상위)
        transform: 이미지 변환 (None이면 검증용 변환 적용)
        figure_gender: "male" | "female" | None (None이면 전체 사용)
    """

    def __init__(
        self,
        annotations_path: str,
        data_root: Optional[str] = None,
        transform=None,
        figure_gender: Optional[str] = None,
    ):
        ann_path = Path(annotations_path)
        with open(ann_path, encoding="utf-8") as f:
            data = json.load(f)

        samples = data["samples"]
        if figure_gender is not None:
            samples = [s for s in samples if s["figure_gender"] == figure_gender]

        self._samples = samples
        self._data_root = Path(data_root) if data_root else ann_path.parent.parent
        self._transform = transform if transform is not None else get_val_transform()

    def __len__(self) -> int:
        return len(self._samples)

    def __getitem__(self, idx: int) -> Tuple[torch.Tensor, Dict[str, torch.Tensor]]:
        sample = self._samples[idx]
        img_path = self._data_root / sample["image_path"]
        image = Image.open(img_path).convert("RGB")
        image_tensor = self._transform(image)

        head_labels = sample["head_labels"]
        labels = {
            head: torch.tensor(values, dtype=torch.float32)
            for head, values in head_labels.items()
        }
        return image_tensor, labels

    def get_metadata(self, idx: int) -> Dict:
        """인덱스에 해당하는 샘플 메타데이터 반환."""
        s = self._samples[idx]
        return {
            "id": s["id"],
            "child_age": s["child_age"],
            "child_gender": s["child_gender"],
            "figure_gender": s["figure_gender"],
            "raw_score": s["raw_score"],
        }
