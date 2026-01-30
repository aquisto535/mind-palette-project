# GrabCut vs Deep Learning 배경 제거 효율성 분석

> **분석 일자**: 2026-01-29  
> **분석 도구**: MCP `sequential-thinking`  
> **결론**: Phase 3 C++ 서버에서는 **GrabCut 채택**

---

## 1. 분해 (Deconstruct) - 근본적 진실

### 배경 제거의 본질
이미지의 각 픽셀을 **전경(Foreground)** 또는 **배경(Background)**으로 분류하는 작업입니다.

### GrabCut 작동 원리
```
초기 마스크 → GMM(가우시안 혼합 모델) 학습 → Graph Cut 에너지 최소화 → 반복 정제
```
- 그래프 이론 기반 최적화 알고리즘
- CPU 연산, OpenCV 네이티브 지원

### Deep Learning 기반 (MODNet, U-Net 등)
```
입력 이미지 → CNN 피처 추출 → 픽셀별 분류 → 마스크 출력
```
- 수백만 파라미터의 학습된 가중치
- GPU 가속 권장

---

## 2. 가정 제거 (Remove Assumptions)

| 일반적 가정 | 실제 검증 |
|------------|----------|
| "DL이 항상 더 정확하다" | ❌ 학습 데이터셋에 의존. 아동화 도메인에서는 일반화 모델 성능 저하 가능 |
| "GrabCut은 느리다" | ❌ `iterCount=1`이면 20-30ms, 충분히 빠름 |
| "배경 제거는 정밀해야 한다" | ❌ 목표는 필압/선 떨림 분석이지, 포토샵 수준 누끼가 아님 |
| "GPU가 필수다" | ❌ ONNX Runtime CPU도 가능하지만 복잡도 증가 |

---

## 3. 정량적 비교 (512x512 이미지 기준)

| 기준 | GrabCut | Deep Learning (MODNet ONNX) |
|------|---------|------------------|
| **처리 속도** | 20-50ms (iter=1~3) | 50-200ms |
| **메모리 사용** | ~10MB | ~200MB+ (모델 로드) |
| **초기화 필요** | ✅ Rect/Mask 필요 | ❌ End-to-end |
| **GPU 필요** | ❌ CPU 전용 | 권장 (CPU 가능) |
| **C++ 통합** | OpenCV 내장 (한 줄) | ONNX Runtime 의존성 추가 |
| **유지보수** | 코드만 | 모델 파일 관리 필요 |

---

## 4. 프로젝트 특수 제약

### 아키텍처 분리 원칙
```
C++ Server = 기하학적 전처리 (빠른 연산)
Python Server = AI 추론 (정밀 분석)
```
→ DL 모델을 C++에 넣으면 역할 경계 위반

### 전처리 목표
- **<100ms** 달성 필요
- GrabCut: 50ms 내 가능 ✅
- DL: 100ms 경계선 (GPU 없으면 초과 위험)

### 아동화 이미지 특성
- 배경: 대부분 **흰색 종이** (단순)
- 전경: 크레용/색연필 그림 (색상 대비 명확)
→ 복잡한 자연 이미지가 아니므로 GrabCut으로 충분

---

## 5. 최종 결론

> **비유**: GrabCut은 "지우개로 배경을 대충 지우는 것", Deep Learning은 "정교한 가위로 오려내는 것"입니다.  
> 아동화 분석에서는 대충 지워도 핵심 특징(필압, 선 떨림)을 추출할 수 있습니다.

### 채택 전략
| Phase | 담당 | 방식 |
|-------|------|------|
| Phase 3 (C++) | 전처리 서버 | **GrabCut** (빠른 초벌 배경 분리) |
| Phase 4 (Python) | AI 서버 | Deep Learning (필요 시 정밀 후처리) |

---

## 참고 자료
- OpenCV GrabCut: https://docs.opencv.org/4.x/d8/d83/tutorial_py_grabcut.html
- MODNet: https://github.com/ZHKKKe/MODNet
