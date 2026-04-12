"""
AI 서버 파이프라인 단계별 시각화 생성 도구

C++ 전처리 서버의 최종 출력(512x512x3 PNG)을 입력받아,
AI 서버 내부 전처리 단계를 단계별 이미지로 저장한다.

의존성: numpy, Pillow, opencv-python (ai-server/.venv에 포함)

사용법:
    python generate_pipeline_stages.py <input_image> [output_dir]

출력 파일:
    01-input-512x512.png      - 입력 이미지 원본 + 3채널 분리 나란히
    02-resize-260.png         - PIL Resize 260x260
    03-channel-normalized.png - ImageNet 정규화 후 채널별 분포 시각화
"""

import sys
import numpy as np
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import cv2

IMAGENET_MEAN = [0.485, 0.456, 0.406]
IMAGENET_STD  = [0.229, 0.224, 0.225]

LABEL_H = 28  # 라벨 영역 높이 (픽셀)
PADDING = 6   # 이미지 간 간격


def _add_label(img_np: np.ndarray, text: str) -> np.ndarray:
    """이미지 위에 흰 배경 검정 텍스트 라벨 추가 (numpy 배열 입출력)"""
    h, w = img_np.shape[:2]
    label_area = np.full((LABEL_H, w, 3), 240, dtype=np.uint8)
    pil = Image.fromarray(label_area)
    draw = ImageDraw.Draw(pil)
    draw.text((4, 4), text, fill=(30, 30, 30))
    label_np = np.array(pil)
    return np.vstack([label_np, img_np])


def _gray_to_bgr(gray: np.ndarray) -> np.ndarray:
    return cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)


def _hstack_with_gap(*images: np.ndarray) -> np.ndarray:
    """같은 높이의 이미지들을 가로로 이어붙임 (PADDING 간격)"""
    h = max(img.shape[0] for img in images)
    padded = []
    for img in images:
        if img.shape[0] < h:
            pad = np.full((h - img.shape[0], img.shape[1], 3), 255, dtype=np.uint8)
            img = np.vstack([img, pad])
        padded.append(img)
        padded.append(np.full((h, PADDING, 3), 255, dtype=np.uint8))
    return np.hstack(padded[:-1])  # 마지막 gap 제거


def _minmax_vis(ch: np.ndarray) -> np.ndarray:
    """float 채널을 0~255 uint8로 min-max 정규화해 반환"""
    lo, hi = ch.min(), ch.max()
    norm = (ch - lo) / (hi - lo + 1e-8)
    return (norm * 255).astype(np.uint8)


def stage_01_input(img_bgr: np.ndarray, output_dir: Path) -> None:
    """입력 이미지 원본 + 채널 분리 나란히 (BGR로 저장)"""
    # C++ merge 순서: {ch_R(gray), ch_G(inv_binary), ch_B(distance)}
    # cv2.split → [0]=gray, [1]=inv_binary, [2]=distance
    ch0, ch1, ch2 = cv2.split(img_bgr)
    labels = [
        ("Original (3ch)", img_bgr),
        ("Ch0  R-Gray", _gray_to_bgr(ch0)),
        ("Ch1  G-InvBinary", _gray_to_bgr(ch1)),
        ("Ch2  B-Distance", _gray_to_bgr(ch2)),
    ]
    tiles = [_add_label(img, lbl) for lbl, img in labels]
    composite = _hstack_with_gap(*tiles)
    out_path = str(output_dir / "01-input-512x512.png")
    cv2.imwrite(out_path, composite)
    print(f"  Saved: {out_path}")


def stage_02_resize(img_pil: Image.Image, output_dir: Path) -> Image.Image:
    """PIL Resize 260x260"""
    resized = img_pil.resize((260, 260), Image.BILINEAR)
    out_path = str(output_dir / "02-resize-260.png")
    resized.save(out_path)
    print(f"  Saved: {out_path}")
    return resized


def stage_03_normalized(resized_pil: Image.Image, output_dir: Path) -> None:
    """ImageNet 정규화 후 각 채널을 가시화한 비교 이미지 저장"""
    img_np = np.array(resized_pil).astype(np.float32) / 255.0  # (260,260,3) RGB 0~1
    mean = np.array(IMAGENET_MEAN, dtype=np.float32)
    std  = np.array(IMAGENET_STD,  dtype=np.float32)
    normed = (img_np - mean) / std  # ~[-2.5, 2.5]

    CELL_W, CELL_H = 260, 260
    BAR_W = 260  # 히스토그램 바 차트 너비
    BAR_H = 120  # 히스토그램 높이
    N_BINS = 50

    channel_names = [
        "Ch0 R-Gray  (normalized)",
        "Ch1 G-InvBinary  (normalized)",
        "Ch2 B-Distance  (normalized)",
    ]
    bar_colors = [(60, 80, 200), (60, 170, 60), (200, 60, 60)]  # BGR

    col_images = []
    for i in range(3):
        ch = normed[:, :, i]

        # 위: 채널 이미지 min-max 시각화
        ch_vis = _minmax_vis(ch)
        cell = _gray_to_bgr(ch_vis)
        cell = _add_label(cell, channel_names[i])

        # 아래: 히스토그램 바 차트 (opencv로 직접 그림)
        hist_img = np.full((BAR_H + 30, BAR_W, 3), 245, dtype=np.uint8)
        counts, edges = np.histogram(ch.flatten(), bins=N_BINS)
        max_count = counts.max() if counts.max() > 0 else 1
        bar_w = max(1, BAR_W // N_BINS)
        for b in range(N_BINS):
            bar_h = int(counts[b] / max_count * (BAR_H - 10))
            x1 = b * bar_w
            x2 = x1 + bar_w - 1
            y1 = BAR_H - bar_h
            y2 = BAR_H
            cv2.rectangle(hist_img, (x1, y1), (x2, y2), bar_colors[i], -1)
        # 0 기준선
        zero_frac = (-ch.min()) / (ch.max() - ch.min() + 1e-8)
        zero_x = int(zero_frac * BAR_W)
        cv2.line(hist_img, (zero_x, 0), (zero_x, BAR_H), (0, 0, 0), 1)
        # 통계 텍스트
        stat_txt = f"mean={ch.mean():.2f}  std={ch.std():.2f}"
        cv2.putText(hist_img, stat_txt, (4, BAR_H + 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.38, (40, 40, 40), 1)
        col_images.append(np.vstack([cell, hist_img]))

    composite = _hstack_with_gap(*col_images)

    # 상단 타이틀 바
    title_bar = np.full((32, composite.shape[1], 3), 220, dtype=np.uint8)
    title_txt = f"Stage 03: ImageNet Normalize  mean={IMAGENET_MEAN}  std={IMAGENET_STD}"
    cv2.putText(title_bar, title_txt, (6, 22),
                cv2.FONT_HERSHEY_SIMPLEX, 0.42, (20, 20, 20), 1)
    composite = np.vstack([title_bar, composite])

    out_path = str(output_dir / "03-channel-normalized.png")
    cv2.imwrite(out_path, composite)
    print(f"  Saved: {out_path}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_pipeline_stages.py <input_image> [output_dir]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_dir = Path(sys.argv[2]) if len(sys.argv) >= 3 else Path("docs/pipeline-stages/images/ai-server")
    output_dir.mkdir(parents=True, exist_ok=True)

    img_bgr = cv2.imread(input_path)
    if img_bgr is None:
        print(f"ERROR: 이미지를 읽을 수 없음: {input_path}")
        sys.exit(1)
    img_pil = Image.fromarray(cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB))
    print(f"[Load] {input_path}  size={img_pil.size}")

    print("[01] Input channels visualization...")
    stage_01_input(img_bgr, output_dir)

    print("[02] Resize to 260x260...")
    resized = stage_02_resize(img_pil, output_dir)

    print("[03] ImageNet normalization visualization...")
    stage_03_normalized(resized, output_dir)

    print(f"\nDone. {output_dir} 에 3개 이미지 저장됨.")


if __name__ == "__main__":
    main()
