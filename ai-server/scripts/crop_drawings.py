"""스캔 이미지에서 그림 영역만 크롭하는 스크립트.

각 이미지에는 2개의 그림이 세로로 배치되어 있음.
아래쪽 그림 = sample_A, 위쪽 그림 = sample_B

사용법:
    python scripts/crop_drawings.py
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

# 소스 이미지 목록 (바탕화면)
SOURCE_DIR = Path(r"C:\Users\user\Desktop\아동화 모음")
OUT_DIR = Path("data/raw")

# 각 이미지에서 크롭할 두 그림 영역 정의
# 형식: (파일명, [(x1, y1, x2, y2), (x1, y1, x2, y2)])
# 각 이미지를 확인 후 그림 박스 좌표를 수동으로 정의
# 이미지는 세로로 두 그림이 있고, 채점표는 오른쪽에 있음
# 그림 영역만 크롭 (채점표 제외)

CROP_SPECS = [
    # (파일명, sample_id_bottom, 하단_크롭박스, sample_id_top, 상단_크롭박스)
    (
        "스크린샷 2026-03-16 215840.png",
        "sample_001",  # 전국-남아-남자상 MA=10-6 원점수=29
        None,          # 자동 크롭 (하단 절반의 그림 영역)
        "sample_002",  # 전국-남아-여자상 MA=10-0 원점수=28
        None,          # 자동 크롭 (상단 절반의 그림 영역)
    ),
    (
        "스크린샷 2026-03-16 215926.png",
        "sample_003",  # 전국-여아-남자상 MA=9-0 원점수=27
        None,
        "sample_004",  # 전국-여아-여자상 MA=8-3 원점수=28
        None,
    ),
    (
        "스크린샷 2026-03-16 220028.png",
        "sample_005",  # 전국-남아-남자상 MA=5-9 원점수=15
        None,
        "sample_006",  # 전국-남아-여자상 MA=5-6 원점수=14
        None,
    ),
    (
        "스크린샷 2026-03-16 220045.png",
        "sample_007",  # 전국-남아-남자상 MA=4-9 원점수=12
        None,
        "sample_008",  # 전국-여아-여자상 MA=3-6 원점수=12
        None,
    ),
    (
        "화면 캡처 2026-03-16 220008.png",
        "sample_009",  # 전국-남아-남자상 MA=7-0 원점수=18
        None,
        "sample_010",  # 전국-여아-여자상 MA=7-9 원점수=28
        None,
    ),
    (
        "화면 캡처 2026-03-16 220107.png",
        "sample_011",  # 전국-여아-남자상 MA=3-6 원점수=8
        None,
        "sample_012",  # 전국-여아-남자상 MA=4-6 원점수=14
        None,
    ),
    (
        "화면 캡처 2026-03-16 220120.png",
        "sample_013",  # 전국-남아-남자상 MA=4-9 원점수=12
        None,
        "sample_014",  # 전국-여아-여자상 MA=3-9 원점수=13
        None,
    ),
    (
        "화면 캡처 2026-03-16 220132.png",
        "sample_015",  # 전국-여아-남자상 MA=3-6 원점수=8
        None,
        "sample_016",  # 전국-여아-남자상 MA=3-3 원점수=10
        None,
    ),
    (
        "화면 캡처 2026-03-16 220148.png",
        "sample_017",  # 전국-남아-남자상 MA=4-0 원점수=10
        None,
        "sample_018",  # 전국-여아-여자상 MA=3-0 원점수=10
        None,
    ),
    (
        "화면 캡처 2026-03-16 220201.png",
        "sample_019",  # 전국-여아-남자상 MA=3-6 원점수=8
        None,
        "sample_020",  # 전국-남아-남자상 MA=3-0 원점수=9
        None,
    ),
]


def _find_figure_start_row(section_gray: np.ndarray) -> int:
    """채점표 행들 이후 그림이 시작하는 행 번호를 찾는다.

    채점표: 섹션 상단 약 30~40% (작은 O 마킹들이 모여 있는 행)
    그림: 섹션 하단 약 40~70% (연속된 넓은 어두운 영역)
    """
    h = section_gray.shape[0]
    row_darkness = (255 - section_gray).mean(axis=1)

    # 채점표 영역 (상단 40%)를 건너뛰고, 그 이후에 그림 시작점 탐색
    skip_rows = int(h * 0.38)
    # 이후 구간에서 연속 어두운 행이 나오는 첫 위치
    for i in range(skip_rows, h):
        # 연속 3행이 어두우면 그림 시작
        if (row_darkness[i] > 8 and i + 2 < h
                and row_darkness[i + 1] > 8
                and row_darkness[i + 2] > 8):
            return max(0, i - 5)  # 약간 여유
    return skip_rows


def auto_crop_bottom_figure(img: Image.Image) -> Image.Image:
    """하단 그림 영역 자동 크롭.

    이미지 구조:
    - 세로로 두 섹션 (상단/하단)
    - 각 섹션: 위쪽 = 채점표(O마킹 행들), 아래쪽 = 그림
    """
    w, h = img.size
    arr = np.array(img).mean(axis=2)
    half_h = h // 2

    bottom_gray = arr[half_h:, :]
    fig_start = _find_figure_start_row(bottom_gray)

    # 양쪽 여백 제거 (세로 레이블 텍스트 영역 제외)
    left_margin = int(w * 0.08)
    right_margin = int(w * 0.05)
    return img.crop((left_margin, half_h + fig_start, w - right_margin, h - 5))


def auto_crop_top_figure(img: Image.Image) -> Image.Image:
    """상단 그림 영역 자동 크롭."""
    w, h = img.size
    arr = np.array(img).mean(axis=2)
    half_h = h // 2

    top_gray = arr[:half_h, :]
    fig_start = _find_figure_start_row(top_gray)

    left_margin = int(w * 0.08)
    right_margin = int(w * 0.05)
    return img.crop((left_margin, fig_start, w - right_margin, half_h - 5))


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    cropped = []

    for spec in CROP_SPECS:
        fname, sid_bottom, box_bottom, sid_top, box_top = spec
        src_path = SOURCE_DIR / fname
        if not src_path.exists():
            print(f"[SKIP] 파일 없음: {src_path}")
            continue

        img = Image.open(src_path).convert("RGB")
        print(f"[INFO] {fname}: {img.size}")

        # 하단 그림 크롭
        if box_bottom:
            bottom_crop = img.crop(box_bottom)
        else:
            bottom_crop = auto_crop_bottom_figure(img)
        # 그림이 90도 시계방향 회전되어 스캔됨 → 반시계방향 90도 회전으로 정립
        bottom_crop = bottom_crop.rotate(270, expand=True)
        out_bottom = OUT_DIR / f"{sid_bottom}.png"
        bottom_crop.save(out_bottom)
        print(f"  → {out_bottom} {bottom_crop.size}")
        cropped.append(sid_bottom)

        # 상단 그림 크롭
        if box_top:
            top_crop = img.crop(box_top)
        else:
            top_crop = auto_crop_top_figure(img)
        top_crop = top_crop.rotate(270, expand=True)
        out_top = OUT_DIR / f"{sid_top}.png"
        top_crop.save(out_top)
        print(f"  → {out_top} {top_crop.size}")
        cropped.append(sid_top)

    print(f"\n완료: {len(cropped)}개 그림 크롭됨 → {OUT_DIR}/")


if __name__ == "__main__":
    main()
