"""합성 막대인간 데이터 생성기.

머리 원 + 몸통 + 팔/다리 선으로 아동화 유사 이미지를 생성하고,
그린 부위에 맞는 Head 레이블을 자동으로 부여한다.

출력 구조:
  data/synthetic_sketches/
    images/sketch_000.png ...
    annotations.json
"""

import json
import math
import os
import random

import cv2
import numpy as np


# ── 난이도별 부위 활성화 확률 ──────────────────────────────────
DIFFICULTY_PROFILES = {
    "easy": {
        "head": 1.0, "neck": 0.3, "torso": 0.9, "arms": 0.6,
        "legs": 0.8, "feet": 0.2, "hands": 0.1, "fingers": 0.0,
        "eyes": 0.7, "nose": 0.5, "mouth": 0.6, "ears": 0.3,
        "hair": 0.4, "clothes": 0.2, "details": 0.0,
    },
    "medium": {
        "head": 1.0, "neck": 0.7, "torso": 1.0, "arms": 0.9,
        "legs": 0.95, "feet": 0.6, "hands": 0.5, "fingers": 0.2,
        "eyes": 0.95, "nose": 0.8, "mouth": 0.9, "ears": 0.7,
        "hair": 0.75, "clothes": 0.6, "details": 0.2,
    },
    "hard": {
        "head": 1.0, "neck": 0.9, "torso": 1.0, "arms": 1.0,
        "legs": 1.0, "feet": 0.9, "hands": 0.8, "fingers": 0.6,
        "eyes": 1.0, "nose": 0.95, "mouth": 0.95, "ears": 0.9,
        "hair": 0.9, "clothes": 0.85, "details": 0.5,
    },
}


def _pick(prob: float) -> bool:
    return random.random() < prob


def draw_stick_figure(size: int = 512) -> tuple:
    """막대인간 한 장을 그리고 (이미지, 활성화된 부위 dict)를 반환."""
    img = np.full((size, size, 3), 255, dtype=np.uint8)
    difficulty = random.choice(["easy", "medium", "hard"])
    p = DIFFICULTY_PROFILES[difficulty]
    drawn = {k: False for k in p}

    cx, cy = size // 2, size // 2
    scale = size / 512

    # ── 머리 ─────────────────────────────────────────────────
    head_r = int(55 * scale)
    head_top = int(cy - 220 * scale)
    head_cx, head_cy = cx, head_top + head_r
    if _pick(p["head"]):
        cv2.circle(img, (head_cx, head_cy), head_r, (30, 30, 30), 2)
        drawn["head"] = True

    # ── 목 ──────────────────────────────────────────────────
    neck_top = head_cy + head_r
    neck_bot = head_cy + head_r + int(25 * scale)
    if drawn["head"] and _pick(p["neck"]):
        cv2.line(img, (cx, neck_top), (cx, neck_bot), (30, 30, 30), 2)
        drawn["neck"] = True

    # ── 몸통 ────────────────────────────────────────────────
    torso_top = neck_bot if drawn["neck"] else neck_top
    torso_bot = torso_top + int(130 * scale)
    tw = int(45 * scale)
    if _pick(p["torso"]):
        cv2.rectangle(img, (cx - tw, torso_top), (cx + tw, torso_bot), (30, 30, 30), 2)
        drawn["torso"] = True

    # ── 팔 ──────────────────────────────────────────────────
    if drawn["torso"] and _pick(p["arms"]):
        arm_len = int(90 * scale)
        angle = random.uniform(30, 60)
        rad = math.radians(angle)
        lx = int(cx - tw - arm_len * math.cos(rad))
        ly = int(torso_top + int(20 * scale) + arm_len * math.sin(rad))
        rx = int(cx + tw + arm_len * math.cos(rad))
        ry = ly
        cv2.line(img, (cx - tw, torso_top + int(20 * scale)), (lx, ly), (30, 30, 30), 2)
        cv2.line(img, (cx + tw, torso_top + int(20 * scale)), (rx, ry), (30, 30, 30), 2)
        drawn["arms"] = True

        # 손
        if _pick(p["hands"]):
            cv2.circle(img, (lx, ly), int(8 * scale), (30, 30, 30), 2)
            cv2.circle(img, (rx, ry), int(8 * scale), (30, 30, 30), 2)
            drawn["hands"] = True

        # 손가락
        if drawn["hands"] and _pick(p["fingers"]):
            for i in range(5):
                fa = math.radians(-80 + i * 40)
                flen = int(15 * scale)
                cv2.line(img, (lx, ly),
                         (lx + int(flen * math.cos(fa)), ly + int(flen * math.sin(fa))),
                         (30, 30, 30), 1)
                cv2.line(img, (rx, ry),
                         (rx + int(flen * math.cos(fa)), ry + int(flen * math.sin(fa))),
                         (30, 30, 30), 1)
            drawn["fingers"] = True

    # ── 다리 ────────────────────────────────────────────────
    if drawn["torso"] and _pick(p["legs"]):
        leg_len = int(110 * scale)
        spread = int(30 * scale)
        ll_bot = (cx - spread, torso_bot + leg_len)
        rl_bot = (cx + spread, torso_bot + leg_len)
        cv2.line(img, (cx - spread // 2, torso_bot), ll_bot, (30, 30, 30), 2)
        cv2.line(img, (cx + spread // 2, torso_bot), rl_bot, (30, 30, 30), 2)
        drawn["legs"] = True

        # 발
        if _pick(p["feet"]):
            ft = int(20 * scale)
            cv2.ellipse(img, (ll_bot[0] - ft // 2, ll_bot[1]), (ft, int(8 * scale)), 0, 0, 180, (30, 30, 30), 2)
            cv2.ellipse(img, (rl_bot[0] + ft // 2, rl_bot[1]), (ft, int(8 * scale)), 0, 0, 180, (30, 30, 30), 2)
            drawn["feet"] = True

    if not drawn["head"]:
        return img, drawn

    # ── 얼굴 세부 ────────────────────────────────────────────
    if _pick(p["eyes"]):
        eo = int(18 * scale)
        er = int(5 * scale)
        cv2.circle(img, (head_cx - eo, head_cy - int(5 * scale)), er, (30, 30, 30), -1)
        cv2.circle(img, (head_cx + eo, head_cy - int(5 * scale)), er, (30, 30, 30), -1)
        drawn["eyes"] = True

    if _pick(p["nose"]):
        nl = int(8 * scale)
        cv2.line(img, (head_cx, head_cy), (head_cx + int(6 * scale), head_cy + nl), (30, 30, 30), 2)
        drawn["nose"] = True

    if _pick(p["mouth"]):
        axes = (int(18 * scale), int(8 * scale))
        cv2.ellipse(img, (head_cx, head_cy + int(20 * scale)), axes, 0, 0, 180, (30, 30, 30), 2)
        drawn["mouth"] = True

    if _pick(p["ears"]):
        er2 = int(10 * scale)
        cv2.circle(img, (head_cx - head_r, head_cy), er2, (30, 30, 30), 2)
        cv2.circle(img, (head_cx + head_r, head_cy), er2, (30, 30, 30), 2)
        drawn["ears"] = True

    if _pick(p["hair"]):
        for _ in range(random.randint(4, 10)):
            hx = head_cx + random.randint(-head_r, head_r)
            cv2.line(img, (hx, head_cy - head_r),
                     (hx + random.randint(-5, 5), head_cy - head_r - int(15 * scale)),
                     (30, 30, 30), 2)
        drawn["hair"] = True

    # ── 옷 ──────────────────────────────────────────────────
    if drawn["torso"] and _pick(p["clothes"]):
        # 셔츠 칼라 표시
        cv2.line(img, (cx - int(15 * scale), torso_top),
                 (cx, torso_top + int(20 * scale)), (30, 30, 30), 1)
        cv2.line(img, (cx + int(15 * scale), torso_top),
                 (cx, torso_top + int(20 * scale)), (30, 30, 30), 1)
        drawn["clothes"] = True

    return img, drawn


def _drawn_to_labels(drawn: dict) -> dict:
    """부위 활성화 dict를 60문항 레이블 dict로 변환.

    실제 HFD 채점 항목과 1:1 대응하지는 않지만,
    합성 사전학습용으로 충분한 근사치를 제공한다.
    """
    d = drawn

    # fmt: off
    items = {
        # Head A: 머리/얼굴 (문항 1,4-21) → 19개
        "1":  int(d["head"]),
        "4":  int(d["head"]),
        "5":  int(d["neck"]),
        "6":  int(d["eyes"]),
        "7":  int(d["eyes"]),
        "8":  int(d["nose"]),
        "9":  int(d["mouth"]),
        "10": int(d["ears"]),
        "11": int(d["hair"]),
        "12": int(d["hair"]),
        "13": int(d["head"]),
        "14": int(d["eyes"]),
        "15": int(d["nose"]),
        "16": int(d["mouth"]),
        "17": int(d["ears"]),
        "18": int(d["hair"]),
        "19": int(d["details"]),
        "20": int(d["details"]),
        "21": int(d["details"]),
        # Head B: 몸통/비례 (문항 2,3,29,30,40-49) → 14개
        "2":  int(d["torso"]),
        "3":  int(d["torso"]),
        "29": int(d["torso"]),
        "30": int(d["torso"]),
        "40": int(d["torso"]),
        "41": int(d["arms"]),
        "42": int(d["arms"]),
        "43": int(d["torso"]),
        "44": int(d["clothes"]),
        "45": int(d["clothes"]),
        "46": int(d["details"]),
        "47": int(d["details"]),
        "48": int(d["details"]),
        "49": int(d["details"]),
        # Head C: 사지/말단 (문항 22-28,31-39) → 16개
        "22": int(d["arms"]),
        "23": int(d["arms"]),
        "24": int(d["hands"]),
        "25": int(d["hands"]),
        "26": int(d["fingers"]),
        "27": int(d["fingers"]),
        "28": int(d["legs"]),
        "31": int(d["legs"]),
        "32": int(d["legs"]),
        "33": int(d["feet"]),
        "34": int(d["feet"]),
        "35": int(d["details"]),
        "36": int(d["details"]),
        "37": int(d["details"]),
        "38": int(d["details"]),
        "39": int(d["details"]),
        # Head D: 의복/질적 (문항 50-60) → 11개
        "50": int(d["clothes"]),
        "51": int(d["clothes"]),
        "52": int(d["details"]),
        "53": int(d["details"]),
        "54": int(d["details"]),
        "55": int(d["details"]),
        "56": int(d["details"]),
        "57": int(d["details"]),
        "58": int(d["details"]),
        "59": int(d["details"]),
        "60": int(d["details"]),
    }
    # fmt: on

    raw_score = sum(items.values())
    return items, raw_score


def _items_to_head_labels(items: dict) -> dict:
    HEAD_A = [1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21]
    HEAD_B = [2, 3, 29, 30, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]
    HEAD_C = [22, 23, 24, 25, 26, 27, 28, 31, 32, 33, 34, 35, 36, 37, 38, 39]
    HEAD_D = [50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60]
    return {
        "head_a": [items[str(i)] for i in HEAD_A],
        "head_b": [items[str(i)] for i in HEAD_B],
        "head_c": [items[str(i)] for i in HEAD_C],
        "head_d": [items[str(i)] for i in HEAD_D],
    }


def _to_hybrid_3ch(img_bgr: np.ndarray) -> np.ndarray:
    """BGR → 하이브리드 3채널 (R=Gray, G=Binary, B=DistTransform)."""
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)
    _, binary = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY_INV)
    dist = cv2.distanceTransform(binary, cv2.DIST_L2, 3)
    dist = cv2.normalize(dist, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
    return cv2.merge([gray, binary, dist])


def main(output_dir: str = "data/synthetic_sketches", count: int = 500):
    images_dir = os.path.join(output_dir, "images")
    os.makedirs(images_dir, exist_ok=True)

    samples = []
    for i in range(count):
        img_bgr, drawn = draw_stick_figure(size=512)
        hybrid = _to_hybrid_3ch(img_bgr)

        fname = f"sketch_{i:04d}.png"
        cv2.imwrite(os.path.join(images_dir, fname), hybrid)

        items, raw_score = _drawn_to_labels(drawn)
        head_labels = _items_to_head_labels(items)
        gender = random.choice(["male", "female"])

        samples.append({
            "id": f"synth_{i:04d}",
            "image_path": f"images/{fname}",
            "child_gender": gender,
            "figure_gender": gender,
            "child_age": random.randint(5, 12),
            "raw_score": raw_score,
            "items": items,
            "head_labels": head_labels,
        })

    ann_path = os.path.join(output_dir, "annotations.json")
    with open(ann_path, "w", encoding="utf-8") as f:
        json.dump({"samples": samples}, f, ensure_ascii=False, indent=2)

    print(f"Generated {count} synthetic sketches in {output_dir}")
    print(f"Annotations saved to {ann_path}")


if __name__ == "__main__":
    main()
