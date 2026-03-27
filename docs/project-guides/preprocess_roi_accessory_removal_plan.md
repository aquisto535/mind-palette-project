# GetContentROI 개선 계획 — 부속 자극 제거

> **작성일**: 2026-03-21
> **대상 컴포넌트**: `preprocess-server` C++ 전처리 파이프라인
> **관련 파일**:
> - `preprocess-server/src/filters/hybrid_preprocess_filter.cpp`
> - `preprocess-server/src/core/image_processor.cpp`
> - `preprocess-server/tests/test_filters.cpp`

---

## 문제

아동 인물화(HFD) 이미지를 전처리할 때, `GetContentROI()`가 **모든 유효 컨투어를 무조건 Union**하기 때문에 신체 외부에 그려진 부속 자극(신발, 가방, 나무, 해 등)까지 ROI에 포함되어 출력 이미지에서 제거되지 않는다.

**현재 로직** (`image_processor.cpp:150-182`, `hybrid_preprocess_filter.cpp:84-123`):
```
모든 컨투어 → 면적 0.1% 이상 필터 → 전부 Union → 하나의 큰 ROI
```
부속 자극이 면적 0.1% 이상이면 무조건 ROI에 포함됨.

---

## 해결 전략: Dominant Contour + Spatial Proximity Filtering

### 핵심 아이디어
1. **가장 큰 컨투어**를 메인 인물(Dominant)로 식별
2. 나머지 컨투어는 **메인 인물의 bounding box와의 공간적 근접도**를 기준으로 포함/제외 판정
3. 메인 인물에 근접한 컨투어(신체 분리 선, 팔/다리)는 포함, 멀리 떨어진 부속 자극은 제외

### 알고리즘

```
1. 기존대로 컨투어 검출 + 0.1% 면적 필터링
2. 가장 큰 컨투어 선택 → dominantRect
3. dominantRect를 margin(20%)만큼 확장 → expandedRect
4. 나머지 컨투어 중 expandedRect와 겹치는(overlap) 것만 Union에 포함
5. 겹치지 않는 컨투어 → 제외 (부속 자극)
```

### 왜 이 접근인가

| 비교 | 현재 방식 | Dominant + Proximity |
|------|----------|---------------------|
| 기준 | 면적 0.1% 이상 → 전부 포함 | 가장 큰 컨투어 기준 공간 근접도 |
| 부속 자극 | 포함됨 | 멀리 떨어진 경우 제외 |
| 분절된 팔/다리 | 포함됨 | 근접하므로 포함 |
| 구현 복잡도 | 낮음 | 낮음 (정렬 + rect 비교) |

- **면적만으로는 불충분**: 큰 나무나 집 그림은 인물보다 클 수 있음
- **공간적 근접도가 핵심**: HFD에서 부속 자극은 인물과 **공간적으로 분리**되어 있음
- **인물 분절 대응**: 팔/다리가 몸통과 떨어져 그려진 경우에도 근접하므로 포함됨

---

## 수정 대상 파일

### 1. `preprocess-server/src/filters/hybrid_preprocess_filter.cpp` (핵심)
`GetContentROI()` 메서드 수정 — Dominant contour 식별 + spatial proximity 필터링 로직 교체

**변경 전 핵심 로직**:
```cpp
// 모든 유효 컨투어를 무조건 Union
for (const auto& contour : contours) {
    if (cv::contourArea(contour) < totalArea * 0.001) continue;
    cv::Rect rect = cv::boundingRect(contour);
    if (first) { unionRect = rect; first = false; }
    else { unionRect |= rect; }
}
```

**변경 후 핵심 로직**:
```cpp
// 1. 유효 컨투어 수집 (0.1% 필터)
std::vector<std::pair<double, cv::Rect>> validRects;
for (const auto& contour : contours) {
    double area = cv::contourArea(contour);
    if (area < totalArea * 0.001) continue;
    validRects.push_back({area, cv::boundingRect(contour)});
}
if (validRects.empty()) return cv::Rect(0, 0, binary.cols, binary.rows);

// 2. 가장 큰 컨투어 = Dominant
auto it = std::max_element(validRects.begin(), validRects.end(),
    [](const auto& a, const auto& b){ return a.first < b.first; });
cv::Rect dominantRect = it->second;

// 3. expandedRect = dominantRect + margin 20%
int marginX = static_cast<int>(dominantRect.width  * 0.2);
int marginY = static_cast<int>(dominantRect.height * 0.2);
cv::Rect expandedRect(
    std::max(0, dominantRect.x - marginX),
    std::max(0, dominantRect.y - marginY),
    std::min(binary.cols - dominantRect.x + marginX, dominantRect.width  + 2 * marginX),
    std::min(binary.rows - dominantRect.y + marginY, dominantRect.height + 2 * marginY)
);

// 4. expandedRect와 겹치는 컨투어만 Union
cv::Rect unionRect = dominantRect;
for (const auto& [area, rect] : validRects) {
    cv::Rect intersection = expandedRect & rect;
    if (intersection.area() > 0) {
        unionRect |= rect;
    }
}
```

### 2. `preprocess-server/src/core/image_processor.cpp`
`GetContentROI()` (L150-182) 동일하게 수정 — 두 구현이 동일 로직이므로 동기화 필수

> 중장기적으로는 공통 유틸 함수로 추출하여 중복 제거를 고려한다.

---

## TDD 실행 순서

### Phase 1: RED — 실패 테스트 작성
`preprocess-server/tests/test_filters.cpp`에 새 테스트 추가:

```cpp
// 케이스 1: 메인 인물 + 멀리 떨어진 부속 자극 → 부속 제외
TEST(HybridPreprocessFilterTest, GetContentROI_ExcludesDistantAccessories) {
    // 500x500 흰 이미지에서
    // 중앙(100,100,200,200) = 큰 사각형(메인 인물)
    // 우하단(430,430,50,50) = 작은 사각형(부속 자극, 멀리 분리)
    // 기대: ROI가 부속 자극을 포함하지 않음
}

// 케이스 2: 메인 인물 + 가까운 분리된 선(팔/다리) → 포함
TEST(HybridPreprocessFilterTest, GetContentROI_IncludesNearbyFragments) {
    // 중앙 큰 사각형 + 바로 인접한 작은 사각형
    // 기대: 두 영역 모두 ROI에 포함
}

// 케이스 3: 메인 인물만 → 기존과 동일
TEST(HybridPreprocessFilterTest, GetContentROI_SingleContourUnchanged) {
    // 단일 컨투어
    // 기대: 해당 컨투어의 bounding rect ± padding
}
```

### Phase 2: GREEN — 최소 구현
위 알고리즘을 `GetContentROI()` 두 곳에 적용하여 테스트 통과

### Phase 3: REFACTOR
기존 테스트 (`CreateHybridPipeline` 등) 모두 통과 확인 후 중복 코드 정리

---

## 검증 방법

1. **CTest 실행**: 기존 테스트 + 새 테스트 모두 통과 확인
2. **시각적 검증**: `남자사람_8_남_06463.jpg`에 동일 로직을 Python으로 적용 후 결과 이미지 비교
   - 수정 전 ROI vs 수정 후 ROI 시각화
   - 부속 자극이 제외되었는지 육안 확인
3. **엣지 케이스**: 인물만 있는 단순 이미지에서 기존 동작이 유지되는지 확인

---

## 파라미터 및 리스크

| 항목 | 값 | 비고 |
|------|-----|------|
| `areaThreshold` | `totalArea * 0.001` | 기존 유지 |
| `proximityMargin` | `20%` (width/height 기준) | 튜닝 필요 시 조정 |
| padding | `10px` | 기존 유지 |

**리스크**:
- margin 20%가 너무 작으면 분리된 팔/다리 누락 가능 → 테스트로 검증
- 매우 드물게 배경 그림이 인물보다 클 경우 오작동 가능 → 우선은 허용, 추후 개선
