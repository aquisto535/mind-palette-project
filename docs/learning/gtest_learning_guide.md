---
title: "Google Test (GTest) 실전 핵심 문법 가이드"
description: "C++ 테스트 프레임워크 GTest의 핵심 매크로와 동작 원리를 제1원칙 사고 기반으로 완벽히 분해하고 체화하는 실전 가이드"
---

# 🧪 Google Test (GTest) 실전 핵심 문법 가이드 (제1원칙 기반)

GTest(Google Test)는 단순히 "테스트 코드를 작성하는 도구"가 아닙니다. 코드가 약속된 **구조(Shape)**를 가졌는지, 의도한 **변환(Logic)**을 수행하는지, 비정상 상황의 **경계(Constraints)**에서도 안전한지를 기계적으로 증명하는 **"실행 가능한 명세서(Executable Specification)"**입니다.

이 가이드는 GTest를 단순히 외우는 것이 아니라, **"왜 이런 매크로가 탄생했는가?"**라는 제1원칙(First Principles) 시점에서 분해하고 재구축합니다.

---

## 🧱 Phase 1: 검증의 물리적 형태 (L1: What - 데이터는 올바른가?)

> **원초적 질문**: "결과값이 내가 기대한 형태(Shape)와 타입(Type)과 일치하는가?"

GTest의 검증 매크로는 크게 **두 가지 철학**으로 나뉩니다. 이 두 가지를 구분하는 것이 테스트 작성의 시작입니다.

### 1-1. `ASSERT_*` vs `EXPECT_*` (치명도에 따른 분해)

*   **`ASSERT_XXX` (치명적 실패, Fatal Failure)**
    *   **의미**: "이 조건이 틀렸다면, 이 테스트의 나머지 코드를 실행하는 것은 의미가 없거나 크래시가 난다! 즉시 테스트를 중단하라."
    *   **예시**: 포인터가 `nullptr`인지 확인 (`ASSERT_NE(ptr, nullptr); ptr->DoSomething();`)
*   **`EXPECT_XXX` (비치명적 실패, Non-fatal Failure)**
    *   **의미**: "이 조건이 틀렸지만, 테스트는 계속 진행해서 실패 목록에 기록해 둬라. 한 번에 여러 실패를 보고 싶다."
    *   **예시**: 이미지의 가로 크기, 세로 크기 각각 확인 (`EXPECT_EQ(img.cols, 512); EXPECT_EQ(img.rows, 512);`)

### 1-2. 핵심 단언문(Assertions) 목록

**A. 진위 판별 (Bool)**
*   `EXPECT_TRUE(condition);` : `condition`이 true인가? (객체 비어있음 확인 등에 유용 `EXPECT_TRUE(img.empty());`)
*   `EXPECT_FALSE(condition);`: `condition`이 false인가?

**B. 동등 비교 (Equality)**
*   `EXPECT_EQ(val1, val2);` : Equal (==)
*   `EXPECT_NE(val1, val2);` : Not Equal (!=)
    *   *💡 왜 `EXPECT_TRUE(a == b)`를 안 쓰고 `EXPECT_EQ(a, b)`를 쓸까요? 실패 시 **"Expected: 5, Actual: 10"**처럼 구체적인 값을 터미널에 출력해 주기 때문입니다.*

**C. 부등 비교 (Inequality)**
*   `EXPECT_LT(val1, val2);` : Less Than (<)
*   `EXPECT_LE(val1, val2);` : Less or Equal (<=)
*   `EXPECT_GT(val1, val2);` : Greater Than (>)
*   `EXPECT_GE(val1, val2);` : Greater or Equal (>=)

**D. 문자열 비교 (C-Strings)**
*   `EXPECT_STREQ(str1, str2);` : C스타일 문자열(`char*`) 내용이 같은가? (대소문자 구분)
*   `EXPECT_STRCASEEQ(str1, str2);` : 대소문자 무시하고 같은가?
    *   *💡 `std::string`은 그냥 `EXPECT_EQ`를 쓰면 오버로딩되어 동작합니다.*

---

## ⚙️ Phase 2: 상태 변환과 격리 로직 (L2: How - 상태 변환은 독립적인가?)

> **원초적 질문**: "의존성이 얽힌 객체의 변환 로직을 어떻게 부작용(Side-effect) 없이 독립적으로 테스트할 것인가?"

단순한 전역 함수 하나를 테스트할 때는 매번 객체를 만들면 되지만, 서버나 이미지 처리 파이프라인 같은 복잡한 객체는 초기 조건(Context)을 세팅하는 데 코드가 너무 많이 듭니다. 여기서 `TEST`와 `TEST_F`의 근본적인 차이가 탄생합니다.

### 2-1. `TEST(TestSuitName, TestName)` : 독립 함수 테스트

상태(State)를 가지지 않는 순수 함수(Pure function)나 독립적인 알고리즘을 테스트할 때 사용합니다.

```cpp
// 상태를 외부에서 공유받지 않는 독립 테스트
TEST(GenerateOutputPathTest, AppendsCleanToFilename) {
    std::string input = "test.jpg";
    EXPECT_EQ(GenerateOutputPath(input), "test_clean.jpg");
}
```

### 2-2. `TEST_F(TestFixtureName, TestName)` : 상태 의존성 테스트 (핵심 ⭐)

`TEST_F`의 'F'는 **Fixture(기구, 설비)**의 약자입니다. **"무언가를 고정시킨 상태에서"** 테스트하겠다는 뜻입니다.

*   **배경**: 여러 테스트 함수가 동일한 초기 객체(예: `ImageProcessor`, `cv::Mat test_image`)를 필요로 할 때, 코드의 중복을 막습니다.
*   **작동 원리 (제1원칙 붕괴 방지)**: GTest는 한 테스트가 끝나면 해당 Fixture 객체를 **파괴하고, 다음 테스트 때 완전히 새롭게 다시 생성**합니다. (메모리 공유로 인한 오염 원천 차단)

#### 🛠️ Fixture 클래스의 해부학 (라이프사이클)

```cpp
// 1. ::testing::Test를 상속받은 Fixture 클래스 정의
class ImageProcessorTest : public ::testing::Test {
protected:
    // 2. SetUp() : "매번" 테스트(TEST_F)가 시작되기 "직전"에 호출됨.
    // 여기서 공통 초기화 로직 (이미지 그리기, DB 연결 등) 수행
    void SetUp() override {
        testImage = cv::Mat(512, 512, CV_8UC3, cv::Scalar(255, 255, 255));
    }

    // 3. TearDown() : "매번" 테스트가 종료된 "직후"에 호출됨. (메모리 해제 등)
    // C++에서는 소멸자가 있으므로 잘 쓰이지 않음 (파일 닫기 용도 등)
    void TearDown() override { }

    // 멤버 변수: 이 안의 모든 TEST_F가 자유롭게 접근 가능!
    cv::Mat testImage;
    ImageProcessor processor;
};

// 4. TEST_F 매크로 사용 (첫 번째 인자는 반드시 Fixture 클래스 이름!)
TEST_F(ImageProcessorTest, Preprocess_OutputIsBGR) {
    // 이미 SetUp()이 호출되어 testImage가 만들어져 있음!
    cv::Mat result = processor.Preprocess(testImage);
    EXPECT_EQ(result.channels(), 3);
} 
// 이 시점에서 ImageProcessorTest 객체는 소멸됨. (다음 TEST_F를 위해 깨끗한 상태 유지)
```

---

## 🛡️ Phase 3: 경계 조건 방어 (L3: Why - 비정상 상황에서도 안전한가?)

> **원초적 질문**: "입력이 미쳤거나(Cracked), 의존 시스템이 죽었을(Down) 때도 내 시스템은 예측 가능하게 실패하는가?"

### 3-1. 예외(Exception) 테스트

예외가 발생하는 것이 정상인 상황(방어 코드)을 검증합니다. "이 상황에서는 무조건 죽어야 해!"를 증명합니다.

*   `EXPECT_THROW(statement, exception_type);`
    *   의미: `statement`를 실행했을 때 반드시 `exception_type` 예외가 **던져져야(Throw)** 테스트가 통과합니다.
    *   ```cpp
        // 빈 이미지를 넣으면 std::invalid_argument 예외가 터져야 성공!
        EXPECT_THROW(processor.Process(cv::Mat()), std::invalid_argument);
        ```
*   `EXPECT_NO_THROW(statement);`
    *   의미: 무슨 일이 있어도 예외가 터지면 안 됩니다.

### 3-2. 죽음(Death) 테스트

예외 처리조차 불가능하고 프로그램이 강제로 종료(Abort)되어야 하는 심각한 시스템 위반(Assertion failure 등)을 테스트합니다.

*   `EXPECT_DEATH(statement, regex_pattern);`
    *   의미: `statement` 실행 시 프로세스가 죽고(Crash/Abort), 남긴 에러 메시지가 정규식 `regex_pattern`과 일치해야 통과.
    *   *💡 GTest는 이 테스트를 위해 내부적으로 자식 프로세스(fork)를 띄워 죽는 것을 관찰합니다.*

### 3-3. Mocking (GMock 연동 - 시스템 고립)

(GTest와 함께 자주 쓰이는 GMock의 개념)
데이터베이스 장애나, 아직 완성되지 않은 네트워크 계층 등을 흉내 내어 "내 로직이 외부 시스템의 응답에 올바르게 반응하는가?"(L3 경계 조건)를 검증합니다.

```cpp
// 가짜(Mock) API 서버 응답 객체 만들기
EXPECT_CALL(mockApi, FetchImage("url"))
    .Times(1)                               // 1번 호출되어야 하고
    .WillOnce(Return(cv::Mat()));           // 고의로 빈 이미지를 리턴해라 (네트워크 장애 상황 묘사)

// 이 장애 상황에서 우리 코드가 어떻게 버티는지 검증
EXPECT_EQ(processor.HandleNetworkImage("url"), false); 
```

---

## 🔬 Phase 4: 실행의 통제 (L3: Why - 원하는 테스트만 실행하기)

> **원초적 질문**: "수백 개의 테스트 중 내가 방금 수정한 코드의 사이드 이펙트(Side Effect)만 어떻게 타겟팅해서 빠르게 확인할 것인가?"

`test_main.cpp`의 맨 아래에 있는 `RUN_ALL_TESTS()`는 말 그대로 빌드된 모든 테스트를 한 번에 실행합니다. 하지만 실전에서 하나의 버그를 고칠 때마다 모든 테스트를 돌리는 것은 시간 낭비이자 제1원칙에 위배됩니다. 이를 통제하기 위해 터미널(혹은 IDE 인자 설정)에서 `--gtest_filter` 옵션을 사용합니다.

### 4-1. 패턴 매칭의 기본 원리

GTest 필터는 `TestSuitName.TestName` 형식을 따르며, 와일드카드(`*`, `?`)와 부정(`-`) 연산자를 지원합니다. 하나의 명령어에서 여러 패턴을 결합하여 복잡한 실행 범위를 통제할 수 있습니다.

### 4-2. 구체적인 실전 명령어 사례 (`unit_tests.exe` 기준)

| 목적 | 터미널 명령어 | 설명 |
|:---|:---|:---|
| **가장 좁은 통제 (단일 타겟)** | `unit_tests.exe --gtest_filter="ImageProcessorTest.Preprocess_OutputIsBGR"` | 이름이 가장 정확히 일치하는 단 하나의 함수만 독립 실행 |
| **픽스처(Fixture) 전체 단위** | `unit_tests.exe --gtest_filter="ImageProcessorTest.*"` | `ImageProcessorTest` 클래스 안에 있는 모든 테스트 블록 실행 |
| **기능(Keyword) 중심 패턴** | `unit_tests.exe --gtest_filter="*Preprocess*"` | 테스트 수트나 함수 이름 어딘가에 "Preprocess" 단어가 들어간 전체 실행 |
| **다중 패턴 병합 (OR 조건)** | `unit_tests.exe --gtest_filter="ServerTest.*:Advanced*"` | `ServerTest`와 이름이 `Advanced`로 시작하는 모든 테스트 동시 실행 (콜론 `:` 묶음) |
| **의도적 배제 (NOT 조건)** | `unit_tests.exe --gtest_filter="ImageProcessorTest.*:-*Empty*"` | `ImageProcessorTest`를 다 실행하되, 중간에 `-` 기호 뒤의 패턴(이름에 "Empty"가 들어간 것)은 강제 제외 |

*💡 **전문가 팁 (디버깅의 심리전)**: 비주얼 스튜디오에서 브레이크포인트를 걸고 디버깅을 할 때, 엉뚱한 테스트 함수에서 멈춰서 흐름이 끊기는 경험을 해보셨을 겁니다. "디버깅(F5)" 버튼을 누르기 전, 프로젝트 시작 옵션에 반드시 위 필터를 걸어 내가 뚫어져라 쳐다볼 단 하나의 테스트만 **격리된 상태에서** 코드가 흐르게 만드세요.*

---

## 🎯 3줄 요약 (어떻게 코드를 읽을 것인가?)

1.  **L1 (데이터 평가)**: 상태 공유가 필요 없는 유틸리티 함수는 `TEST` 문법으로 읽고, `EXPECT_EQ`로 반환값의 Shape가 의도대로인지 확인한다.
2.  **L2 (변환과 맥락)**: `TEST_F`를 발견하면 무조건 **`SetUp()` 함수로 올라가서 초기 생태계(Context)가 어떻게 구성되었는지** 먼저 확인하고 개별 테스트 로직을 읽는다.
3.  **L3 (예측된 재난)**: `_THROW`나 `_DEATH`가 보이면, 개발자가 "절대 들어오면 안 되는 엣지 케이스"를 어떻게 방어했는지 그 악의적 입력의 경계를 확인한다.
