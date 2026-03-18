"""HFD 60문항 ↔ 4개 Head 매핑 모듈.

Head A (머리/얼굴):  문항 1,4-21      → 19개
Head B (몸통/비례):  문항 2,3,29,30,40-49 → 14개
Head C (사지/말단):  문항 22-28,31-39 → 16개
Head D (의복/질적):  문항 50-60       → 11개

여자 척도(Female Scale) 예외 문항:
  남자상 문항 중 여자아이에 대해 다른 기준이 적용되는 6문항.
  (현재 구현에서는 동일 문항 번호 유지; 향후 모델 분리 시 활용)
"""

from typing import Dict, List

# fmt: off
HEAD_A_ITEMS: List[int] = [1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21]
HEAD_B_ITEMS: List[int] = [2, 3, 29, 30, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]
HEAD_C_ITEMS: List[int] = [22, 23, 24, 25, 26, 27, 28, 31, 32, 33, 34, 35, 36, 37, 38, 39]
HEAD_D_ITEMS: List[int] = [50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60]
# fmt: on

ALL_ITEMS: List[int] = HEAD_A_ITEMS + HEAD_B_ITEMS + HEAD_C_ITEMS + HEAD_D_ITEMS

# 여자 척도 예외 6문항 (여자아이가 여자상을 그릴 때 다른 기준 적용)
FEMALE_EXCEPTION_ITEMS: List[int] = [2, 3, 52, 53, 54, 55]

_HEAD_ORDER = [
    ("head_a", HEAD_A_ITEMS),
    ("head_b", HEAD_B_ITEMS),
    ("head_c", HEAD_C_ITEMS),
    ("head_d", HEAD_D_ITEMS),
]


def items_to_head_labels(
    items: Dict[str, int], scale: str = "male"
) -> Dict[str, List[int]]:
    """60문항 딕셔너리를 4개 Head 레이블 딕셔너리로 변환.

    Args:
        items: {"1": 0, "2": 1, ..., "60": 0} 형태의 문항-점수 딕셔너리
        scale: "male" | "female" (여자 척도 여부; 현재는 동일 처리)

    Returns:
        {"head_a": [...], "head_b": [...], "head_c": [...], "head_d": [...]}

    Raises:
        ValueError: scale이 "male"이나 "female"이 아닌 경우
    """
    if scale not in ("male", "female"):
        raise ValueError(f"scale은 'male' 또는 'female'이어야 합니다. 받은 값: {scale!r}")

    return {
        head_name: [int(items.get(str(item_no), 0)) for item_no in item_list]
        for head_name, item_list in _HEAD_ORDER
    }
