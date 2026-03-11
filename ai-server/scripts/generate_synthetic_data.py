import numpy as np
import cv2
import os
import random

def generate_synthetic_sketch(size=512):
    # 흰 배경 생성
    img = np.full((size, size, 3), 255, dtype=np.uint8)
    
    # 랜덤한 선들 그리기 (스케치 모사)
    for _ in range(random.randint(5, 15)):
        pt1 = (random.randint(0, size), random.randint(0, size))
        pt2 = (random.randint(0, size), random.randint(0, size))
        color = (random.randint(0, 50), random.randint(0, 50), random.randint(0, 50))
        thickness = random.randint(1, 3)
        cv2.line(img, pt1, pt2, color, thickness)
        
    # 하이브리드 3채널 모사
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, binary = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY_INV)
    dist = cv2.distanceTransform(binary, cv2.DIST_L2, 3)
    dist = cv2.normalize(dist, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)
    
    # 채널 재조합 (R=Gray, G=Binary, B=Dist)
    hybrid = cv2.merge([gray, binary, dist])
    return hybrid

def main(output_dir="data/synthetic_sketches", count=20):
    os.makedirs(output_dir, exist_ok=True)
    for i in range(count):
        sketch = generate_synthetic_sketch()
        cv2.imwrite(os.path.join(output_dir, f"sketch_{i:03d}.png"), sketch)
    print(f"Generated {count} synthetic sketches in {output_dir}")

if __name__ == "__main__":
    main()
