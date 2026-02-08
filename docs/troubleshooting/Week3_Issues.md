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

## 5. 기술 의사결정: GrabCut 제거 (Technical Decision Record)

### 배경 (Context)
- **도메인 분석**: 아동화는 대부분 **흰 종이에 그려진 단순 배경**
- **성능 측정**: GrabCut 처리 시간 **4,233ms** (목표 100ms의 40배 초과)
- **대안 검증**: Canny 에지 검출은 **4ms**로 1000배 빠르며 충분한 품질 제공

### 의사결정 (Decision)
**GrabCut을 실제 전처리 파이프라인에서 제거하기로 결정**

**근거:**
1. **성능**: 4.2초는 실시간 처리 목표(100ms)에 부적합
2. **도메인 적합성**: 단순 배경(흰 종이)에는 Canny 에지 검출로 충분
3. **OpenCV 공식 권장**: 라인아트/스케치 처리는 Canny 사용 (Context7 확인)
4. **오버엔지니어링 회피**: 불필요한 복잡도 제거

### 최종 전략 (Implementation Strategy)

#### ✅ 실제 파이프라인 (Production Code)
```
Preprocess (Letterbox + Denoise + Grayscale)
  ↓
Canny Edge Detection (threshold: 50/150)
  ↓
Morphology Enhancement (MORPH_CLOSE)
  ↓
Adaptive Binarization
```
- **목표 성능**: <50ms
- **GrabCut 제외**: 단순성과 성능 우선

#### 🧪 테스트 코드 (Test Suite)
```cpp
// test_main.cpp에 GrabCut 테스트 유지
TEST_F(AdvancedImageProcessorTest, RemoveBackground_ReturnsNonEmptyMask) {
    cv::Mat mask = processor.RemoveBackground(testImage);
    EXPECT_FALSE(mask.empty());
}
```
- **GrabCut 구현 코드 유지**: `image_processor.cpp::RemoveBackground()`
- **목적**: 기술 역량 증명, 향후 확장 가능성 보존

#### 💼 포트폴리오 어필 포인트
1. **구현 능력**: GrabCut 알고리즘 이해 및 구현 완료
2. **성능 최적화**: 벤치마크 기반 의사결정 (4.2초 → 제거)
3. **도메인 이해**: 아동화 특성 분석을 통한 적절한 기술 선택
4. **엔지니어링 판단**: 오버엔지니어링 회피

### 향후 계획 (Future Considerations)
- **Phase 4 (Python AI)**: 복잡한 배경 이미지 지원 시 Deep Learning 모델 도입
- **선택적 기능**: GrabCut을 옵션으로 제공 가능 (사용자 설정)

---
**작성자**: Antigravity (Senior AI Engineer)  
**최종 업데이트**: 2026-01-31  
**의사결정 참여자**: 사용자 + Antigravity

