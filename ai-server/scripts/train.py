"""HFD 모델 2단계 학습 스크립트.

Phase A: 합성 막대인간 데이터로 사전학습 (50 epoch)
Phase B: 실제 아동화 데이터로 미세조정 (100 epoch, 조기종료)

사용법:
    python scripts/train.py --gender male
    python scripts/train.py --gender female --device cuda
"""

import argparse
import os
from pathlib import Path
from typing import Dict, List, Optional

import torch
import torch.nn as nn
from torch.optim import AdamW
from torch.optim.lr_scheduler import CosineAnnealingLR
from torch.utils.data import DataLoader, Subset

from src.config import ModelConfig, TrainingConfig
from src.core.augmentation import get_train_transform, get_val_transform
from src.core.dataset import HFDDataset
from src.core.model import HFDClassifier
from src.infra.logger import get_logger

logger = get_logger(__name__)

HEAD_NAMES = ["head_a", "head_b", "head_c", "head_d"]


# ── 손실 계산 ─────────────────────────────────────────────────────────────
def compute_loss(
    outputs: tuple,
    labels: Dict[str, torch.Tensor],
    criterion: nn.BCEWithLogitsLoss,
) -> torch.Tensor:
    """4개 Head 출력에 대한 합산 손실."""
    total = sum(
        criterion(outputs[i], labels[HEAD_NAMES[i]])
        for i in range(len(HEAD_NAMES))
    )
    return total


# ── 에폭 학습 루프 ────────────────────────────────────────────────────────
def train_epoch(
    model: HFDClassifier,
    loader: DataLoader,
    optimizer: torch.optim.Optimizer,
    criterion: nn.BCEWithLogitsLoss,
    device: str,
) -> float:
    model.train()
    total_loss = 0.0
    for imgs, labels in loader:
        imgs = imgs.to(device)
        labels = {k: v.to(device) for k, v in labels.items()}

        optimizer.zero_grad()
        outputs = model(imgs)
        loss = compute_loss(outputs, labels, criterion)
        loss.backward()
        optimizer.step()
        total_loss += loss.item()

    return total_loss / len(loader)


# ── Phase A: 합성 데이터 사전학습 ────────────────────────────────────────
def run_phase_a(
    model: HFDClassifier,
    cfg: TrainingConfig,
    device: str,
) -> HFDClassifier:
    """합성 막대인간 데이터로 사전학습."""
    synthetic_ann = Path(cfg.synthetic_dir) / "annotations.json"
    if not synthetic_ann.exists():
        logger.warning(f"합성 데이터 없음: {synthetic_ann}. Phase A 건너뜀.")
        return model

    dataset = HFDDataset(
        annotations_path=str(synthetic_ann),
        data_root=str(Path(cfg.synthetic_dir)),
        transform=get_train_transform(),
    )
    if len(dataset) == 0:
        logger.warning("합성 데이터셋이 비어 있음. Phase A 건너뜀.")
        return model

    loader = DataLoader(dataset, batch_size=cfg.phase_a_batch_size, shuffle=True, drop_last=False)
    criterion = nn.BCEWithLogitsLoss()
    optimizer = AdamW(
        filter(lambda p: p.requires_grad, model.parameters()),
        lr=cfg.phase_a_lr,
        weight_decay=cfg.weight_decay,
    )
    scheduler = CosineAnnealingLR(optimizer, T_max=cfg.phase_a_epochs)

    logger.info(f"Phase A 시작: {len(dataset)}장, {cfg.phase_a_epochs} epoch")
    for epoch in range(1, cfg.phase_a_epochs + 1):
        loss = train_epoch(model, loader, optimizer, criterion, device)
        scheduler.step()
        if epoch % 10 == 0:
            logger.info(f"  [A] epoch {epoch}/{cfg.phase_a_epochs}  loss={loss:.4f}")

    logger.info("Phase A 완료")
    return model


# ── Phase B: 실제 데이터 미세조정 ────────────────────────────────────────
def run_phase_b(
    model: HFDClassifier,
    cfg: TrainingConfig,
    figure_gender: str,
    device: str,
) -> HFDClassifier:
    """실제 아동화 데이터로 미세조정."""
    ann_path = Path(cfg.annotations_path)
    if not ann_path.exists():
        logger.warning(f"실제 데이터 없음: {ann_path}. Phase B 건너뜀.")
        return model

    dataset = HFDDataset(
        annotations_path=str(ann_path),
        data_root=cfg.data_dir,
        transform=get_train_transform(),
        figure_gender=figure_gender,
    )
    if len(dataset) == 0:
        logger.warning(f"figure_gender={figure_gender} 데이터 없음. Phase B 건너뜀.")
        return model

    loader = DataLoader(dataset, batch_size=cfg.phase_b_batch_size, shuffle=True, drop_last=False)
    criterion = nn.BCEWithLogitsLoss()
    optimizer = AdamW(
        filter(lambda p: p.requires_grad, model.parameters()),
        lr=cfg.phase_b_lr,
        weight_decay=cfg.weight_decay,
    )
    scheduler = CosineAnnealingLR(optimizer, T_max=cfg.phase_b_epochs)

    best_loss = float("inf")
    patience_counter = 0
    logger.info(f"Phase B 시작: {len(dataset)}장, 최대 {cfg.phase_b_epochs} epoch")

    for epoch in range(1, cfg.phase_b_epochs + 1):
        loss = train_epoch(model, loader, optimizer, criterion, device)
        scheduler.step()

        if epoch % 10 == 0:
            logger.info(f"  [B] epoch {epoch}/{cfg.phase_b_epochs}  loss={loss:.4f}")

        if loss < best_loss - 1e-4:
            best_loss = loss
            patience_counter = 0
        else:
            patience_counter += 1
            if patience_counter >= cfg.phase_b_early_stop_patience:
                logger.info(f"조기 종료: epoch {epoch}, best_loss={best_loss:.4f}")
                break

    logger.info("Phase B 완료")
    return model


# ── 저장 ─────────────────────────────────────────────────────────────────
def save_checkpoint(model: HFDClassifier, path: str):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    torch.save(model.state_dict(), path)
    logger.info(f"모델 저장: {path}")


# ── 메인 ─────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="HFD 모델 학습")
    parser.add_argument("--gender", choices=["male", "female"], default="male")
    parser.add_argument("--device", default="cpu")
    parser.add_argument("--skip-phase-a", action="store_true")
    args = parser.parse_args()

    model_cfg = ModelConfig()
    train_cfg = TrainingConfig(device=args.device)
    device = args.device

    model = HFDClassifier(model_cfg).to(device)

    if not args.skip_phase_a:
        model = run_phase_a(model, train_cfg, device)

    model = run_phase_b(model, train_cfg, figure_gender=args.gender, device=device)

    out_path = os.path.join(
        train_cfg.output_dir,
        f"mind_palette_{args.gender}.pt",
    )
    save_checkpoint(model, out_path)
    logger.info(f"학습 완료. 저장 경로: {out_path}")


if __name__ == "__main__":
    main()
