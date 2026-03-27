"""수정 전/후 GetContentROI 비교 시각화.

- Before: 모든 컨투어 Union (부속 자극 포함)
- After:  Dominant + Proximity (부속 자극 제외)
"""
import sys
import os
import cv2
import numpy as np

IMG_PATH = "남자사람_8_남_06463.jpg"
OUT_DIR  = "roi_comparison_output"
os.makedirs(OUT_DIR, exist_ok=True)

# ── 이미지 로드 ──────────────────────────────────────────────
with open(IMG_PATH, "rb") as f:
    raw = f.read()
img_orig = cv2.imdecode(np.frombuffer(raw, np.uint8), cv2.IMREAD_COLOR)
assert img_orig is not None

# ── Hybrid Pipeline 전처리 단계 재현 ─────────────────────────
TARGET_INITIAL = 1024
h0, w0 = img_orig.shape[:2]
scale0 = min(TARGET_INITIAL / w0, TARGET_INITIAL / h0)
img_resized = cv2.resize(img_orig, (int(w0*scale0), int(h0*scale0)), interpolation=cv2.INTER_AREA)

img_denoised = cv2.GaussianBlur(img_resized, (5, 5), 0)
gray = cv2.cvtColor(img_denoised, cv2.COLOR_BGR2GRAY)

binary = cv2.adaptiveThreshold(
    gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
    cv2.THRESH_BINARY_INV, 11, 2
)
kernel3 = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
binary = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel3)

# ── 공통: 컨투어 검출 ────────────────────────────────────────
kernel5 = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
morph   = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel5)
contours, _ = cv2.findContours(morph, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

total_area = binary.shape[0] * binary.shape[1]
valid = [(cv2.contourArea(c), cv2.boundingRect(c)) for c in contours
         if cv2.contourArea(c) >= total_area * 0.001]

print(f"유효 컨투어 수: {len(valid)}")
for i, (a, r) in enumerate(sorted(valid, reverse=True)[:8]):
    print(f"  [{i+1}] area={int(a):,}  rect=({r[0]},{r[1]},{r[2]},{r[3]})")

# ── BEFORE: 모든 컨투어 Union ────────────────────────────────
def roi_before(valid_rects, img_shape):
    H, W = img_shape[:2]
    union = None
    for _, r in valid_rects:
        union = r if union is None else (
            lambda u, r: (min(u[0],r[0]), min(u[1],r[1]),
                          max(u[0]+u[2],r[0]+r[2])-min(u[0],r[0]),
                          max(u[1]+u[3],r[1]+r[3])-min(u[1],r[1]))
        )(union, r)
    if union is None:
        return (0, 0, W, H)
    pad = 10
    x = max(0, union[0]-pad); y = max(0, union[1]-pad)
    w = min(W-x, union[2]+2*pad); h = min(H-y, union[3]+2*pad)
    return (x, y, w, h)

# ── AFTER: Dominant + Proximity (margin 20%) ─────────────────
def roi_after(valid_rects, img_shape):
    H, W = img_shape[:2]
    if not valid_rects:
        return (0, 0, W, H)
    dominant_area, dominant_r = max(valid_rects, key=lambda x: x[0])
    dx, dy, dw, dh = dominant_r
    mx = int(dw * 0.2); my = int(dh * 0.2)
    ex = max(0, dx-mx)
    ey = max(0, dy-my)
    ew = min(W - dx + mx, dw + 2*mx)
    eh = min(H - dy + my, dh + 2*my)
    # expanded rect: [ex, ey, ew, eh]
    union = dominant_r
    for _, r in valid_rects:
        # intersection of expanded and r
        ix = max(ex, r[0]); iy = max(ey, r[1])
        iw = min(ex+ew, r[0]+r[2]) - ix
        ih = min(ey+eh, r[1]+r[3]) - iy
        if iw > 0 and ih > 0:
            nx = min(union[0], r[0]); ny = min(union[1], r[1])
            nw = max(union[0]+union[2], r[0]+r[2]) - nx
            nh = max(union[1]+union[3], r[1]+r[3]) - ny
            union = (nx, ny, nw, nh)
    pad = 10
    x = max(0, union[0]-pad); y = max(0, union[1]-pad)
    w = min(W-x, union[2]+2*pad); h = min(H-y, union[3]+2*pad)
    return (x, y, w, h)

rb = roi_before(valid, binary.shape)
ra = roi_after(valid, binary.shape)
print(f"\nROI Before: x={rb[0]}, y={rb[1]}, w={rb[2]}, h={rb[3]}")
print(f"ROI After:  x={ra[0]}, y={ra[1]}, w={ra[2]}, h={ra[3]}")

# ── 시각화 ───────────────────────────────────────────────────
vis_base = img_resized.copy()

# Before: 빨간 박스
vis_before = vis_base.copy()
cv2.rectangle(vis_before, (rb[0], rb[1]), (rb[0]+rb[2], rb[1]+rb[3]), (0, 0, 255), 3)
cv2.putText(vis_before, "BEFORE (all union)", (10, 40),
            cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2)

# After: Dominant 박스만 (노란색)
vis_after = vis_base.copy()
_, dom_r = max(valid, key=lambda x: x[0])
cv2.rectangle(vis_after, (dom_r[0], dom_r[1]),
              (dom_r[0]+dom_r[2], dom_r[1]+dom_r[3]), (0, 255, 255), 3)
cv2.putText(vis_after, "AFTER (dominant only)", (10, 40),
            cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 255), 2)

# ── 크롭 결과 ────────────────────────────────────────────────
crop_before = img_resized[rb[1]:rb[1]+rb[3], rb[0]:rb[0]+rb[2]]
# After crop: Dominant bbox (padding 포함)
pad = 10
dx = max(0, dom_r[0]-pad); dy = max(0, dom_r[1]-pad)
dw = min(img_resized.shape[1]-dx, dom_r[2]+2*pad)
dh = min(img_resized.shape[0]-dy, dom_r[3]+2*pad)
crop_after  = img_resized[dy:dy+dh, dx:dx+dw]

TARGET = 512
def letterbox(img):
    h, w = img.shape[:2]
    s = min(TARGET/w, TARGET/h)
    nw, nh = int(w*s), int(h*s)
    r = cv2.resize(img, (nw, nh), interpolation=cv2.INTER_AREA)
    canvas = np.full((TARGET, TARGET, 3), 255, dtype=np.uint8)
    xo, yo = (TARGET-nw)//2, (TARGET-nh)//2
    canvas[yo:yo+nh, xo:xo+nw] = r
    return canvas

final_before = letterbox(crop_before) if crop_before.size else np.full((TARGET,TARGET,3),128,dtype=np.uint8)
final_after  = letterbox(crop_after)  if crop_after.size  else np.full((TARGET,TARGET,3),128,dtype=np.uint8)

# ── 저장 ─────────────────────────────────────────────────────
cv2.imwrite(os.path.join(OUT_DIR, "1_roi_before.jpg"),   vis_before)
cv2.imwrite(os.path.join(OUT_DIR, "2_roi_after.jpg"),    vis_after)
cv2.imwrite(os.path.join(OUT_DIR, "3_crop_before.jpg"),  final_before)
cv2.imwrite(os.path.join(OUT_DIR, "4_crop_after.jpg"),   final_after)

# ── 2x2 그리드 ───────────────────────────────────────────────
CELL = 512
def resize_cell(img):
    h, w = img.shape[:2]
    s = min(CELL/w, CELL/h)
    return cv2.resize(img, (int(w*s), int(h*s)), interpolation=cv2.INTER_AREA)

def pad_cell(img):
    h, w = img.shape[:2]
    canvas = np.zeros((CELL, CELL, 3), dtype=np.uint8)
    yo, xo = (CELL-h)//2, (CELL-w)//2
    canvas[yo:yo+h, xo:xo+w] = img
    return canvas

cells = [
    pad_cell(resize_cell(vis_before)),
    pad_cell(resize_cell(vis_after)),
    pad_cell(resize_cell(final_before)),
    pad_cell(resize_cell(final_after)),
]
labels = ["ROI Before (red)", "ROI After (green)", "Crop Before", "Crop After"]

for cell, label in zip(cells, labels):
    cv2.putText(cell, label, (8, CELL-12),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 100), 2)

grid = np.vstack([
    np.hstack(cells[:2]),
    np.hstack(cells[2:]),
])
cv2.imwrite(os.path.join(OUT_DIR, "0_comparison_grid.jpg"), grid)

print(f"\n출력: {os.path.abspath(OUT_DIR)}/")
print("완료!")
