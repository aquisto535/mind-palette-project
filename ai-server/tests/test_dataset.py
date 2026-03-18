"""TDD RED: HFDDataset 테스트."""

import json
import tempfile
from pathlib import Path

import numpy as np
import pytest
import torch
from PIL import Image

from src.core.dataset import HFDDataset


def _make_sample(
    sample_id: str,
    child_gender: str = "male",
    figure_gender: str = "male",
    age: int = 10,
    raw_score: int = 29,
) -> dict:
    """테스트용 단일 샘플 딕셔너리 생성."""
    items = {str(i): (1 if i % 3 == 0 else 0) for i in range(1, 61)}
    head_labels = {
        "head_a": [items[str(i)] for i in [1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21]],
        "head_b": [items[str(i)] for i in [2, 3, 29, 30, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]],
        "head_c": [items[str(i)] for i in [22, 23, 24, 25, 26, 27, 28, 31, 32, 33, 34, 35, 36, 37, 38, 39]],
        "head_d": [items[str(i)] for i in [50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60]],
    }
    return {
        "id": sample_id,
        "image_path": f"raw/{sample_id}.png",
        "child_gender": child_gender,
        "figure_gender": figure_gender,
        "child_age": age,
        "raw_score": raw_score,
        "items": items,
        "head_labels": head_labels,
    }


@pytest.fixture
def dataset_dir(tmp_path):
    """임시 데이터셋 디렉토리 픽스처."""
    raw_dir = tmp_path / "raw"
    raw_dir.mkdir()
    labels_dir = tmp_path / "labels"
    labels_dir.mkdir()

    # 더미 이미지 3장 생성
    samples = [
        _make_sample("s001", child_gender="male", figure_gender="male"),
        _make_sample("s002", child_gender="male", figure_gender="female"),
        _make_sample("s003", child_gender="female", figure_gender="male"),
    ]
    for s in samples:
        img = Image.fromarray(np.full((64, 64, 3), 255, dtype=np.uint8))
        img.save(raw_dir / f"{s['id']}.png")

    annotations = {"samples": samples}
    ann_path = labels_dir / "annotations.json"
    ann_path.write_text(json.dumps(annotations), encoding="utf-8")

    return tmp_path, ann_path


class TestHFDDataset:
    def test_len(self, dataset_dir):
        """전체 데이터셋 길이 = 3."""
        _, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path))
        assert len(ds) == 3

    def test_getitem_image_shape(self, dataset_dir):
        """이미지 텐서 shape = (3, 260, 260)."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        img, _ = ds[0]
        assert img.shape == (3, 260, 260)

    def test_getitem_label_keys(self, dataset_dir):
        """레이블 딕셔너리에 4개 head 키가 있다."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        _, labels = ds[0]
        assert set(labels.keys()) == {"head_a", "head_b", "head_c", "head_d"}

    def test_getitem_label_types(self, dataset_dir):
        """레이블 값은 float32 텐서."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        _, labels = ds[0]
        for v in labels.values():
            assert isinstance(v, torch.Tensor)
            assert v.dtype == torch.float32

    def test_getitem_label_shapes(self, dataset_dir):
        """각 head 레이블 텐서 크기."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        _, labels = ds[0]
        assert labels["head_a"].shape == (19,)
        assert labels["head_b"].shape == (14,)
        assert labels["head_c"].shape == (16,)
        assert labels["head_d"].shape == (11,)

    def test_label_values_binary(self, dataset_dir):
        """레이블은 0.0 또는 1.0만 포함."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        _, labels = ds[0]
        for v in labels.values():
            assert torch.all((v == 0.0) | (v == 1.0))

    def test_figure_gender_filter_male(self, dataset_dir):
        """figure_gender='male' 필터링 → 2개 (s001, s003)."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(
            annotations_path=str(ann_path),
            data_root=str(tmp_path),
            figure_gender="male",
        )
        assert len(ds) == 2

    def test_figure_gender_filter_female(self, dataset_dir):
        """figure_gender='female' 필터링 → 1개 (s002)."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(
            annotations_path=str(ann_path),
            data_root=str(tmp_path),
            figure_gender="female",
        )
        assert len(ds) == 1

    def test_get_metadata(self, dataset_dir):
        """get_metadata로 child_age, child_gender 조회 가능."""
        tmp_path, ann_path = dataset_dir
        ds = HFDDataset(annotations_path=str(ann_path), data_root=str(tmp_path))
        meta = ds.get_metadata(0)
        assert "child_age" in meta
        assert "child_gender" in meta
        assert "figure_gender" in meta
        assert "raw_score" in meta
