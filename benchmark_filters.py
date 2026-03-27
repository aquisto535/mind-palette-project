"""필터 파라미터 벤치마크 및 시각화 스크립트.

각 필터의 3가지 파라미터 후보값을 실제 이미지에 적용하고
PSNR/SSIM을 측정한 뒤 결과 이미지를 저장합니다.
"""
import sys
sys.path.insert(0, ".")

import cv2
import numpy as np
import os

# ── 출력 디렉토리 ──────────────────────────────────────────
OUT_DIR = "benchmark_output"
os.makedirs(OUT_DIR, exist_ok=True)

IMG_PATH = "남자사람_8_남_06463.jpg"
# Windows에서 한글 파일명을 올바르게 읽기 위해 imdecode 사용
import pathlib
with open(IMG_PATH, 'rb') as f:
    img_bytes = f.read()
img_orig = cv2.imdecode(np.frombuffer(img_bytes, np.uint8), cv2.IMREAD_COLOR)
assert img_orig is not None, f"이미지를 읽을 수 없습니다: {IMG_PATH}"

# 처리 속도를 위해 512x512로 리사이즈
img = cv2.resize(img_orig, (512, 512), interpolation=cv2.INTER_AREA)
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# ── PSNR/SSIM 함수 ─────────────────────────────────────────
def compute_psnr(orig, proc):
    if orig.shape != proc.shape:
        proc = cv2.resize(proc, (orig.shape[1], orig.shape[0]))
    if len(proc.shape) != len(orig.shape):
        if len(orig.shape) == 2:
            proc = cv2.cvtColor(proc, cv2.COLOR_BGR2GRAY) if len(proc.shape) == 3 else proc
        else:
            proc = cv2.cvtColor(proc, cv2.COLOR_GRAY2BGR)
    return cv2.PSNR(orig.astype(np.float64), proc.astype(np.float64))

def compute_ssim(orig, proc):
    if orig.shape != proc.shape:
        proc = cv2.resize(proc, (orig.shape[1], orig.shape[0]))
    if len(proc.shape) != len(orig.shape):
        if len(orig.shape) == 2:
            proc = cv2.cvtColor(proc, cv2.COLOR_BGR2GRAY) if len(proc.shape) == 3 else proc
        else:
            proc = cv2.cvtColor(proc, cv2.COLOR_GRAY2BGR)
    i1 = orig.astype(np.float64)
    i2 = proc.astype(np.float64)
    C1, C2 = 6.5025, 58.5225
    mu1 = cv2.GaussianBlur(i1, (11,11), 1.5)
    mu2 = cv2.GaussianBlur(i2, (11,11), 1.5)
    mu1_sq, mu2_sq, mu1_mu2 = mu1*mu1, mu2*mu2, mu1*mu2
    s1 = cv2.GaussianBlur(i1*i1,(11,11),1.5) - mu1_sq
    s2 = cv2.GaussianBlur(i2*i2,(11,11),1.5) - mu2_sq
    s12= cv2.GaussianBlur(i1*i2,(11,11),1.5) - mu1_mu2
    num = (2*mu1_mu2+C1)*(2*s12+C2)
    den = (mu1_sq+mu2_sq+C1)*(s1+s2+C2)
    ssim_map = num / den
    if ssim_map.ndim == 3:
        return float(ssim_map.mean())
    return float(cv2.mean(ssim_map)[0])

# ── 결과 저장용 ────────────────────────────────────────────
results = {}

def save_and_measure(orig_for_metric, processed, filename):
    cv2.imwrite(os.path.join(OUT_DIR, filename), processed)
    # 동일 채널 수로 맞춰서 측정
    psnr = compute_psnr(orig_for_metric, processed)
    ssim = compute_ssim(orig_for_metric, processed)
    return psnr, ssim

print("=" * 60)
print("필터 파라미터 벤치마크")
print("=" * 60)

# ── 1. BinarizeFilter — blockSize ─────────────────────────
print("\n[1] BinarizeFilter — blockSize (C=2 고정)")
results["BinarizeFilter_blockSize"] = []
for bs in [7, 11, 15]:
    out = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                                cv2.THRESH_BINARY, bs, 2)
    psnr, ssim = save_and_measure(gray, out, f"1_binarize_blockSize{bs}.png")
    results["BinarizeFilter_blockSize"].append((bs, psnr, ssim))
    print(f"  blockSize={bs:2d} → PSNR={psnr:.1f}dB  SSIM={ssim:.4f}")

# ── 2. BinarizeFilter — C ──────────────────────────────────
print("\n[2] BinarizeFilter — C (blockSize=11 고정)")
results["BinarizeFilter_C"] = []
for c in [1, 2, 3]:
    out = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                                cv2.THRESH_BINARY, 11, c)
    psnr, ssim = save_and_measure(gray, out, f"2_binarize_C{c}.png")
    results["BinarizeFilter_C"].append((c, psnr, ssim))
    print(f"  C={c} → PSNR={psnr:.1f}dB  SSIM={ssim:.4f}")

# ── 3. DenoiseFilter — gaussianSize ───────────────────────
print("\n[3] DenoiseFilter — gaussianSize")
results["DenoiseFilter_gaussianSize"] = []
for ks in [3, 5, 7]:
    out = cv2.GaussianBlur(gray, (ks, ks), 0)
    psnr, ssim = save_and_measure(gray, out, f"3_denoise_gaussianSize{ks}.png")
    results["DenoiseFilter_gaussianSize"].append((ks, psnr, ssim))
    print(f"  gaussianSize={ks} → PSNR={psnr:.1f}dB  SSIM={ssim:.4f}")

# ── 4. ClaheFilter — clipLimit ────────────────────────────
print("\n[4] ClaheFilter — clipLimit (tileGridSize=8 고정)")
results["ClaheFilter_clipLimit"] = []
for cl in [1.0, 2.0, 4.0]:
    clahe = cv2.createCLAHE(clipLimit=cl, tileGridSize=(8,8))
    out = clahe.apply(gray)
    psnr, ssim = save_and_measure(gray, out, f"4_clahe_clipLimit{cl}.png")
    results["ClaheFilter_clipLimit"].append((cl, psnr, ssim))
    print(f"  clipLimit={cl} → PSNR={psnr:.1f}dB  SSIM={ssim:.4f}")

# ── 5. NlMeansDenoiseFilter — h ───────────────────────────
print("\n[5] NlMeansDenoiseFilter — h")
results["NlMeansDenoiseFilter_h"] = []
for h in [5, 10, 15]:
    out = cv2.fastNlMeansDenoising(gray, None, h=h, templateWindowSize=7, searchWindowSize=21)
    psnr, ssim = save_and_measure(gray, out, f"5_nlmeans_h{h}.png")
    results["NlMeansDenoiseFilter_h"].append((h, psnr, ssim))
    print(f"  h={h:2d} → PSNR={psnr:.1f}dB  SSIM={ssim:.4f}")

# ── 6. OtsuCannyFilter — sigma ────────────────────────────
print("\n[6] OtsuCannyFilter — sigma")
results["OtsuCannyFilter_sigma"] = []
_, dummy = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)
otsu_val = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)[0]
print(f"  Otsu threshold = {otsu_val:.1f}")
for sigma in [0.33, 0.5, 0.66]:
    low  = (1.0 - sigma) * otsu_val
    high = (1.0 + sigma) * otsu_val
    out  = cv2.Canny(gray, low, high)
    edge_count = int(np.count_nonzero(out))
    cv2.imwrite(os.path.join(OUT_DIR, f"6_canny_sigma{sigma}.png"), out)
    results["OtsuCannyFilter_sigma"].append((sigma, edge_count))
    print(f"  sigma={sigma} (low={low:.1f}, high={high:.1f}) → 엣지픽셀={edge_count:,}")

# ── 원본 저장 ──────────────────────────────────────────────
cv2.imwrite(os.path.join(OUT_DIR, "0_original_512.png"), img)
cv2.imwrite(os.path.join(OUT_DIR, "0_original_gray.png"), gray)

# ── ADR 업데이트용 수치 출력 ───────────────────────────────
print("\n" + "=" * 60)
print("ADR 업데이트용 측정값")
print("=" * 60)

filters = [
    ("BinarizeFilter_blockSize", "blockSize", "값"),
    ("BinarizeFilter_C", "C", "값"),
    ("DenoiseFilter_gaussianSize", "gaussianSize", "값"),
    ("ClaheFilter_clipLimit", "clipLimit", "값"),
    ("NlMeansDenoiseFilter_h", "h", "값"),
]
for key, param, unit in filters:
    print(f"\n### {key.split('_')[0]} — {param}")
    print("| 값 | PSNR (dB) | SSIM |")
    print("|-----|-----------|------|")
    for row in results[key]:
        val, psnr, ssim = row
        print(f"| {val} | {psnr:.1f} | {ssim:.4f} |")

print("\n### OtsuCannyFilter — sigma")
print("| sigma | 엣지 픽셀 수 |")
print("|-------|-------------|")
for sigma, cnt in results["OtsuCannyFilter_sigma"]:
    print(f"| {sigma} | {cnt:,} |")

print(f"\n출력 디렉토리: {os.path.abspath(OUT_DIR)}")
print("완료!")
