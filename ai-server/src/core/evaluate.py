"""HFD 모델 평가 모듈.

Head별 Accuracy, F1, 원점수 MAE, LOOCV 지원.
scikit-learn 없이 순수 numpy/torch로 구현.
"""

from typing import Dict, List, Tuple

import numpy as np
import torch


def compute_accuracy(preds: torch.Tensor, targets: torch.Tensor, threshold: float = 0.5) -> float:
    """이진 분류 정확도 계산.

    Args:
        preds: 로짓 또는 확률 텐서 (N,)
        targets: 실제 레이블 텐서 (N,) — 0 또는 1
        threshold: 이진화 임계값

    Returns:
        정확도 (0.0~1.0)
    """
    binary_preds = (torch.sigmoid(preds) >= threshold).float()
    return (binary_preds == targets).float().mean().item()


def compute_f1(preds: torch.Tensor, targets: torch.Tensor, threshold: float = 0.5) -> float:
    """이진 F1 스코어 계산.

    Args:
        preds: 로짓 또는 확률 텐서
        targets: 실제 레이블 텐서
        threshold: 이진화 임계값

    Returns:
        F1 스코어 (0.0~1.0)
    """
    binary_preds = (torch.sigmoid(preds) >= threshold).float()
    tp = ((binary_preds == 1) & (targets == 1)).float().sum().item()
    fp = ((binary_preds == 1) & (targets == 0)).float().sum().item()
    fn = ((binary_preds == 0) & (targets == 1)).float().sum().item()

    precision = tp / (tp + fp + 1e-8)
    recall = tp / (tp + fn + 1e-8)
    f1 = 2 * precision * recall / (precision + recall + 1e-8)
    return f1


def compute_raw_score_mae(
    pred_logits: Dict[str, torch.Tensor],
    true_labels: Dict[str, torch.Tensor],
    threshold: float = 0.5,
) -> float:
    """예측 원점수와 실제 원점수의 MAE(평균 절대 오차) 계산.

    Args:
        pred_logits: {"head_a": tensor, ...} — 모델 출력 로짓
        true_labels: {"head_a": tensor, ...} — 실제 레이블
        threshold: 이진화 임계값

    Returns:
        원점수 MAE
    """
    pred_score = sum(
        (torch.sigmoid(logit) >= threshold).float().sum().item()
        for logit in pred_logits.values()
    )
    true_score = sum(label.sum().item() for label in true_labels.values())
    return abs(pred_score - true_score)


def evaluate_heads(
    pred_logits: Dict[str, torch.Tensor],
    true_labels: Dict[str, torch.Tensor],
    threshold: float = 0.5,
) -> Dict[str, Dict[str, float]]:
    """각 Head에 대해 accuracy와 f1을 계산한다.

    Returns:
        {"head_a": {"accuracy": 0.8, "f1": 0.75}, ...}
    """
    results = {}
    for head in pred_logits:
        preds = pred_logits[head]
        targets = true_labels[head]
        results[head] = {
            "accuracy": compute_accuracy(preds, targets, threshold),
            "f1": compute_f1(preds, targets, threshold),
        }
    return results


def loocv_evaluate(
    dataset,
    model_factory,
    train_fn,
    threshold: float = 0.5,
) -> Dict[str, float]:
    """Leave-One-Out Cross-Validation 평가.

    Args:
        dataset: HFDDataset 인스턴스 (len >= 2)
        model_factory: () → model 생성 함수
        train_fn: (model, train_indices) → 학습된 model 반환 함수
        threshold: 예측 임계값

    Returns:
        {"mean_raw_score_mae": float, "std_raw_score_mae": float}
    """
    n = len(dataset)
    if n < 2:
        raise ValueError("LOOCV는 최소 2개 샘플이 필요합니다.")

    maes = []
    for i in range(n):
        train_indices = [j for j in range(n) if j != i]
        model = model_factory()
        model = train_fn(model, train_indices)
        model.eval()

        img, true_labels = dataset[i]
        with torch.no_grad():
            pred_logits = model(img.unsqueeze(0))

        # 모델 출력이 튜플인 경우 dict로 변환
        if isinstance(pred_logits, (tuple, list)):
            head_names = ["head_a", "head_b", "head_c", "head_d"]
            pred_dict = {
                head_names[k]: pred_logits[k].squeeze(0)
                for k in range(len(pred_logits))
            }
        else:
            pred_dict = {k: v.squeeze(0) for k, v in pred_logits.items()}

        mae = compute_raw_score_mae(pred_dict, true_labels, threshold)
        maes.append(mae)

    return {
        "mean_raw_score_mae": float(np.mean(maes)),
        "std_raw_score_mae": float(np.std(maes)),
        "per_fold_mae": maes,
    }
