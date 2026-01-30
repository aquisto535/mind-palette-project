# Week 3 트러블슈팅: GrabCut 결과 잘림 및 왜곡 이슈

## 1. 문제 현상 (Symptom)
`human.jpg` (236x445) 이미지를 전처리 파이프라인으로 처리할 때, 결과물에서 인물의 **머리(상단)와 발(하단)이 잘려 나가는** 현상 발생.

## 2. 문제 분석 (Root Cause Analysis)

### 원인 1: GrabCut 하드코딩된 마진 (주 원인)
`ImageProcessor::RemoveBackground` 구현 시, 이미지의 가장자리 10%를 무조건 **배경(GC_BGD)**으로 간주하도록 설계됨.
- **로직**: `marginX = cols / 10`, `marginY = rows / 10`을 제외한 중앙 80% 영역만 전경 후보(`GC_PR_FGD`)로 설정.
- **결과**: 인물이 이미지 상단이나 하단에 바짝 붙어 있을 경우, 상/하단 10% 영역(약 51픽셀)에 포함된 머리와 발이 강제로 제거됨.

### 원인 2: 왜곡된 리사이즈 (해결됨)
초기 파이프라인 테스트에서 `cv::resize(original, resized, Size(512, 512))`를 사용하여 세로로 긴 이미지를 정정사각형으로 강하게 늘림(Stretching).
- **해결**: `ResizeKeepingAspectRatio` (Letterbox) 도입으로 기하학적 왜곡은 해결되었으나, '잘림(Cropping)' 문제는 원인 1에 의해 지속됨.

### 원인 3: 패딩 영역 처리 미흡
Letterbox 적용 시 발생하는 검은색 패딩 영역이 GrabCut 알고리즘에 전달될 때, 이 영역이 배경임을 명시적으로 알려주지 않아 알고리즘이 혼선을 빚거나 연산 시간이 불필요하게 증가함.

## 3. GrabCut 성능 이슈 (Performance Issue)
- **현상**: 처리 시간이 1.4초에서 4.2초로 급증.
- **분석**: 512x512 고해상도(GrabCut 기준) 전체 영역에 대해 반복 연산을 수행하면서 발생하는 오버헤드. 실시간성(100ms 목표)에 심각한 저해 요인.

## 4. 해결 전략 (Resolution Strategy)

### 단기적 해결 (C++ Preprocessing)
- [ ] **마스크 설정 유연화**: 상하단 마진을 10%에서 2%로 줄이거나, 사용자가 ROI(관심 영역)를 지정할 수 있도록 인터페이스 확장.
- [ ] **Letterbox 패딩 인지**: 패딩 영역(검은색)을 확실한 배경(`GC_BGD`)으로 선행 설정하여 연산 범위 축소 및 정확도 향상.

### 장기적 해결 (Architecture Choice)
- **Deep Learning 전환**: GrabCut은 기하학적/색상 기반 알고리즘으로 복잡한 배경이나 인물 경계 처리에 한계가 있음. Phase 4에서 Python 기반 AI 모델(MODNet 등)을 도입하여 이 문제를 근본적으로 해결할 예정.

---
**작성자**: Antigravity (Senior AI Engineer)  
**날짜**: 2026-01-29
