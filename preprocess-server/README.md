# Preprocess Server (C++)

Mind Palette 시스템의 **고성능 이미지 전처리 및 정제**를 담당하는 마이크로서비스입니다. C++17 기반으로 작성되었으며, 메모리 관리(소유권), 동시성(멀티스레딩), 그리고 견고한 파일 I/O 시스템을 제1원칙(First Principles)에 따라 설계하였습니다.

---

## 🏗️ 시스템 아키텍처

서버는 크게 **3개의 계층(Layer)**으로 분리되어 각자의 역할에 집중합니다 (SRP: 단일 책임 원칙).

### 1. HTTP Layer (`server.h`)
- **Crow Web Framework**를 활용하여 REST API (`POST /preprocess`)를 제공합니다.
- HTTP I/O 스레드가 무거운 이미지 처리에 블로킹되지 않도록, 요청이 들어오면 `ThreadPool`에 작업을 위임하고 `Future`를 통해 비동기 대기합니다.

### 2. Infra Layer 
#### 2.1 ThreadPool (`thread_pool.h`)
- **Producer-Consumer 패턴**: I/O 스레드(Producer)가 작업을 큐에 넣고, 워커 스레드(Consumer)가 꺼내서 처리합니다.
- 핵심 원리:
  - `condition_variable`: Polling으로 인한 CPU 낭비를 막고, 작업이 없을 때는 스레드를 재웁니다(Sleep/Wake 메커니즘).
  - `emplace_back`: `workers_.emplace_back([this](){...})`를 통해 객체를 복사해서 넣는 대신, 벡터 내부에서 직접 `std::thread`를 생성하여 오버헤드를 없앱니다.
  - `[this] 캡처`: 스레드가 람다 패키지 내부에 관리자 사무실 열쇠(`this`)를 가지고 태어나 `mutex_`나 `tasks_`에 안전하게 접근합니다.

#### 2.2 AtomicFileWriter (`atomic_writer.h`)
- **원자적 쓰기 (Atomic Write)**: 중간 상태 로 인한 파일 손상을 막습니다.
  - 직접 쓰는 대신 `.tmp` 파일에 완벽히 다 작성한 후, OS의 단일 연산인 `rename`을 통해 진짜 이름표를 교체합니다.
- **Fail-Fast 원칙**: `OpenCV imencode` 실패와 같은 데이터 오류를 숨기기 위한 구형 Fallback 로직을 제거하여, 500 에러를 통해 즉각 오류를 드러내도록 설계되었습니다.

### 3. Core Layer (핵심 도메인)
이미지 5단계 전처리(축소 → 노이즈 제거 → 이진화 → ROI 추출 → 합성) 로직을 **FilterPipeline** 구조로 관리하며, 각 단계는 **제1원칙(First Principles)**에 따라 설계되었습니다.

#### 3.1 전처리 알고리즘 딥다이브 (데이터 흐름 3단계)

본 프로젝트의 전처리는 단순한 정제를 넘어, AI 모델(EfficientNet-B2)이 스케치 도메인의 특징을 가장 잘 추출할 수 있도록 하는 **도메인 적응(Domain Adaptation)** 과정입니다.

| 알고리즘 (Algorithm) | L1: 데이터 구조 (What) | L2: 변환 로직 (How) | L3: 제약과 검증 (Why) |
| :--- | :--- | :--- | :--- |
| **CLAHE & NlMeans** | `[H, W, 1]` Grayscale | 로컬 히스토그램 평활화 + 비지역적 평균 노이즈 제거 | **조명 불균일 및 저화질 방어**: 그림자가 진 스캔본이나 구겨진 종이에서도 일관된 선 명도를 확보합니다. |
| **Adaptive Binarize** | `[H, W, 1]` 0/255 Binary | 주변 픽셀 평균 대비 임계값 결정 (`Gaussian C`) | **선 두께 강건성**: 연필, 볼펜, 네임펜 등 도구에 상관없이 형태의 기하학적 구조를 이진 공간에 고정합니다. |
| **Morphology Close** | 이진화된 텐서 데이터 | 팽창(Dilation) 후 침식(Erosion) 연산 | **선 끊김 보정**: 아이들이 살살 그려 끊어진 실선을 하나로 연결하여 AI가 윤곽선을 놓치지 않게 합니다. |
| **Smart ROI (Contour)** | `std::vector<Point>` | 최대 면적 컨투어(`Dominant`) 기반 바운딩 박스 추출 | **부속 자극 제거**: 인물화 외의 잡영이나 종이 끝부분을 제거하고 분석 대상인 '인물'에만 초점을 맞춥니다. |
| **Hybrid 3-Channel** | `[512, 512, 3]` Multi-channel | R(Gray), G(InvBinary), B(DistanceMap) 합성 | **도메인 한계 극복**: RGB 질감에 최적화된 사전학습 모델에 '공간적 거리감'과 '필압' 정보를 채널별로 주입합니다. |

#### 3.2 정량적 분석 레이어 (Analysis Layer)

AI 추론이 놓칠 수 있는 수치적 특징을 직접 추출하여 임상적 힌트를 제공합니다.

*   **PressureAnalyzer (필압 분석)**: R-채널(Grayscale)의 픽셀 히스토그램을 분석하여 평균 밝기와 밀도를 산출합니다. 과도한 필압(공격성)이나 너무 낮은 필압(무기력)을 수치화합니다.
*   **TremorAnalyzer (선 떨림 분석)**: **Hu Moments (불변 모멘트)**를 활용하여 선의 기하학적 복잡도를 계산합니다. 신경학적 미세 떨림이나 형태의 왜곡 정도를 정량적으로 측정합니다.

#### 3.3 확장에 열려있는 설계 (Strategy & Composite Pattern)
- `IFilter`: 모든 필터의 규격 (Strategy).
- `FilterPipeline`: 여러 필터를 담아 한 번에 실행하는 컨테이너 (Composite).
- 개방-폐쇄 원칙(OCP): 새로운 효과를 추가할 때 기존 코드를 수정하지 않고 새로운 `IFilter` 구현체만 추가하면 됩니다.

#### 3.3 소유권과 이동 시맨틱 (Modern C++)
- 각 필터(`IFilter`)는 `std::unique_ptr`로 독점 소유됩니다.
- 이는 복사 금지(`= delete`)를 통해 컴파일 타임에 메모리 버그를 원천 차단하며, `std::move`를 통해 안전하게 소유권을 전달합니다.

---

## 🚀 워크플로우 (이미지 하나가 처리되는 과정)

1. **수신**: HTTP POST 요청 도착 및 `ThreadPool` 작업 큐 삽입.
2. **최적화 축소 (Early Resize)**: 성능 향상을 위해 768px 이하로 선제 축소 (전체 지연시간 < 100ms 달성).
3. **심층 정제 (Deep Clean)**: 
   - `ClaheFilter`: 부분적 명암 대비 극대화.
   - `NlMeansDenoiseFilter`: 엣지는 보존하고 배경의 종이 질감만 제거.
4. **기하학적 복원 (Geometric Reconstruction)**: 
   - `AdaptiveThreshold`: 조명에 강건한 이진화.
   - `MorphologyFilter`: 미세한 선 떨림 보정 및 끊어진 선 연결.
5. **도메인 합성 (Hybrid Merge)**: 
   - **R 채널**: 원본 필압 정보 보존.
   - **G 채널**: 순수 형태 윤곽선.
   - **B 채널**: Distance Transform을 통한 윤곽선 거리 정보 (공간적 특징).
6. **안전 저장 (Atomic Write)**: `AtomicFileWriter`를 통한 파일 무결성 보장.
7. **응답**: 분석용 하이브리드 이미지 경로 반환.

---


---

## 🛠 빌드 및 실행 가이드 (Windows)

- 의존성 관리는 `vcpkg`를 통해 자동화되어 있습니다. (`vcpkg.json` 및 `vcpkg-configuration.json` 사용)
- **빌드 (명령어)**:
  ```powershell
  # 정적 링크 관련 예전 Heap Mismatch 이슈 해결을 위해 동적 링크 빌드를 수행합니다.
  cmake -B build -S .
  cmake --build build --config Release
  ```
- **테스트 (CTest)**:
  ```powershell
  ctest --test-dir build --output-on-failure
  ```
  (* TDD 기반 개발 지원을 위해 Google Test (`unit_tests`) 모듈이 세팅되어 있습니다.*)
