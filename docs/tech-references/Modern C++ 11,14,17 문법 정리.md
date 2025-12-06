___
### 1. C++11 (대변혁: Modern C++의 시작)

가장 많은 변화가 있었던 버전입니다. 이 기능들은 선택이 아니라 필수입니다.

| 분류        | 기능 (Feature)                | 설명                         | 간단 예시                                   |
| --------- | --------------------------- | -------------------------- | --------------------------------------- |
| **핵심 문법** | **Auto**                    | 변수 타입 자동 추론                | `auto i = 10;`                          |
|           | **Range-based for loop**    | 범위 기반 for문                 | `for(auto& x : vec) { ... }`            |
|           | **Lambda Expressions**      | 익명 함수 (함수 객체)              | `[](int x){ return x+1; }`              |
|           | **Move Semantics**          | 이동 시멘틱 (R-value reference) | `std::move(obj)`, `T&&`                 |
|           | **nullptr**                 | 타입 안전성이 보장된 null 포인터       | `int* p = nullptr;`                     |
|           | **Smart Pointers**          | 자동 메모리 관리                  | `unique_ptr`, `shared_ptr`              |
|           | **Uniform Initialization**  | 중괄호 `{}` 초기화 통일            | `int x{5};`, `vector<int> v{1,2};`      |
|           | **constexpr**               | 컴파일 타임 상수 계산               | `constexpr int size() { return 10; }`   |
|           | **enum class**              | 스코프가 있는 열거형 (타입 안전)        | `enum class Color { Red };`             |
|           | **override / final**        | 가상 함수 명시적 제어               | `void func() override;`                 |
|           | **default / delete**        | 생성자 등 함수 생성 제어             | `Func() = default;`, `Func() = delete;` |
|           | **Decltype**                | 표현식의 타입 추론                 | `decltype(x) y = x;`                    |
|           | **Delegating Constructors** | 생성자 위임 (다른 생성자 호출)         | `Class() : Class(0) {}`                 |
|           | **Variadic Templates**      | 가변 인자 템플릿                  | `template<typename... Args>`            |
|           | **Type Alias (using)**      | `typedef` 대체               | `using Vec = vector<int>;`              |
|           | **static_assert**           | 컴파일 타임 조건 검사               | `static_assert(N > 0, "Error");`        |
|           | **Attributes**              | 컴파일러 지시자 (noreturn 등)      | `[[noreturn]] void die();`              |
| **라이브러리** | **std::thread**             | 표준 스레드 라이브러리               | `std::thread t(func);`                  |
|           | **std::mutex / atomic**     | 동기화 및 원자적 연산               | `std::lock_guard`, `std::atomic<int>`   |
|           | **std::array**              | 고정 크기 배열 래퍼 (C배열 대체)       | `std::array<int, 5> arr;`               |
|           | **std::unordered_map**      | 해시 맵 (Hash Table)          | `unordered_map<string, int> m;`         |
|           | **std::tuple**              | pair의 확장판 (N개 데이터)         | `tuple<int, float, char> t;`            |
|           | **std::regex**              | 정규 표현식                     | `regex_match(...)`                      |
|           | **std::chrono**             | 시간 라이브러리                   | `chrono::seconds(5)`                    |

---

### 2. C++14 (완성도 향상: C++11의 버그 수정 및 보완)

C++11에서 "이게 왜 안 되지?" 싶었던 것들이 해결된 버전입니다.

|분류|기능 (Feature)|설명|간단 예시|
|---|---|---|---|
|**핵심 문법**|**Generic Lambdas**|람다 매개변수에 `auto` 사용 가능|`[](auto x, auto y) { return x+y; }`|
||**Return type deduction**|함수 리턴 타입 `auto` 추론 가능|`auto func() { return 5; }`|
||**Relaxed constexpr**|`constexpr` 함수 내 변수/루프 사용 가능|`constexpr` 함수 내 `if`, `for` 가능|
||**Variable Templates**|변수에도 템플릿 적용 가능|`template<typename T> constexpr T pi = ...;`|
||**Binary Literals**|2진수 표기법|`int x = 0b1010;`|
||**Digit Separators**|숫자 가독성을 위한 따옴표|`int money = 1'000'000;`|
||**[[deprecated]]**|사용 금지 권장 속성 추가|`[[deprecated("Use newFunc")]] void old();`|
|**라이브러리**|**std::make_unique**|`unique_ptr` 생성 도우미 (중요!)|`auto p = std::make_unique<T>();`|
||**std::shared_lock**|Read-Write Lock 지원 (읽기 전용 락)|`shared_lock<shared_mutex> lock(m);`|
||**User-defined Literals**|표준 라이브러리용 리터럴 (시간 등)|`using namespace std::literals; auto t = 10s;`|
||**std::exchange**|값을 교체하고 옛 값 리턴|`old_val = std::exchange(curr, new_val);`|

---

### 3. C++17 (생산성 증대: 코드를 짧고 예쁘게)

파이썬 같은 현대적인 언어의 편리한 기능들이 대거 도입되었습니다. 현재 많은 기업의 표준 버전입니다.

|분류|기능 (Feature)|설명|간단 예시|
|---|---|---|---|
|**핵심 문법**|**Structured Binding**|구조체/튜플 등의 값을 한 번에 분해 할당|`auto [x, y] = myPoint;`|
||**If/Switch with Init**|조건문 안에서 변수 초기화|`if (auto iter = m.find(k); iter != m.end())`|
||**Inline Variables**|헤더 파일에 변수 정의 가능 (링킹 에러 방지)|`inline int globalVal = 10;`|
||**If constexpr**|컴파일 타임에 분기 처리 (템플릿 필수)|`if constexpr (is_integral_v<T>) ...`|
||**Fold Expressions**|가변 인자 템플릿을 쉽게 연산|`(args + ...)` (모든 인자 합계)|
||**Template deduction for classes**|클래스 템플릿 인자 생략 가능|`pair p(1, 5.0);` (`pair<int, double>` 생략)|
||**Nested Namespaces**|네임스페이스 중첩 간소화|`namespace A::B::C { ... }`|
||**Copy Elision**|RVO(리턴 값 최적화) 강제 보장|리턴 시 복사 생성자 호출 생략 보장|
||**[[nodiscard]]**|리턴값 무시하면 경고 발생|`[[nodiscard]] int func();`|
||**[[maybe_unused]]**|사용 안 해도 경고 끄기|`[[maybe_unused]] int x;`|
||**[[fallthrough]]**|switch 문에서 의도적인 흐름 표시|case 문 사이에 작성|
|**라이브러리**|**std::filesystem**|파일 시스템(경로, 파일 조작) 표준화|`fs::exists("file.txt")` (매우 중요!)|
||**std::string_view**|문자열 복사 없는 읽기 전용 뷰 (성능↑)|`void func(string_view sv);`|
||**std::optional**|값이 있을 수도 없을 수도 있는 타입|`optional<int> ret;`|
||**std::variant**|Type-safe Union (여러 타입 중 하나)|`variant<int, string> v;`|
||**std::any**|아무 타입이나 담는 컨테이너|`any a = 1; a = "str";`|
||**Parallel Algorithms**|STL 알고리즘 병렬 처리 실행 정책|`sort(execution::par, v.begin(), v.end())`|
||**std::byte**|바이트 단위 데이터 전용 타입|`std::byte b;`|

---

### 💡 학습 팁

이 표를 보시고 "이걸 언제 다 외우지?"라고 걱정하지 않으셔도 됩니다.

1. **필수:** **C++11의 핵심 문법**과 **C++14의 `make_unique`**는 무조건 알아야 합니다.
2. **추천:** 코드를 예쁘게 짜고 싶다면 **C++17의 `Structured Binding`**과 **`std::filesystem`**을 먼저 써보세요.
3. 나머지는 필요할 때 "C++17에 이런 게 있었나?" 하고 이 표를 다시 찾아보시면 됩니다.