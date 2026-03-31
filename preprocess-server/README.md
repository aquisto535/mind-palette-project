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
이미지 5단계 전처리(축소 → 노이즈 제거 → 이진화 → ROI 추츨 → 합성) 로직을 **FilterPipeline** 구조로 관리합니다.

#### 3.1 확장에 열려있는 설계 (Strategy & Composite Pattern)
- `IFilter`: 모든 필터의 규격 (Strategy).
- `FilterPipeline`: 여러 필터를 담아 한 번에 실행하는 컨테이너 (Composite).
- 개방-폐쇄 원칙(OCP): 새로운 효과를 추가할 때 기존 코드를 수정하지 않고 새로운 `IFilter` 구현체만 추가하면 됩니다.

#### 3.2 Fluent Interface & Method Chaining
```cpp
pipeline.add(std::make_unique<ResizeFilter>())
        .add(std::make_unique<DenoiseFilter>())
        .add(std::make_unique<HybridPreprocessFilter>());
```
- `add()` 함수가 얕은 복사나 이동 대신 `FilterPipeline&`(자신의 참조)을 반환하게 하여 가독성을 높이고 불필요한 메모리 복사를 원천 차단했습니다.

#### 3.3 소유권과 이동 시맨틱 (Modern C++)
- 각 필터(`IFilter`)는 `std::unique_ptr`로 독점 소유됩니다.
- 이는 복사 금지(`= delete`)를 통해 컴파일 타임에 물리적으로 얕은 복사로 인한 Dangling Pointer 등 메모리 버그를 원천 차단하며, 필요한 경우 이동 생성자(`&& = default`)를 통해 안전하게 소유권을 전달(`std::move`)합니다.

---

## 🚀 워크플로우 (이미지 하나가 처리되는 과정)

1. **수신**: HTTP POST 요청 도착
2. **위임**: I/O 스레드가 `Promise/Future` 쌍을 만들고 `ThreadPool`의 `tasks_` 큐에 전처리 람다 함수(작업) 삽입 (`enqueue`)
3. **가동**: 자고 있던 워커 스레드가 `notify_one()`을 듣고 깨어나 큐에서 작업을 `pop`함
4. **전처리 (자물쇠 해제된 상태)**:
   - `ResizeFilter`: 768px 이하로 크기를 축소하여 성능 향상 (< 100ms)
   - `DenoiseFilter`: 종이 질감을 블러(가우시안)로 날림
   - `HybridPreprocessFilter`: Binarize(흰색/검은색 분리) → SmartCrop(의미없는 여백 제거) → Merge(EfficientNet-B2 입력 형태인 3채널 합성)
5. **완료 알림**: 결과를 `promise->set_value()`에 담아 I/O 스레드를 깨움
6. **안전 저장**: `AtomicFileWriter`로 `.tmp`에 임시 저장 후 `rename` 발동
7. **응답**: API Gateway에 처리된 이미지의 파일 경로 반환

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
