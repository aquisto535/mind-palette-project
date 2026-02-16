# Preprocess Server 모듈 기술 보고서

## C++ 이미지 전처리 서버 — 설계 및 구현 상세 보고서

---

**프로젝트명:** Mind Palette  
**모듈명:** Preprocess Server  
**기술 스택:** C++17 · CMake · Crow · OpenCV · spdlog · vcpkg · Google Test  
**작성일:** 2026-02-14  
**문서 유형:** 기술 보고서 (논문 형태)

---

## 목차

1. [서론](#1-서론)
2. [시스템 개요 및 요구사항](#2-시스템-개요-및-요구사항)
3. [기술 스택 선정 근거](#3-기술-스택-선정-근거)
4. [프로젝트 구조 및 아키텍처](#4-프로젝트-구조-및-아키텍처)
5. [빌드 시스템 설계](#5-빌드-시스템-설계)
6. [코어 모듈 상세 분석](#6-코어-모듈-상세-분석)
7. [필터 시스템 설계 (디자인 패턴)](#7-필터-시스템-설계-디자인-패턴)
8. [인프라 모듈 (동시성 및 성능)](#8-인프라-모듈-동시성-및-성능)
9. [유틸리티 모듈](#9-유틸리티-모듈)
10. [HTTP 서버 및 API 설계](#10-http-서버-및-api-설계)
11. [이미지 처리 파이프라인](#11-이미지-처리-파이프라인)
12. [테스트 전략](#12-테스트-전략)
13. [확장성 및 향후 개선 방향](#13-확장성-및-향후-개선-방향)
14. [결론](#14-결론)

---

## 1. 서론

### 1.1 배경 및 목적

Preprocess Server는 Mind Palette 프로젝트의 **C++ 기반 이미지 전처리 서버**이다. 아동이 그린 인물화(사람 그림) 이미지를 수신하여, AI 모델(EfficientNet-B2)의 입력에 적합한 형태로 전처리하는 역할을 담당한다.

본 서버의 핵심 목적은 다음과 같다:

1. **이미지 정규화:** 다양한 크기·비율의 원본 이미지를 512×512 고정 크기로 변환
2. **노이즈 제거:** 촬영 환경의 노이즈를 제거하여 분석 정확도 향상
3. **특징 추출 보조:** Canny 에지 검출, 이진화 등을 통해 그림의 윤곽선 강조
4. **포맷 변환:** 단일 채널(Grayscale) 결과를 3채널 RGB로 변환하여 AI 모델 호환성 확보

### 1.2 C++ 선정 이유

이미지 전처리라는 **계산 집약적(Compute-Intensive)** 작업을 위해 다음의 이유로 C++을 선택하였다:

- **네이티브 OpenCV 지원:** OpenCV의 모든 기능을 오버헤드 없이 직접 활용
- **메모리 제어:** `cv::Mat`의 참조 카운팅과 직접 메모리 관리로 대용량 이미지 처리 시 메모리 효율 극대화
- **멀티스레딩:** `std::thread`, `std::mutex`, `std::condition_variable`을 통한 저수준 동시성 제어
- **저지연 HTTP 서버:** Crow 프레임워크의 경량 C++ HTTP 서버로 마이크로초 단위의 요청 처리

### 1.3 개발 진행 단계

본 서버는 **점진적 개발(Incremental Development)** 방식으로 다음 단계를 거쳐 구현되었다:

| 단계 | 내용 | 핵심 결과물 |
|---|---|---|
| Week 1 | 프로젝트 초기화, Crow 서버 헬스 체크 | `main.cpp`, `server.h` |
| Week 2 | 기본 이미지 전처리 (Resize, Denoise, Grayscale) | `ImageProcessor` 클래스 |
| Week 3 | 고급 전처리 (GrabCut, Canny, Morphology, Binarize) + 디자인 패턴 리팩터링 | `IFilter`, `FilterPipeline`, `PipelineFactory` |
| Week 4 | 동시성 인프라 (ThreadPool, AtomicWriter, Benchmark) | `infra/` 모듈 |

---

## 2. 시스템 개요 및 요구사항

### 2.1 기능적 요구사항

| ID | 설명 | 상태 |
|---|---|---|
| FR-01 | HTTP POST로 이미지 경로를 수신하여 전처리 수행 | ✅ 완료 |
| FR-02 | Letterbox 리사이즈 (종횡비 유지, 512×512) | ✅ 완료 |
| FR-03 | 가우시안 + 미디언 블러 노이즈 제거 | ✅ 완료 |
| FR-04 | Canny 에지 검출 | ✅ 완료 |
| FR-05 | 적응적 이진화 (Adaptive Thresholding) | ✅ 완료 |
| FR-06 | 모폴로지 연산 (MORPH_CLOSE) | ✅ 완료 |
| FR-07 | GrabCut 배경 제거 | ✅ 완료 (성능 이유로 프로덕션 파이프라인에서 제외) |
| FR-08 | 결과 이미지를 3채널 RGB로 변환 | ✅ 완료 |
| FR-09 | 처리된 이미지를 파일로 저장 | ✅ 완료 |
| FR-10 | 헬스 체크 엔드포인트 | ✅ 완료 |

### 2.2 비기능적 요구사항

| ID | 설명 | 기준 |
|---|---|---|
| NFR-01 | 단일 이미지 처리 시간 | < 500ms |
| NFR-02 | 멀티스레드 동시 처리 | CPU 코어 수에 비례하는 스케일링 |
| NFR-03 | 파일 쓰기 안전성 | 원자적 파일 쓰기 (크래시 시 데이터 무결성 보장) |
| NFR-04 | 크로스 플랫폼 | Windows (MSVC) + Linux (GCC/Clang) |

---

## 3. 기술 스택 선정 근거

### 3.1 C++17 표준

**선정 이유 및 활용 기능:**

| C++17 기능 | 적용 위치 | 목적 |
|---|---|---|
| `std::filesystem` | `server.h`, `atomic_writer.cpp` | 플랫폼 독립적 파일/경로 조작 |
| `std::optional` | `server.h`, `task_queue.h` | 실패 가능한 반환값의 명시적 표현 |
| `auto` 타입 추론 | 전체 | 코드 간결성 |
| Structured Binding | 제한적 사용 | 다중 반환값 분해 |
| Nested Namespace | 미사용 (향후 적용 권장) | 네임스페이스 간결화 |

### 3.2 OpenCV

**용도:** 이미지 로딩, 리사이즈, 블러, 색공간 변환, 에지 검출, 이진화, GrabCut 등 전처리 전반

**vcpkg.json 의존성 설정:**
```json
{
  "name": "opencv",
  "default-features": false,
  "features": ["dnn", "jpeg", "png", "tiff", "webp", "thread"]
}
```

- `default-features: false`: 불필요한 모듈(GUI, Video 등) 제외로 빌드 시간 및 바이너리 크기 최소화
- `dnn`: 향후 딥러닝 추론 통합 대비
- `jpeg`, `png`, `tiff`, `webp`: 다양한 이미지 포맷 지원
- `thread`: 멀티스레드 이미지 처리

### 3.3 Crow (HTTP 서버)

**선정 이유:**

- **헤더 전용(Header-Only):** 별도 빌드 불필요, `#include "crow.h"`만으로 사용
- **Express.js 유사 API:** 라우트 정의가 직관적 (`CROW_ROUTE(app, "/path")`)
- **멀티스레딩:** `.multithreaded()`로 OS의 논리 코어 수에 맞는 스레드 풀 자동 생성
- **JSON 내장:** `crow::json::load()`, `crow::json::wvalue`로 JSON 파싱/생성

### 3.4 spdlog

**선정 이유:**

- **고성능:** 비동기 로깅, 포맷 문자열 컴파일 타임 검증
- **다중 싱크:** 콘솔(색상) + 파일(로테이팅) 동시 출력
- **포맷 유연성:** `[날짜시간] [로거명] [레벨] 메시지` 형식 커스터마이징

### 3.5 Google Test

**용도:** 단위 테스트 프레임워크  
**CMake 통합:** `gtest_discover_tests()`로 CTest와 자동 연동

### 3.6 vcpkg (패키지 매니저)

**매니페스트 모드:** `vcpkg.json`에 의존성을 선언하면 빌드 시 자동 설치. Visual Studio에서 폴더 열기만으로 모든 의존성이 해결됨.

**재현성 확보:** `vcpkg-configuration.json`으로 레지스트리 baseline(커밋 해시)을 고정하여 어느 환경에서든 동일한 의존성 버전 보장.

---

## 4. 프로젝트 구조 및 아키텍처

### 4.1 디렉토리 구조

```
preprocess-server/
├── CMakeLists.txt              # 빌드 시스템 정의 (112라인)
├── CMakeSettings.json          # Visual Studio CMake 설정
├── vcpkg.json                  # vcpkg 의존성 매니페스트
├── vcpkg-configuration.json    # vcpkg 레지스트리 baseline 고정
├── README.md                   # 빌드 가이드
├── verify_linux.sh             # Linux 빌드 검증 스크립트
├── src/
│   ├── main.cpp                # 서버 진입점 (13라인)
│   ├── core/                   # 핵심 비즈니스 로직
│   │   ├── server.h            # HTTP 라우트 정의 + 요청 처리 (178라인)
│   │   ├── image_processor.h   # 이미지 처리 클래스 선언 (52라인)
│   │   ├── image_processor.cpp # 이미지 처리 구현 (177라인)
│   │   ├── filter.h            # IFilter 인터페이스 (35라인)
│   │   ├── filter_pipeline.h   # FilterPipeline 클래스 (57라인)
│   │   ├── filter_pipeline.cpp # 파이프라인 실행 로직 (34라인)
│   │   ├── pipeline_factory.h  # PipelineFactory 클래스 (40라인)
│   │   └── pipeline_factory.cpp # 사전 정의 파이프라인 (36라인)
│   ├── filters/                # 개별 필터 구현체 (8개)
│   │   ├── resize_filter.h/cpp
│   │   ├── denoise_filter.h/cpp
│   │   ├── grayscale_filter.h/cpp
│   │   ├── canny_filter.h/cpp
│   │   ├── binarize_filter.h/cpp
│   │   ├── morphology_filter.h/cpp
│   │   ├── invert_filter.h/cpp
│   │   └── rgb_convert_filter.h/cpp
│   ├── infra/                  # 인프라 (동시성, 성능)
│   │   ├── thread_pool.h/cpp   # 스레드 풀 (Producer-Consumer)
│   │   ├── task_queue.h        # 동기/비동기 태스크 큐
│   │   ├── atomic_writer.h/cpp # 원자적 파일 쓰기
│   │   └── benchmark.h/cpp     # 성능 측정 유틸리티
│   └── utils/                  # 유틸리티
│       ├── Logger.h            # spdlog 래퍼
│       └── Logger.cpp          # 로거 초기화
├── tests/                      # Google Test 단위 테스트
│   ├── test_main.cpp           # 통합 테스트 (서버 + 이미지 처리)
│   ├── test_filters.cpp        # 필터 단위 테스트
│   ├── test_thread_pool.cpp    # 스레드 풀 테스트
│   └── test_atomic_writer.cpp  # 원자적 쓰기 테스트
└── examples/                   # 수동 테스트 및 벤치마크
    ├── test_week3.cpp
    ├── test_pipeline.cpp
    ├── test_canny_pipeline.cpp
    ├── test_filter_pipeline.cpp
    ├── test_benchmark.cpp
    ├── test_atomic_integration.cpp
    └── test_server_jpg.cpp
```

### 4.2 계층 아키텍처

```
┌─────────────────────────────────────────────────┐
│            HTTP Layer (Crow)                     │
│  server.h: 라우트 정의, 요청 검증, 응답 생성     │
├─────────────────────────────────────────────────┤
│            Core Layer (비즈니스 로직)              │
│  ┌──────────────────────────────────────────┐   │
│  │ ImageProcessor (프로덕션 서버 사용)       │   │
│  │ 직접 메서드 호출 방식                     │   │
│  │ Preprocess→Canny→Morphology→Binarize→RGB │   │
│  └──────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────┐   │
│  │ FilterPipeline + PipelineFactory          │   │
│  │ (디자인 패턴 학습 및 테스트/벤치마크 전용) │   │
│  └──────────────────────────────────────────┘   │
├─────────────────────────────────────────────────┤
│            Filters Layer (Strategy 패턴)         │
│  Resize | Denoise | Grayscale | Canny |         │
│  Binarize | Morphology | Invert | RgbConvert    │
├─────────────────────────────────────────────────┤
│            Infra Layer (인프라)                   │
│  ThreadPool | TaskQueue | AtomicWriter |         │
│  Benchmark                                       │
├─────────────────────────────────────────────────┤
│            Utils Layer                           │
│  Logger (spdlog 래퍼)                            │
├─────────────────────────────────────────────────┤
│            External Libraries                    │
│  OpenCV | Crow | spdlog | ASIO | GTest          │
└─────────────────────────────────────────────────┘
```

---

## 5. 빌드 시스템 설계

### 5.1 CMake 설정 분석

**최소 버전:** CMake 3.15  
**C++ 표준:** C++17 (`CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_STANDARD_REQUIRED ON`)

**Windows 정적 링크 전략:**

```cmake
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
endif()
```

- **정적 링크 선택 이유:** OpenCV의 DLL 의존성 문제를 근본적으로 해결. 배포 시 단일 실행 파일로 모든 의존성 포함
- **CRT 정적 링크:** `/MT`(Release) 또는 `/MTd`(Debug)로 C 런타임도 정적 링크

### 5.2 빌드 타겟 구성

| 타겟 | 유형 | 목적 |
|---|---|---|
| `preprocess_server` | 실행 파일 | 프로덕션 서버 |
| `unit_tests` | 실행 파일 | GTest 기반 단위 테스트 |
| `test_week3` | 실행 파일 | Week 3 수동 테스트 |
| `test_pipeline` | 실행 파일 | GrabCut 포함 파이프라인 테스트 |
| `test_canny_pipeline` | 실행 파일 | 프로덕션 파이프라인 테스트 |
| `test_filter_pipeline` | 실행 파일 | 디자인 패턴 파이프라인 테스트 |
| `test_benchmark` | 실행 파일 | 성능 벤치마크 |
| `test_atomic_integration` | 실행 파일 | 원자적 쓰기 통합 테스트 |
| `test_server_jpg` | 실행 파일 | JPG 서버 처리 테스트 |

---

## 6. 코어 모듈 상세 분석

### 6.1 ImageProcessor 클래스

**파일:** `src/core/image_processor.h/cpp`  
**역할:** 프로덕션 서버의 핵심 이미지 처리 클래스. `server.h`에서 직접 메서드를 호출하여 전처리 파이프라인을 구성한다. Week 2에서 기초 기능을 구현하고 Week 3에서 고급 전처리 메서드를 추가하였으며, 현재 서버의 프로덕션 파이프라인은 이 클래스의 메서드를 순차 호출하는 방식으로 동작한다.

**공개 인터페이스:**

```cpp
class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor() = default;

    cv::Mat Load(const std::string& path);
    cv::Mat Preprocess(const cv::Mat& input);            // Resize→Denoise→Grayscale
    bool Save(const cv::Mat& image, const std::string& path);

    // Week 3: 고급 전처리
    cv::Mat RemoveBackground(const cv::Mat& input, int iterCount = 3);
    cv::Mat DetectEdges(const cv::Mat& grayscale, double low = 50, double high = 150);
    cv::Mat EnhanceContours(const cv::Mat& binary, int kernelSize = 3);
    cv::Mat Binarize(const cv::Mat& grayscale);
    cv::Mat ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize = 512);

private:
    const int kTargetSize = 512;
};
```

### 6.2 Preprocess 파이프라인 상세

```cpp
cv::Mat ImageProcessor::Preprocess(const cv::Mat& input) {
    cv::Mat processed;
    input.copyTo(processed);

    // Step 1: Letterbox Resize (종횡비 유지 512×512)
    processed = ResizeKeepingAspectRatio(processed, kTargetSize);

    // Step 2: 이중 노이즈 제거
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);  // 고주파 노이즈
    cv::medianBlur(processed, processed, 3);                     // 소금-후추 노이즈

    // Step 3: 그레이스케일 변환
    cv::Mat grayscale;
    cv::cvtColor(processed, grayscale, cv::COLOR_BGR2GRAY);

    return grayscale;
}
```

### 6.3 Letterbox Resize 알고리즘

```cpp
cv::Mat ImageProcessor::ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    // 종횡비를 유지하는 스케일 팩터 계산
    double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );

    int newWidth = static_cast<int>(input.cols * scale);
    int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight));

    // 검정 패딩이 있는 512×512 캔버스 생성
    cv::Mat canvas(targetSize, targetSize, input.type(), cv::Scalar(0, 0, 0));

    // 중앙 배치
    int offsetX = (targetSize - newWidth) / 2;
    int offsetY = (targetSize - newHeight) / 2;
    resized.copyTo(canvas(cv::Rect(offsetX, offsetY, newWidth, newHeight)));

    return canvas;
}
```

**Letterbox 방식 선택 이유:**

- **종횡비 왜곡 방지:** 단순 리사이즈는 그림의 비율을 변형시켜 AI 모델의 분석 정확도에 영향
- **YOLO/EfficientNet 호환:** 객체 탐지·분류 모델에서 표준으로 사용하는 전처리 방식
- **검정 패딩:** 빈 영역을 0(검정)으로 채워 모델이 패딩과 실제 이미지를 구분 가능

### 6.4 GrabCut 배경 제거

```cpp
cv::Mat ImageProcessor::RemoveBackground(const cv::Mat& input, int iterCount) {
    // 마스크 초기화: 가장자리 10%를 배경 후보, 중앙을 전경 후보로 설정
    cv::Mat mask = cv::Mat::zeros(input.size(), CV_8UC1);
    int marginX = input.cols / 10;
    int marginY = input.rows / 10;

    cv::Rect foregroundRect(marginX, marginY,
                            input.cols - 2 * marginX,
                            input.rows - 2 * marginY);
    mask(foregroundRect).setTo(cv::GC_PR_FGD);

    cv::Mat bgdModel, fgdModel;
    cv::grabCut(input, mask, cv::Rect(), bgdModel, fgdModel,
                iterCount, cv::GC_INIT_WITH_MASK);

    // GC_FGD(1) + GC_PR_FGD(3) → 255(전경)
    cv::Mat result;
    cv::compare(mask, cv::GC_PR_FGD, result, cv::CMP_EQ);
    cv::Mat fgdMask;
    cv::compare(mask, cv::GC_FGD, fgdMask, cv::CMP_EQ);
    result = result | fgdMask;

    return result;
}
```

**프로덕션 제외 이유:**

- `iterCount=3` 기준 40~60ms 소요 — 다른 필터 대비 10배 이상 느림
- 종이 위 그림의 경우 배경 제거 효과가 불분명
- Canny + Binarize 조합이 GrabCut 없이도 충분한 윤곽선 추출 가능

---

## 7. 필터 시스템 설계 (디자인 패턴)

### 7.1 아키텍처 개요

Week 3에서 기존 `ImageProcessor`의 직접 메서드 호출 방식을 **3가지 디자인 패턴**으로 리팩터링하였다:

1. **Strategy Pattern:** `IFilter` 인터페이스로 각 필터를 교체 가능한 알고리즘으로 캡슐화
2. **Composite Pattern:** `FilterPipeline`으로 여러 필터를 체인으로 연결
3. **Factory Pattern:** `PipelineFactory`로 사전 정의된 파이프라인 생성

### 7.2 IFilter 인터페이스 (Strategy Pattern)

```cpp
class IFilter {
public:
    virtual ~IFilter() = default;
    virtual cv::Mat apply(const cv::Mat& input) const = 0;
    virtual std::string name() const = 0;
};

using FilterPtr = std::unique_ptr<IFilter>;
```

**설계 원칙:**

- **OCP(개방-폐쇄 원칙):** 새 필터 추가 시 기존 코드 수정 불필요. `IFilter`를 상속하는 새 클래스만 추가
- **순수 가상 함수:** `apply()`와 `name()` 모두 반드시 구현 강제
- **const 참조 입력:** `const cv::Mat&`로 원본 이미지 변경 방지
- **스마트 포인터:** `std::unique_ptr<IFilter>`로 소유권 명확화 및 메모리 자동 관리

### 7.3 구현된 8개 필터

| 필터 | 클래스명 | 기능 | 파라미터 |
|---|---|---|---|
| 리사이즈 | `ResizeFilter` | Letterbox 리사이즈 | `targetSize=512` |
| 노이즈 제거 | `DenoiseFilter` | Gaussian + Median 블러 | `gaussianSize=5`, `medianSize=3` |
| 그레이스케일 | `GrayscaleFilter` | BGR→Gray 변환 | 없음 |
| Canny 에지 | `CannyFilter` | Canny 에지 검출 | `lowThreshold=50`, `highThreshold=150` |
| 이진화 | `BinarizeFilter` | 적응적 임계값 이진화 | `blockSize=11`, `c=2` |
| 모폴로지 | `MorphologyFilter` | 형태학적 연산 (CLOSE) | `kernelSize=3`, `operation=MORPH_CLOSE` |
| 반전 | `InvertFilter` | 색상 반전 (`bitwise_not`) | 없음 |
| RGB 변환 | `RgbConvertFilter` | Gray→BGR 3채널 변환 | 없음 |

### 7.4 FilterPipeline (Composite Pattern)

```cpp
class FilterPipeline {
public:
    FilterPipeline& add(FilterPtr filter);   // 체이닝 지원
    cv::Mat execute(const cv::Mat& input) const;
    void clear();
    size_t size() const;
    bool empty() const;

private:
    std::vector<FilterPtr> filters_;
};
```

**핵심 설계:**

- **복사 금지, 이동 허용:** `unique_ptr` 시맨틱에 따라 복사 생성자/대입 연산자 삭제, 이동 시맨틱 기본 활성화
- **메서드 체이닝:** `add()`가 `*this` 참조를 반환하여 `pipeline.add(a).add(b).add(c)` 패턴 지원
- **실패 전파:** 중간 필터가 빈 결과를 반환하면 즉시 에러 로그 출력 후 빈 `cv::Mat` 반환

### 7.5 PipelineFactory (Factory Pattern)

```cpp
class PipelineFactory {
public:
    static FilterPipeline createPreprocessPipeline(int targetSize = 512);
    // Resize → Denoise → Grayscale

    static FilterPipeline createSketchPipeline(int targetSize = 512);
    // Resize → Denoise → Grayscale → Canny → Morphology → Invert → RGB

    static FilterPipeline createEdgeDetectionPipeline();
    // Grayscale → Canny
};
```

**Sketch Pipeline (테스트/벤치마크 전용) 상세:**

> **참고:** 현재 프로덕션 서버(`server.h`)는 `PipelineFactory`를 사용하지 않고 `ImageProcessor`의 메서드를 직접 호출한다. `PipelineFactory`는 테스트, 벤치마크, 예제 코드에서 활용되며, 향후 동적 파이프라인 구성이 필요할 때 서버에 도입될 수 있다.

```
입력 이미지 (BGR, 가변 크기)
    │
    ▼  ResizeFilter(512)
512×512 BGR (Letterbox, 검정 패딩)
    │
    ▼  DenoiseFilter(5, 3)
512×512 BGR (노이즈 감소)
    │
    ▼  GrayscaleFilter
512×512 Gray (단일 채널)
    │
    ▼  CannyFilter(50, 150)
512×512 Binary (에지 = 255, 배경 = 0)
    │
    ▼  MorphologyFilter(3, MORPH_CLOSE)
512×512 Binary (에지 간극 메움)
    │
    ▼  InvertFilter
512×512 Binary (배경 = 255, 에지 = 0, 흰 종이 위 검은 선)
    │
    ▼  RgbConvertFilter
512×512 BGR 3채널 (EfficientNet-B2 호환)
    │
    ▼  출력
```

---

## 8. 인프라 모듈 (동시성 및 성능)

### 8.1 ThreadPool (Producer-Consumer)

**파일:** `src/infra/thread_pool.h/cpp`

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 0);  // 0 = hardware_concurrency
    ~ThreadPool();

    template<class F>
    void enqueue(F&& task);   // 유니버설 레퍼런스 + 완벽 전달

    size_t size() const;
    void shutdown();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> shutdown_{false};
};
```

**동작 원리:**

1. 생성자에서 `numThreads`개의 워커 스레드 생성
2. 각 워커는 무한 루프에서 `condition_variable`로 대기
3. `enqueue()` 호출 시 태스크를 큐에 추가하고 `cv_.notify_one()`
4. 워커가 깨어나서 큐에서 태스크를 꺼내 실행
5. `shutdown()` 시 `stop_` 플래그 설정 후 모든 워커 `join`

**스레드 안전성:**

- `std::lock_guard` / `std::unique_lock`으로 큐 접근 보호
- `std::atomic<bool>`으로 플래그 원자적 갱신
- `cv_.wait(lock, predicate)`로 허위 깨우기(Spurious Wakeup) 방지

### 8.2 TaskQueue (동기/비동기 이중 구현)

**파일:** `src/infra/task_queue.h`

```cpp
template<typename T>
class ITaskQueue {                    // 추상 인터페이스
    virtual void push(T task) = 0;
    virtual std::optional<T> pop() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

template<typename T>
class SyncTaskQueue : public ITaskQueue<T> { ... };   // 단일 스레드 (테스트용)

template<typename T>
class AsyncTaskQueue : public ITaskQueue<T> { ... };   // 멀티스레드 (프로덕션)
```

**AsyncTaskQueue 핵심:**

- `push()`에서 `closed_` 플래그 확인 — 닫힌 큐에는 태스크 추가 불가
- `pop()`은 블로킹 — 큐가 비어있으면 `condition_variable`로 대기
- `tryPop()`은 논블로킹 — 즉시 반환 (실패 시 `std::nullopt`)
- `close()`로 모든 대기 스레드를 깨워 Graceful Shutdown 지원

### 8.3 AtomicFileWriter

**파일:** `src/infra/atomic_writer.h/cpp`

**원자적 파일 쓰기 패턴:**

```
1. 임시 파일에 쓰기: path.jpg.tmp
2. 쓰기 완료 확인 (flush + close)
3. 원자적 이름 변경: path.jpg.tmp → path.jpg (fs::rename)
```

**폴백 전략:**

- 1차: `cv::imencode()` → 바이너리 쓰기 → `fs::rename()`
- 2차: PPM 포맷 폴백 (순수 텍스트, OpenCV 코덱 불필요)

```cpp
bool AtomicFileWriter::saveAsPPM(const cv::Mat& image, const std::string& path) {
    std::ofstream ofs(path);
    ofs << "P3\n" << image.cols << " " << image.rows << "\n255\n";
    // 픽셀 데이터를 텍스트로 직접 쓰기
    for(int y = 0; y < rgb.rows; ++y) {
        for(int x = 0; x < rgb.cols; ++x) {
            cv::Vec3b p = rgb.at<cv::Vec3b>(y, x);
            ofs << (int)p[0] << " " << (int)p[1] << " " << (int)p[2] << " ";
        }
    }
}
```

### 8.4 Benchmark

**파일:** `src/infra/benchmark.h/cpp`

- `measure()`: 단일 실행 시간 측정 (밀리초)
- `measureAverage()`: N회 반복 평균 시간
- `runScalabilityTest()`: 스레드 수별 처리량 비교 (1, 2, 4, 8, ... 스레드)

---

## 9. 유틸리티 모듈

### 9.1 Logger

**파일:** `src/utils/Logger.h/cpp`

spdlog를 래핑하여 프로젝트 전용 로깅 시스템을 제공한다.

**싱크 구성:**

| 싱크 | 타입 | 레벨 | 출력 |
|---|---|---|---|
| 콘솔 | `stdout_color_sink_mt` | DEBUG | 색상 코드 포함 |
| 파일 | `rotating_file_sink_mt` | INFO | 10MB × 3개 로테이팅 |

**매크로 API:**

```cpp
#define LOG_DEBUG(...) Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)  Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)  Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::get()->error(__VA_ARGS__)
```

---

## 10. HTTP 서버 및 API 설계

### 10.1 서버 진입점

```cpp
int main() {
    Logger::init();
    LOG_INFO("Preprocess Server starting...");

    crow::SimpleApp app;
    setup_routes(app);
    app.port(8081).multithreaded().run();
}
```

- **Port 8081:** API Gateway(3000)와 분리된 내부 포트
- **`.multithreaded()`:** IOCP(Windows) / epoll(Linux) 기반 비동기 I/O + 스레드 풀

### 10.2 API 엔드포인트

| Method | Path | 용도 |
|---|---|---|
| GET | `/` | 서버 상태 메시지 |
| GET | `/health` | 헬스 체크 |
| POST | `/preprocess` | 이미지 전처리 |

### 10.3 `/preprocess` 요청 처리 플로우

```
POST /preprocess { "imagePath": "/shared/uploads/test.jpg" }
    │
    ▼ ValidatePreprocessRequest()
    ├── JSON 파싱 검증 → 400 Invalid JSON
    ├── imagePath 필드 존재 확인 → 400 Missing imagePath
    ├── imagePath 비어있음 확인 → 400 imagePath is empty
    └── 파일 존재 확인 → 404 File not found
    │
    ▼ ProcessImageFile() — ImageProcessor 직접 호출 방식
    ├── Step 1: processor.Load(imagePath) → 이미지 로딩
    ├── Step 2: processor.Preprocess(img) → Letterbox Resize + Denoise + Grayscale
    ├── Step 3: processor.DetectEdges(preprocessed, 50, 150) → Canny 에지 검출
    ├── Step 4: processor.EnhanceContours(edges, 3) → 모폴로지 MORPH_CLOSE
    ├── Step 5: processor.Binarize(preprocessed) → 적응적 이진화
    ├── Step 6: cv::cvtColor(binarized, result, COLOR_GRAY2BGR) → 3채널 RGB 변환
    └── (각 단계별 empty() 검증 → 실패 시 std::nullopt 반환)
    │
    ▼ SaveProcessedImage() — ImageProcessor::Save() 사용
    ├── 출력 디렉토리 생성 (없으면)
    └── processor.Save(img, outputPath) → 파일 저장
    │
    ▼ CreatePreprocessResponse()
    └── { "processedPath": "/shared/processed/test_clean.jpg" }
```

**성능 측정:** `std::chrono::high_resolution_clock`으로 처리 시간을 밀리초 단위로 측정하여 로그에 기록.

---

## 11. 이미지 처리 파이프라인

### 11.1 프로덕션 파이프라인 (ImageProcessor 직접 호출)

현재 서버(`server.h`)는 `PipelineFactory`를 사용하지 않고 **`ImageProcessor`의 메서드를 직접 순차 호출**하는 방식으로 프로덕션 파이프라인을 구성한다. 이는 명시적인 단계별 에러 검증과 코드의 직관성을 위해 의도적으로 선택한 설계이다.

**프로덕션 파이프라인 상세 흐름:**

```
입력 이미지 (BGR, 가변 크기)
    │
    ▼  processor.Load(imagePath)
원본 이미지 로딩 (실패 시 nullopt 반환)
    │
    ▼  processor.Preprocess(img)
512×512 Gray (Letterbox Resize + Denoise + Grayscale)
    │
    ▼  processor.DetectEdges(preprocessed, 50, 150)
512×512 Binary (Canny 에지 검출)
    │
    ▼  processor.EnhanceContours(edges, 3)
512×512 Binary (MORPH_CLOSE 에지 간극 메움)
    │
    ▼  processor.Binarize(preprocessed)
512×512 Binary (적응적 이진화)
    │
    ▼  cv::cvtColor(binarized, result, COLOR_GRAY2BGR)
512×512 BGR 3채널 (EfficientNet-B2 호환)
    │
    ▼  출력
```

**각 단계별 에러 검증:** 모든 중간 결과에 대해 `empty()` 체크를 수행하여, 실패 시 즉시 로그를 남기고 `std::nullopt`을 반환한다. 이는 `FilterPipeline`의 체인 방식 대비 디버깅이 용이한 장점이 있다.

### 11.2 디자인 패턴 파이프라인 (FilterPipeline, 테스트/벤치마크 전용)

`IFilter` → `FilterPipeline` → `PipelineFactory` 체계는 프로덕션 서버에서는 사용되지 않지만, 테스트 코드(`examples/`)와 벤치마크에서 활용된다. 이 패턴은 향후 동적 파이프라인 구성(JSON 설정 기반 런타임 변경)이 필요할 경우 서버에 도입될 수 있다.

| 구분 | 프로덕션 (server.h) | 디자인 패턴 (FilterPipeline) |
|---|---|---|
| 방식 | `ImageProcessor` 직접 메서드 호출 | `IFilter` 인터페이스 통한 체인 |
| 에러 처리 | 단계별 `empty()` 검증 + 로그 | 체인 내부 실패 전파 |
| 장점 | 단순, 직관적, 디버깅 용이 | 확장성, 유연성, 동적 구성 |
| 사용 위치 | 서버 프로덕션 | 테스트, 벤치마크, 예제 |

### 11.3 OpenCV 알고리즘 상세

**GaussianBlur:**
- 커널 크기: 5×5, 시그마: 자동(0)
- 고주파 노이즈(카메라 센서 노이즈) 제거
- 에지 검출 전 필수 전처리

**medianBlur:**
- 커널 크기: 3
- 소금-후추 노이즈(이산적 밝기 이상) 제거
- GaussianBlur는 유지하면서 medianBlur는 별도 필터로도 분리

**Canny Edge Detection:**
- L2gradient = true: 정확한 그래디언트 크기 계산 (`√(Gx² + Gy²)`)
- 히스테리시스 임계값: low=50, high=150 (비율 1:3 권장)

**Adaptive Threshold:**
- `ADAPTIVE_THRESH_GAUSSIAN_C`: 가우시안 가중 평균 기반
- `THRESH_BINARY_INV`: 어두운 객체(그림 선)를 255, 밝은 배경을 0으로 변환
- `blockSize=11, C=2`: 11×11 이웃 영역, 평균에서 2를 뺀 값을 임계값으로 사용

---

## 12. 테스트 전략

### 12.1 테스트 계층

| 레벨 | 대상 | 프레임워크 | 파일 |
|---|---|---|---|
| 단위 테스트 | 개별 필터, ThreadPool, AtomicWriter | Google Test | `tests/*.cpp` |
| 통합 테스트 | 서버 + 이미지 처리 | Google Test | `tests/test_main.cpp` |
| 수동 테스트 | 전체 파이프라인 | 실행 파일 | `examples/*.cpp` |

### 12.2 CMake 테스트 통합

```cmake
enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
    tests/test_main.cpp
    tests/test_filters.cpp
    tests/test_thread_pool.cpp
    tests/test_atomic_writer.cpp
    ...
)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main ...)

include(GoogleTest)
gtest_discover_tests(unit_tests)  # CTest 자동 등록
```

---

## 13. 확장성 및 향후 개선 방향

- **동적 파이프라인 구성:** JSON 설정 파일로 런타임에 파이프라인 구성을 변경하고, 이 때 `FilterPipeline` + `PipelineFactory` 패턴을 서버에 도입하여 유연한 필터 조합 지원
- **GPU 가속 (CUDA):** OpenCV의 `cv::cuda` 모듈 도입으로 대규모 배치 처리 가속
- **Docker 컨테이너화:** Linux 빌드 기반 Docker 이미지 생성으로 배포 자동화
- **API 버전닝:** `/v1/preprocess`, `/v2/preprocess` 등 버전별 파이프라인 분리

---

## 14. 결론

Preprocess Server는 **C++17, Crow, OpenCV** 기반의 고성능 이미지 전처리 서버이다. 프로덕션 코드에서는 **`ImageProcessor`의 메서드를 직접 순차 호출**하는 명시적 파이프라인을 채택하여 단계별 에러 검증과 디버깅 용이성을 확보하였으며, **Strategy + Composite + Factory 디자인 패턴** 기반의 `FilterPipeline` 체계는 테스트·벤치마크 코드에서 활용되어 향후 동적 파이프라인 구성 도입 시의 확장 기반을 마련하고 있다. ThreadPool, AtomicFileWriter, TaskQueue 등의 동시성 인프라를 통해 멀티코어 환경에서의 효율적인 병렬 처리를 지원하며, Google Test 기반의 체계적인 테스트 전략으로 코드 품질을 보장한다.

특히 **점진적 개발(Week 1~4)** 방식으로 설계·구현되어, 단순한 헬스 체크 서버에서 시작하여 복잡한 이미지 처리 파이프라인과 동시성 인프라까지 유기적으로 발전한 과정은 소프트웨어 공학적으로도 의미 있는 사례이다.

---

**부록: 파일 목록 및 코드 라인 수**

| 파일 | 라인 수 | 주요 역할 |
|---|---|---|
| `main.cpp` | 13 | 서버 진입점 |
| `server.h` | 178 | HTTP 라우트 + 요청 처리 |
| `image_processor.h/cpp` | 229 | 기초 이미지 처리 |
| `filter.h` | 35 | IFilter 인터페이스 |
| `filter_pipeline.h/cpp` | 91 | 파이프라인 실행 |
| `pipeline_factory.h/cpp` | 76 | 팩토리 |
| 8개 필터 (h+cpp) | ~350 | 개별 필터 구현 |
| `thread_pool.h/cpp` | 136 | 스레드 풀 |
| `task_queue.h` | 135 | 동기/비동기 큐 |
| `atomic_writer.h/cpp` | 164 | 원자적 파일 쓰기 |
| `benchmark.h/cpp` | 112 | 성능 측정 |
| `Logger.h/cpp` | 59 | 로깅 시스템 |
| **합계** | **~1,578** | |
