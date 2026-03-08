# Week 4.7 트러블슈팅: 런타임 어설션(Heap Mismatch), 인코딩, 그리고 실행 경로 이슈

이 문서는 `preprocess-server`의 유닛 테스트(`unit_tests.exe`) 실행 시 발생한 세 가지 치명적인 오류들의 근본 원인과 해결책을 **제1원칙 사고**를 바탕으로 기록합니다.

---

## 1. 런타임 어설션 오류: `Debug Assertion Failed! (__acrt_first_block == header)`

### 🔴 문제 현상
*   비주얼 스튜디오에서 `unit_tests.exe` 디버그 실행 시 "Debug Assertion Failed!" 팝업과 함께 `debug_heap.cpp`에서 크래시 발생.
*   빌드는 성공하지만 런타임에 **힙(Heap) 주권 문제**가 발생함.

### 🧠 근본 원인
*   **CRT 링크 방식의 불일치(Mismatch)**:
    *   `CMakeLists.txt`에서 강제한 **정적 CRT(`/MTd`)**와 `vcpkg`가 기본 설치한 **동적 CRT(`/MDd`)** 라이브러리가 혼용됨.
    *   애플리케이션과 라이브러리가 서로 다른 힙(Heap) 메모리 영역을 사용하여, `std::string`이나 `cv::Mat`과 같이 메모리 해제가 필요한 객체를 주고받을 때 다른 영역의 메모리를 해제하려다 크래시가 난 것임.

### ✅ 해결책
*   **동적 링크(Dynamic Link)로의 회귀**: 윈도우 C++ 개발 및 `vcpkg` 생태계의 표준인 동적 링크 방식으로 일원화.
*   **CMake 수정**:
    ```cmake
    if(WIN32)
        # vcpkg 기본 트리플렛(x64-windows)을 사용하여 동적 링크(/MDd)로 통일
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
        set(X_VCPKG_APPLOCAL_DEPS ON CACHE BOOL "") # 필요한 DLL을 실행 파일 옆으로 자동 복사
    endif()
    ```

---

## 2. MSVC 인코딩 오류: `C4819` 및 클래스 선언 오류 `C2523`

### 🔴 문제 현상
*   `test_main.cpp`의 특정 줄에서 한글 주석을 추가한 후, 300개가 넘는 IntelliSense 오류(헤더 미설치 등)와 "멤버 함수를 클래스 외부에서 다시 선언할 수 없습니다"라는 엉뚱한 에러 발생.

### 🧠 근본 원인
*   **개행 먹힘(Newline Swallowing)**:
    *   한글 주석의 특정 바이트 시퀀스가 MSVC 컴파일러에 의해 **줄바꿈 무시 문자(`\`)**로 오인됨.
    *   결과적으로 주석 바로 아랫줄에 있던 `cv::rectangle`과 클래스를 닫는 중괄호 `}`가 주석 처리됨으로써, 전체 클래스 구조가 파괴되고 그 아래의 `TEST_F`가 클래스 내부 선언으로 오해받음.

### ✅ 해결책
*   **컴파일러 인코딩 강제 설정**: `CMakeLists.txt`에 `/utf-8` 플래그 추가.
    ```cmake
    if(MSVC)
        add_compile_options(/W3 /utf-8) # 모든 소스코드를 UTF-8로 해석하도록 강제
    endif()
    ```

---

## 3. PowerShell 실행 경로 오류: `The term 'unit_tests.exe' is not recognized`

### 🔴 문제 현상
*   PowerShell 터미널에서 `unit_tests.exe`를 직접 쳤으나 명령어를 찾을 수 없다는 오류 발생.

### 🧠 근본 원인
1.  **빌드 결과물 위치**: 실행 파일이 소스 폴더가 아닌 `build/bin/Debug/` 서브 폴더에 생성됨.
2.  **PowerShell 규칙**: 현재 디렉토리에 파일이 있더라도 보안상 `.\` 접두어 없이 파일 이름만으로 실행하는 것을 금지함.

### ✅ 해결책
*   **전체 경로 명시 및 접두어 사용**:
    ```powershell
    .\build\bin\Debug\unit_tests.exe --gtest_filter="ImageProcessorTest.*"
    ```

---

## 🎯 교훈 및 요약
*   **Runtime Consistency**: Windows에서 `vcpkg`를 쓸 때는 가급적 `x64-windows-static`보다는 기본 동적 링크 설정을 따르는 것이 힙 주권 문제 해결에 유리함.
*   **Encoding Hygiene**: 한국어 주석을 사용하는 C++ 프로젝트에서는 반드시 `/utf-8` 플래그를 명시하여 컴파일러의 엉뚱한 파싱 에러를 차단해야 함.
*   **Execution Visibility**: 빌드 자동화 환경에서 실행 파일의 위치와 셸(Shell)별 실행 관례는 사전에 명확히 인지되어야 함.
