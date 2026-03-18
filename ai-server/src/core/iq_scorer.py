"""HFD IQ 점수 및 백분위 계산 모듈.

IQ 산출 공식: IQ = 100 + 15 * ((raw_score - M) / SD)
출처: 전국 규준 통계치와 백분위 매핑 데이터.md
"""

from typing import Dict

# 전국 규준 데이터 (연령 3~13세)
# 구조: NORMS[child_gender][figure_gender][age] = {"M": float, "SD": float}
NATIONAL_NORMS: Dict = {
    "male": {
        "male": {
            3: {"M": 8.7, "SD": 4.1}, 4: {"M": 11.2, "SD": 4.6}, 5: {"M": 13.9, "SD": 5.0},
            6: {"M": 16.5, "SD": 5.6}, 7: {"M": 20.2, "SD": 6.4}, 8: {"M": 23.5, "SD": 7.0},
            9: {"M": 26.2, "SD": 7.1}, 10: {"M": 28.8, "SD": 7.6}, 11: {"M": 30.9, "SD": 7.6},
            12: {"M": 32.2, "SD": 7.6}, 13: {"M": 31.8, "SD": 7.7},
        },
        "female": {
            3: {"M": 7.7, "SD": 4.8}, 4: {"M": 10.3, "SD": 5.3}, 5: {"M": 13.1, "SD": 5.7},
            6: {"M": 15.9, "SD": 6.1}, 7: {"M": 19.6, "SD": 6.5}, 8: {"M": 23.2, "SD": 7.3},
            9: {"M": 25.9, "SD": 7.4}, 10: {"M": 28.0, "SD": 7.6}, 11: {"M": 29.7, "SD": 7.8},
            12: {"M": 30.7, "SD": 8.2}, 13: {"M": 30.4, "SD": 8.0},
        },
    },
    "female": {
        "male": {
            3: {"M": 10.8, "SD": 5.7}, 4: {"M": 13.4, "SD": 6.0}, 5: {"M": 16.1, "SD": 6.1},
            6: {"M": 19.4, "SD": 6.3}, 7: {"M": 22.6, "SD": 6.8}, 8: {"M": 25.4, "SD": 7.6},
            9: {"M": 27.8, "SD": 8.0}, 10: {"M": 29.8, "SD": 8.2}, 11: {"M": 31.6, "SD": 8.2},
            12: {"M": 33.1, "SD": 8.3}, 13: {"M": 33.8, "SD": 8.3},
        },
        "female": {
            3: {"M": 12.4, "SD": 6.5}, 4: {"M": 15.6, "SD": 6.6}, 5: {"M": 18.5, "SD": 7.0},
            6: {"M": 21.9, "SD": 7.4}, 7: {"M": 25.4, "SD": 7.9}, 8: {"M": 28.2, "SD": 8.6},
            9: {"M": 31.0, "SD": 9.4}, 10: {"M": 33.2, "SD": 9.6}, 11: {"M": 33.7, "SD": 9.6},
            12: {"M": 35.7, "SD": 9.7}, 13: {"M": 36.3, "SD": 9.9},
        },
    },
}

# IQ → 백분위 매핑 (규준표 기반)
IQ_TO_PERCENTILE: Dict[int, int] = {
    133: 99, 132: 98, 131: 98, 130: 98, 129: 97, 128: 97, 127: 96, 126: 96,
    125: 95, 124: 95, 123: 94, 122: 93, 121: 92, 120: 91, 119: 90, 118: 88,
    117: 87, 116: 86, 115: 84, 114: 82, 113: 81, 112: 79, 111: 77, 110: 75,
    109: 73, 108: 71, 107: 68, 106: 66, 105: 63, 104: 61, 103: 58, 102: 55,
    101: 51, 100: 50, 99: 47, 98: 45, 97: 42, 96: 39, 95: 37, 94: 34,
    93: 32, 92: 29, 91: 27, 90: 25, 89: 23, 88: 21, 87: 19, 86: 18,
    85: 16, 84: 14, 83: 13, 82: 12, 81: 10, 80: 9, 79: 8, 78: 7,
    77: 6, 76: 5, 75: 5, 74: 4, 73: 4, 72: 3, 71: 3, 70: 2,
    69: 2, 68: 2, 67: 1,
}

IQ_MIN = 67
IQ_MAX = 133
SUPPORTED_AGES = set(range(3, 14))  # 만 3~13세


def calculate_iq(
    raw_score: float,
    age: int,
    child_gender: str,
    figure_gender: str,
) -> int:
    """원점수로부터 IQ(표준점수)를 산출한다.

    Args:
        raw_score: 60문항 합계 원점수
        age: 아동 만 나이 (3~13세)
        child_gender: "male" | "female"
        figure_gender: "male" | "female"

    Returns:
        IQ 정수 (67~133 범위로 클램핑됨)

    Raises:
        ValueError: 유효하지 않은 인자
    """
    if child_gender not in ("male", "female"):
        raise ValueError(f"child_gender는 'male' 또는 'female'이어야 합니다: {child_gender!r}")
    if figure_gender not in ("male", "female"):
        raise ValueError(f"figure_gender는 'male' 또는 'female'이어야 합니다: {figure_gender!r}")
    if age not in SUPPORTED_AGES:
        raise ValueError(f"지원 연령은 3~13세입니다. 받은 값: {age}")

    norm = NATIONAL_NORMS[child_gender][figure_gender][age]
    m, sd = norm["M"], norm["SD"]
    iq_raw = 100 + 15 * ((raw_score - m) / sd)
    return max(IQ_MIN, min(IQ_MAX, round(iq_raw)))


def get_percentile(iq: int) -> int:
    """IQ(표준점수)를 백분위로 변환한다.

    Args:
        iq: IQ 점수 (범위 밖은 클램핑)

    Returns:
        백분위 (1~99)
    """
    clamped = max(IQ_MIN, min(IQ_MAX, iq))
    return IQ_TO_PERCENTILE[clamped]


def score_to_result(
    raw_score: float,
    age: int,
    child_gender: str,
    figure_gender: str,
) -> Dict:
    """원점수 → IQ + 백분위 통합 결과 반환.

    Returns:
        {"iq": int, "percentile": int, "raw_score": float}
    """
    iq = calculate_iq(raw_score, age, child_gender, figure_gender)
    percentile = get_percentile(iq)
    return {"iq": iq, "percentile": percentile, "raw_score": raw_score}
