# C++ 실전 개발 가이드

> **실제 개발에서 바로 사용할 수 있는 C++ 실무 가이드**

---

## 📚 목차
1. [코딩 스타일과 명명 규칙](#1-코딩-스타일과-명명-규칙)
2. [메모리 관리 실무 가이드](#2-메모리-관리-실무-가이드)
3. [예외 처리 실무 전략](#3-예외-처리-실무-전략)
4. [성능 최적화 팁](#4-성능-최적화-팁)
5. [디버깅과 테스트 전략](#5-디버깅과-테스트-전략)
6. [라이브러리 설계 가이드](#6-라이브러리-설계-가이드)
7. [실무에서 자주 하는 실수들](#7-실무에서-자주-하는-실수들)
8. [개발 환경 설정](#8-개발-환경-설정)
9. [Chromium C++ 스타일 가이드 (Dos and Don'ts)](#9-chromium-c-스타일-가이드-dos-and-donts)

---

## 1. 코딩 스타일과 명명 규칙

### 🎯 Google C++ Style Guide 핵심 요약

#### 명명 규칙
```cpp
// 변수명: snake_case
int my_variable = 0;
std::string user_name = "developer";

// 클래스/타입명: CamelCase
class MyClass {};
struct DataStructure {};
using MyType = int;

// 함수명: CamelCase
void CalculateValue();
int GetUserScore();

// 멤버 변수: snake_case_ (마지막에 밑줄)
class Player {
private:
    int health_points_;
    std::string player_name_;
};

// 상수: k로 시작하는 kCamelCase
const int kMaxPlayerCount = 10;
const double kPi = 3.14159;

// 네임스페이스: snake_case
namespace game_engine {
namespace graphics_system {
    // ...
}
}
```

#### 포매팅 규칙
```cpp
// 들여쓰기: 2칸 공백
class Example {
 public:
  void SomeFunction() {
    if (condition) {
      DoSomething();
    }
  }
};

// 한 줄 길이: 80자 제한
void VeryLongFunctionNameWithManyParameters(
    const std::string& first_parameter,
    int second_parameter,
    bool third_parameter);
```

### 🔑 실무 팁
- **일관성이 최우선**: 팀의 기존 코드 스타일을 따라가기
- **의미 있는 이름**: `data`, `temp` 같은 모호한 이름 피하기
- **약어 최소화**: `mgr` 보다는 `manager` 사용

---

## 2. 메모리 관리 실무 가이드

### 🎯 스마트 포인터 선택 기준

#### 실무 상황별 선택

```cpp
// 1. 일반적인 객체 관리 - shared_ptr (기본 선택)
class GameEngine {
private:
    std::shared_ptr<Renderer> renderer_;
    std::shared_ptr<AudioSystem> audio_system_;
    
public:
    GameEngine() {
        renderer_ = std::make_shared<Renderer>();
        audio_system_ = std::make_shared<AudioSystem>();
    }
};

// 2. 성능이 중요한 경우 - unique_ptr
class HighPerformanceSystem {
private:
    std::unique_ptr<Buffer> buffer_;  // 빠른 단독 소유
    
public:
    void ProcessData() {
        buffer_ = std::make_unique<Buffer>(1024);
        // 처리 후 자동 해제
    }
};

// 3. 순환 참조 방지 - weak_ptr
class Parent {
    std::vector<std::shared_ptr<Child>> children_;
};

class Child {
    std::weak_ptr<Parent> parent_;  // 순환 참조 방지
};
```

#### 메모리 누수 방지 체크리스트

```cpp
// ✅ 권장 패턴
class ResourceManager {
public:
    // RAII 패턴 사용
    ResourceManager() {
        resource_ = std::make_shared<Resource>();
    }
    
    // 복사/이동 생성자 명시적 정의
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    ResourceManager(ResourceManager&&) = default;
    ResourceManager& operator=(ResourceManager&&) = default;
    
private:
    std::shared_ptr<Resource> resource_;
};

// ❌ 피해야 할 패턴
class BadResourceManager {
    Resource* resource_;
public:
    BadResourceManager() {
        resource_ = new Resource();  // 위험!
    }
    // 소멸자에서 delete 깜빡할 수 있음
};
```

---

## 3. 예외 처리 실무 전략

### 🎯 try-catch vs if문 결정 가이드

#### 실무 판단 기준

| 상황 | 사용할 것 | 예시 |
| --- | --- | --- |
| 사용자 입력 검증 | **if문** | 이메일 형식 체크, 나이 범위 확인 |
| 파일 I/O | **try-catch** | 파일 읽기/쓰기, 네트워크 통신 |
| 메모리 할당 | **try-catch** | 대용량 데이터 처리 |
| API 호출 | **try-catch** | 외부 라이브러리 사용 |
| 비즈니스 로직 | **if문** | 게임 규칙, 계산 로직 |


#### 실무 예외 처리 패턴

```cpp
// 1. 계층적 예외 처리
class GameSystem {
public:
    void ProcessUserAction() {
        try {
            ValidateInput();
            ExecuteAction();
            SaveGameState();
        } catch (const ValidationError& e) {
            ShowUserMessage("입력이 올바르지 않습니다: " + e.what());
        } catch (const SystemError& e) {
            LogError("시스템 오류: " + e.what());
            ShowUserMessage("일시적인 문제가 발생했습니다.");
        } catch (const std::exception& e) {
            LogError("예상치 못한 오류: " + e.what());
            ShowUserMessage("알 수 없는 오류가 발생했습니다.");
        }
    }
};

// 2. RAII와 예외 안전성
class FileProcessor {
public:
    void ProcessFile(const std::string& filename) {
        auto file = std::make_shared<FileHandle>(filename);
        try {
            auto data = file->ReadData();
            ProcessData(data);
            file->WriteResult(data);
        } catch (...) {
            // file은 자동으로 해제됨 (RAII)
            throw;  // 예외 재발생
        }
    }
};
```

---

## 4. 성능 최적화 팁

### 🎯 실무에서 효과적인 최적화 기법

#### 이동 시맨틱 활용

```cpp
// 1. 함수 반환값 최적화
class DataContainer {
public:
    // ✅ 이동 시맨틱으로 효율적 반환
    std::vector<int> GetLargeData() && {
        return std::move(data_);  // 이동으로 반환
    }
    
    // ✅ 참조로 읽기 전용 접근
    const std::vector<int>& GetLargeData() const& {
        return data_;  // 복사 없이 참조 반환
    }

private:
    std::vector<int> data_;
};

// 2. 컨테이너 최적화
class OptimizedContainer {
public:
    // reserve로 메모리 재할당 최소화
    void AddManyItems(size_t expected_count) {
        items_.reserve(expected_count);
        for (size_t i = 0; i < expected_count; ++i) {
            items_.emplace_back(i);  // 복사 대신 in-place 생성
        }
    }

private:
    std::vector<Item> items_;
};
```

#### 메모리 지역성 최적화

```cpp
// ✅ 캐시 친화적 구조
struct OptimizedGameObject {
    float x, y, z;      // 자주 사용하는 데이터를 연속 배치
    float dx, dy, dz;
    int health;
    // 덜 중요한 데이터는 포인터로
    std::unique_ptr<ComplexData> details;
};

// ❌ 캐시 비친화적 구조
struct BadGameObject {
    std::string name;   // 동적 할당
    float x;
    std::vector<int> inventory;  // 동적 할당
    float y;
    std::map<std::string, int> stats;  // 분산된 메모리
    float z;
};
```

---

## 5. 디버깅과 테스트 전략

### 🎯 효과적인 디버깅 기법

#### 어설션과 로깅

```cpp
#include <cassert>
#include <iostream>

class DebugHelper {
public:
    // 1. 개발 중 어설션 사용
    void ProcessArray(const std::vector<int>& data, size_t index) {
        assert(index < data.size() && "인덱스가 범위를 벗어남");
        assert(!data.empty() && "빈 배열은 처리할 수 없음");
        
        // 실제 처리 로직
        ProcessItem(data[index]);
    }
    
    // 2. 운영 환경용 검증
    bool SafeProcessArray(const std::vector<int>& data, size_t index) {
        if (index >= data.size()) {
            LogError("인덱스 범위 오류: " + std::to_string(index));
            return false;
        }
        
        if (data.empty()) {
            LogWarning("빈 배열 처리 시도");
            return false;
        }
        
        ProcessItem(data[index]);
        return true;
    }

private:
    void LogError(const std::string& message) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
    
    void LogWarning(const std::string& message) {
        std::cout << "[WARN] " << message << std::endl;
    }
};
```

#### 단위 테스트 패턴

```cpp
// 간단한 테스트 프레임워크 예시
class Calculator {
public:
    int Add(int a, int b) { return a + b; }
    int Divide(int a, int b) {
        if (b == 0) throw std::invalid_argument("0으로 나눌 수 없음");
        return a / b;
    }
};

// 테스트 코드
void TestCalculator() {
    Calculator calc;
    
    // 기본 테스트
    assert(calc.Add(2, 3) == 5);
    assert(calc.Add(-1, 1) == 0);
    
    // 예외 테스트
    try {
        calc.Divide(10, 0);
        assert(false && "예외가 발생해야 함");
    } catch (const std::invalid_argument&) {
        // 예상된 예외
    }
    
    std::cout << "모든 테스트 통과!" << std::endl;
}
```

---

## 6. 라이브러리 설계 가이드

### 🎯 재사용 가능한 코드 작성

#### 템플릿 라이브러리 설계

```cpp
// 1. 유연한 인터페이스 제공
template<typename T, typename Comparator = std::less<T>>
class SortedContainer {
public:
    void Insert(const T& item) {
        auto pos = std::lower_bound(data_.begin(), data_.end(), item, comp_);
        data_.insert(pos, item);
    }
    
    bool Contains(const T& item) const {
        auto pos = std::lower_bound(data_.begin(), data_.end(), item, comp_);
        return pos != data_.end() && !comp_(item, *pos);
    }

private:
    std::vector<T> data_;
    Comparator comp_;
};

// 2. 특수화로 최적화
template<>
class SortedContainer<std::string> {
    // 문자열 특화 최적화
    std::set<std::string> data_;  // 더 효율적인 구조 사용
};
```

#### 안전한 API 설계

```cpp
class SafeFileReader {
public:
    // 명시적인 에러 처리
    enum class Result {
        Success,
        FileNotFound,
        PermissionDenied,
        ReadError
    };
    
    Result ReadFile(const std::string& filename, std::string& content) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                return Result::FileNotFound;
            }
            
            content.assign(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
            return Result::Success;
            
        } catch (const std::exception&) {
            return Result::ReadError;
        }
    }
    
    // 편의 함수 (예외 기반)
    std::string ReadFileOrThrow(const std::string& filename) {
        std::string content;
        auto result = ReadFile(filename, content);
        
        switch (result) {
            case Result::Success:
                return content;
            case Result::FileNotFound:
                throw std::runtime_error("파일을 찾을 수 없음: " + filename);
            case Result::PermissionDenied:
                throw std::runtime_error("파일 접근 권한 없음: " + filename);
            case Result::ReadError:
                throw std::runtime_error("파일 읽기 오류: " + filename);
        }
        return "";  // unreachable
    }
};
```

---

## 7. 실무에서 자주 하는 실수들

### 🚨 피해야 할 일반적인 실수

#### 메모리 관리 실수

```cpp
// ❌ 실수 1: 원시 포인터와 스마트 포인터 혼용
class BadExample {
    std::shared_ptr<Resource> smart_ptr_;
    Resource* raw_ptr_;  // 위험!
    
public:
    void SomeFunction() {
        raw_ptr_ = smart_ptr_.get();  // 댕글링 포인터 위험
        smart_ptr_.reset();           // raw_ptr_이 무효화됨
        raw_ptr_->DoSomething();      // 크래시!
    }
};

// ✅ 올바른 방법
class GoodExample {
    std::shared_ptr<Resource> resource_;
    
public:
    void SomeFunction() {
        if (resource_) {
            resource_->DoSomething();  // 안전
        }
    }
};
```

#### 예외 안전성 실수

```cpp
// ❌ 실수 2: 예외 안전하지 않은 코드
class UnsafeClass {
    Resource* resource1_;
    Resource* resource2_;
    
public:
    UnsafeClass() {
        resource1_ = new Resource();
        resource2_ = new Resource();  // 여기서 예외 발생 시 resource1_ 누수
    }
};

// ✅ 올바른 방법
class SafeClass {
    std::unique_ptr<Resource> resource1_;
    std::unique_ptr<Resource> resource2_;
    
public:
    SafeClass() 
        : resource1_(std::make_unique<Resource>())
        , resource2_(std::make_unique<Resource>()) {
        // 예외 발생 시 자동으로 정리됨
    }
};
```

#### 성능 실수

```cpp
// ❌ 실수 3: 불필요한 복사
void ProcessItems(std::vector<LargeObject> items) {  // 복사!
    for (const auto& item : items) {  // 또 복사!
        ProcessItem(item);
    }
}

// ✅ 올바른 방법
void ProcessItems(const std::vector<LargeObject>& items) {  // 참조
    for (const auto& item : items) {  // 참조
        ProcessItem(item);
    }
}
```

---

## 8. 개발 환경 설정

### 🎯 생산성 향상을 위한 도구 설정

#### 컴파일러 플래그

```bash
# 개발 단계
g++ -std=c++17 -Wall -Wextra -g -O0 -fsanitize=address -fsanitize=undefined

# 릴리즈 단계
g++ -std=c++17 -O3 -DNDEBUG -march=native
```

#### CMakeLists.txt 예시

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 디버그 설정
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0")
    add_definitions(-DDEBUG)
endif()

# 릴리즈 설정
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# 라이브러리 링크
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
```

#### 유용한 도구들

```cpp
// 1. Static Analysis (정적 분석)
// clang-tidy, cppcheck 사용

// 2. Memory Debugging
// Valgrind, AddressSanitizer 사용

// 3. Profiling
// gprof, perf, Intel VTune 사용

// 4. Code Coverage
// gcov, lcov 사용
```

---

## 9. Chromium C++ 스타일 가이드 (Dos and Don'ts)

> **참고**: Chromium 프로젝트의 가이드라인을 기반으로 한 실전 조언입니다. 강제 사항은 아니지만, 좋은 C++ 습관을 들이는 데 유용합니다.

### 🎯 헤더 파일 최소화 (Minimize Code in Headers)

*   **사용하지 않는 `#include` 제거**: 컴파일 시간을 줄이고 의존성을 낮춥니다.
*   **전방 선언(Forward Declarations) 활용**: 가능한 경우 `#include` 대신 전방 선언을 사용하여 컴파일 의존성을 끊으세요.
*   **인라인 함수 주의**: 헤더 파일에 함수 정의를 넣으면 암묵적으로 인라인 요청이 됩니다. 간단한 Getter/Setter 외에는 피하세요. 생성자/소멸자는 생각보다 비용이 클 수 있으므로 헤더에 정의하지 않는 것이 좋습니다.

**전방 선언을 사용할 수 있는 경우:**
*   포인터나 참조를 선언할 때 (`MyClass* ptr`, `MyClass& ref`)
*   함수 인자로 참조를 받을 때 (`void func(MyClass& arg)`)
*   친구 클래스(`friend`) 선언 시

**전방 선언을 사용할 수 없는 경우:**
*   객체 인스턴스를 생성할 때 (`MyClass obj`)
*   멤버에 접근할 때 (`obj.Method()`)
*   상속받을 때 (`class Derived : public Base`)

### 🎯 정적 변수 (Static Variables)

C++11부터는 함수 스코프 내의 정적 변수 초기화가 **Thread-safe** 합니다.

```cpp
void foo() {
  static int ok_count = ComputeTheCount();  // OK (Thread-safe)
  static constexpr int better_count = 42;   // 더 좋음 (컴파일 타임 상수)
}
```

### 🎯 복사/이동 생성자 명시 (Explicitly declare copy/move)

클래스의 복사/이동 가능 여부를 `public` 섹션에 명시적으로 선언하거나 삭제(`delete`)하세요. 이는 의도치 않은 복사를 방지하고 코드를 명확하게 만듭니다.

```cpp
class TypeName {
 public:
  TypeName(int arg);
  // ...
  TypeName(const TypeName&) = delete;            // 복사 금지
  TypeName& operator=(const TypeName&) = delete; // 대입 금지
  // ...
};
```

### 🎯 변수 초기화 (Variable Initialization)

상황에 맞는 초기화 문법을 사용하세요.

1.  **단순 값 할당 (`=`)**: 리터럴이나 간단한 값으로 초기화할 때 사용합니다.
    ```cpp
    int i = 1;
    std::string s = "Hello";
    ```
2.  **생성자 문법 (`()`)**: 복잡한 로직이나 명시적 생성자가 필요할 때 사용합니다.
    ```cpp
    MyClass c(1.7, false, "test");
    ```
3.  **유니폼 초기화 (`{}`)**: 위 두 가지가 불가능할 때 사용합니다. (예: 컨테이너 초기화, 명시적 생성자를 가진 멤버 초기화 등)
    ```cpp
    std::vector<std::string> v = {"one", "two"};
    ```
4.  **주의**: `auto`와 `{}`를 함께 쓰지 마세요. 의도치 않게 `std::initializer_list`로 추론될 수 있습니다.

### 🎯 멤버 변수 선언 시 초기화 (Initialize members in declaration)

가능하다면 멤버 변수를 선언과 동시에 초기화하세요. 생성자에서 초기화하는 실수를 줄이고, 기본값을 명확히 할 수 있습니다.

```cpp
class C {
 public:
  C() : a_(2) {} // b_는 선언 시 초기화된 0을 사용

 private:
  int a_;
  int b_ = 0;      // 권장: 선언 시 초기화
  std::string c_;  // string 기본 생성자가 호출되므로 초기화 불필요
};
```

### 🎯 `make_unique`와 `MakeRefCounted` 사용

`new` 연산자를 직접 사용하는 대신 헬퍼 함수를 사용하세요. 코드가 더 간결해지고 예외 안전성이 높아집니다.

```cpp
// BAD
std::unique_ptr<T> t(new T(1, 2, 3));

// GOOD
auto t = std::make_unique<T>(1, 2, 3);
```

### 🎯 `auto`와 포인터

`auto`가 포인터 타입을 추론할 때는 `*`를 명시하여 포인터임을 확실히 하세요.

```cpp
auto item = new Item();   // BAD: item이 포인터인지 한눈에 알기 어려움
auto* item = new Item();  // GOOD: 포인터임이 명확함
```

### 🎯 `const`의 올바른 사용

*   `const` 메서드에서 `non-const` 포인터/참조를 반환하지 마세요.
*   가능한 모든 곳에 `const`를 붙이세요.
*   `const_cast`는 피하세요.

### 🎯 `= default` 활용

기본 생성자/소멸자 등을 구현할 때, 빈 중괄호 `{}` 대신 `= default`를 사용하세요. 가독성이 좋아지고 의도가 명확해집니다.

```cpp
class Good {
 public:
  Good();
  ~Good() = default; // 권장
};
```

---

## 🎓 실무 체크리스트

### 코드 리뷰 시 확인사항

- [ ] **메모리 관리**: 스마트 포인터 사용, 누수 없음
- [ ] **예외 안전성**: RAII 패턴, 적절한 예외 처리
- [ ] **성능**: 불필요한 복사 없음, 이동 시맨틱 활용
- [ ] **가독성**: 명확한 변수명, 적절한 주석
- [ ] **테스트**: 단위 테스트 작성, 경계 조건 검증

### 성능 최적화 우선순위

1. **알고리즘 최적화** (가장 중요)
2. **메모리 접근 패턴** 개선
3. **불필요한 복사** 제거
4. **컴파일러 최적화** 활용
5. **인라인 함수** 사용

### 디버깅 단계

1. **재현 가능한 테스트 케이스** 작성
2. **디버거로 스택 트레이스** 확인
3. **메모리 누수 도구** 실행
4. **정적 분석 도구** 활용
5. **코드 리뷰** 요청

---

## 🛠️ 개발 도구 권장사항

### 컴파일러 설정

```bash
# 개발 단계
g++ -std=c++17 -Wall -Wextra -g -O0 -fsanitize=address

# 릴리즈 단계
g++ -std=c++17 -O3 -DNDEBUG -march=native
```

### 유용한 도구들

- **정적 분석**: clang-tidy, cppcheck
- **메모리 디버깅**: Valgrind, AddressSanitizer
- **프로파일링**: gprof, perf
- **코드 커버리지**: gcov, lcov

---

## 📋 실무 개발 팁

### 1. 생성자/소멸자 체계 활용

```cpp
// 지역 변수의 생명주기를 활용한 자동 관리
void DatabaseTransaction() {
    DatabaseLock lock;  // 생성자에서 락 획득
    
    // 작업 수행
    ProcessData();
    
} // 스코프 종료 시 소멸자에서 자동으로 락 해제
```

### 2. 네임스페이스 활용

```cpp
namespace game_engine {
namespace graphics {
    class Renderer {};
}
namespace audio {
    class SoundEngine {};
}
}

using namespace game_engine::graphics;  // 특정 네임스페이스만 사용
```

### 3. 컴파일 시간 최적화

```cpp
// 전방 선언 활용
class Engine;  // 헤더에서는 전방 선언만

class Car {
private:
    std::unique_ptr<Engine> engine_;  // 포인터/참조면 충분
public:
    void StartEngine();  // 구현은 cpp 파일에
};
```

---

## 🔗 추천 리소스

### 온라인 도구
- **Compiler Explorer**: 컴파일러 최적화 확인
- **C++ Insights**: 템플릿 확장 결과 확인
- **Quick Bench**: 성능 벤치마크

### 참고 서적
- **Effective C++**: Scott Meyers
- **C++ Core Guidelines**: Bjarne Stroustrup
- **Modern C++ Design**: Andrei Alexandrescu

### 커뮤니티
- **Stack Overflow**: 문제 해결
- **Reddit r/cpp**: 최신 동향
- **cppreference.com**: 표준 라이브러리 레퍼런스

---

## ⚡ 빠른 참조 카드

### 자주 사용하는 패턴

```cpp
// 1. RAII 자원 관리
class ResourceManager {
    std::unique_ptr<Resource> resource_;
public:
    ResourceManager() : resource_(std::make_unique<Resource>()) {}
    // 소멸자에서 자동 해제
};

// 2. 팩토리 패턴
std::unique_ptr<Shape> CreateShape(ShapeType type) {
    switch (type) {
        case ShapeType::Circle:
            return std::make_unique<Circle>();
        case ShapeType::Rectangle:
            return std::make_unique<Rectangle>();
    }
}

// 3. 싱글톤 패턴 (Modern C++)
class Singleton {
public:
    static Singleton& Instance() {
        static Singleton instance;
        return instance;
    }
private:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
```

이 가이드를 통해 실무에서 안전하고 효율적인 C++ 코드를 작성하시기 바랍니다!
