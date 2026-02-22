# Refactoring Strategy: Transition to Filter Pipeline

## 1. 개요 (Overview)
현재 `ImageProcessor::Preprocess`에 구현된 **Hybrid 3-Channel Strategy**는 동작하지만, 특정 로직(Smart ROI, Merge)이 거대한 함수 내에 하드코딩되어 있습니다. 이를 유연한 `FilterPipeline` 아키텍처로 완전히 전환하기 위한 단계별 전략입니다.

## 2. 현황 분석 (Current Status)
- **Active Logic**: `ImageProcessor::Preprocess` (Monolithic, Fast)
- **Architecture**: `FilterPipeline` + `PipelineFactory` + `HybridPreprocessFilter` (Modular, Extensible)
- **Redundancy**: `ImageProcessor`와 `HybridPreprocessFilter`가 동일한 로직을 중복 보유.

## 3. 리팩토링 로드맵 (Roadmap)

### Phase A: 아키텍처 전환 (Facade Pattern 적용)
`ImageProcessor`가 직접 로직을 수행하지 않고, `PipelineFactory`에 위임하도록 변경합니다.
```cpp
// ImageProcessor::Preprocess
FilterPipeline pipeline = PipelineFactory::createHybridPipeline();
return pipeline.execute(input);
```
- **효과**: `ImageProcessor` 코드량 90% 감소. 로직의 단일 진실 공급원(SSOT)을 `src/filters`로 이동.

### Phase B: God Filter 분해 (Decomposition)
현재 `HybridPreprocessFilter`는 너무 많은 일(Threshold, Crop, Resize, Merge)을 하고 있습니다. 이를 단일 책임 원칙(SRP)에 따라 분리합니다.
- `SmartCropFilter`: ROI 계산 및 Crop 전담.
- `AdaptiveBinaryFilter`: Thresholding 및 Morphology 전담.
- `HybridMergeFilter`: 3채널 병합 전담.

**수정된 Pipeline 구성**:
```cpp
pipeline.add(ResizeFilter(1024))
        .add(DenoiseFilter(5))
        .add(AdaptiveBinaryFilter(11, 2))  // Gray -> Binary & Gray (State?)
        .add(SmartCropFilter(0.001))       // Updates ROI
        .add(HybridMergeFilter());         // Uses ROI & Binary/Gray -> Final Mat
```
> *Challenge*: `FilterPipeline`은 현재 `Mat -> Mat` 단일 흐름입니다. `Gray`, `Binary` 두 가지 상태를 유지하려면 `Context` 객체를 전달하는 방식으로 `IFilter::apply` 서명 변경이 필요할 수 있습니다.

## 4. 정리 대상 (Cleanup Candidates)
Hybrid Strategy에 부합하지 않거나 중복되는 레거시 코드를 제거합니다.

- **[DELETE] `src/filters/canny_filter.*`**: Canny Edge Detection은 현재 전략에서 폐기됨. (즉시 삭제)
- **[REVIEW] `src/filters/grayscale_filter.*`**: `AdaptiveBinaryFilter` 내에서 자동 변환되므로 불필요할 수 있음.
- **[REVIEW] `src/filters/binarize_filter.*`**: `AdaptiveBinaryFilter`로 대체 후 삭제.


## 5. 적용된 디자인 패턴 (Design Patterns Applied)
본 리팩토링 전략은 다음의 GoF 패턴을 기반으로 설계되었습니다.

### A. Strategy Pattern (전략 패턴)
- **적용**: `IFilter` 인터페이스와 그 구현체들 (`DenoiseFilter`, `HybridPreprocessFilter` 등)
- **효과**: 알고리즘을 캡슐화하여 런타임에 교체 가능하게 함. 새로운 필터를 추가할 때 기존 코드를 수정하지 않아도 됨 (OCP 준수).
- **Example**: `DenoiseFilter`(Gaussian) 대신 `BilateralFilter`를 끼워 넣어도 파이프라인은 수정될 필요 없음.

### B. Composite Pattern (컴포지트 패턴)
- **적용**: `FilterPipeline` 클래스
- **효과**: 개별 객체(`IFilter`)와 객체들의 조합(`FilterPipeline`)을 동일하게 다룸. 파이프라인 안에 또 다른 파이프라인을 넣는 계층 구조 가능.
- **Example**: `FilterPipeline`도 `IFilter`를 상속받으므로, `MainPipeline` 안에 `SubPipeline`을 `add()` 할 수 있음.

### C. Factory Pattern (팩토리 패턴)
- **적용**: `PipelineFactory` 클래스
- **효과**: 복잡한 객체 생성 로직(필터의 순서, 파라미터 설정 등)을 클라이언트(`ImageProcessor`)로부터 분리.
- **Example**: `ImageProcessor`는 `new ResizeFilter(1024)` 등을 알 필요 없이 `createHybridPipeline()`만 호출하면 됨.

### D. Facade Pattern (퍼사드 패턴) (Phase A 목표)
- **적용**: `ImageProcessor` 클래스
- **효과**: 복잡한 서브시스템(Factory, Pipeline, Filters)을 감싸서 단순한 인터페이스(`Preprocess`)만 제공.
- **Example**: 외부(`Server`)는 내부 파이프라인 구조를 몰라도 `imageProcessor.Preprocess(img)` 한 줄로 기능을 사용 가능.

---
**Note**: 본 문서는 `brain/refactoring_strategy.md`에 저장됩니다.
