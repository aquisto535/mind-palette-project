import numpy as np
import torch
import cv2
import os
from tqdm import tqdm
from glob import glob

def calculate_stats(image_dir: str):
    """3채널 하이브리드 입력용 데이터셋 평균/표준편차 계산.
    
    R: Gray, G: Binary, B: Distance Transform 기준.
    """
    image_paths = glob(os.path.join(image_dir, "*.jpg")) + glob(os.path.join(image_dir, "*.png"))
    
    if not image_paths:
        print("No images found in directory.")
        return

    n_channels = 3
    sum_vals = np.zeros(n_channels)
    sq_sum_vals = np.zeros(n_channels)
    pixel_count = 0

    for path in tqdm(image_paths, desc="Calculating stats"):
        img = cv2.imread(path)
        if img is None: continue
        
        # [0, 1] 범위로 정규화
        img = img.astype(np.float32) / 255.0
        
        # 채널별 합계 누적
        sum_vals += np.sum(img, axis=(0, 1))
        sq_sum_vals += np.sum(np.square(img), axis=(0, 1))
        pixel_count += img.shape[0] * img.shape[1]

    mean = sum_vals / pixel_count
    std = np.sqrt((sq_sum_vals / pixel_count) - np.square(mean))

    print("\n[Dataset Statistics]")
    print(f"Mean: {mean}")
    print(f"Std:  {std}")
    
    return mean, std

import sys

if __name__ == "__main__":
    if len(sys.argv) > 1:
        calculate_stats(sys.argv[1])
    else:
        print("Usage: python calculate_dataset_stats.py <image_dir>")
