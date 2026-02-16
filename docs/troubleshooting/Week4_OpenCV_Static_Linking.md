# Week 4 트러블슈팅: OpenCV 코덱 이슈 및 DLL 지옥 해결

## 1. 문제 현상 (Symptom)
- Windows 환경에서 `cv::imwrite`를 호출하여 JPG 이미지를 저장하려고 할 때 `cv::Exception` 발생 또는 파일 생성 실패.
- 실행 파일 폴더에 모든 OpenCV DLL(`opencv_world410.dll` 등)과 의존성 DLL(jpeg62, libpng 등)을 복사했음에도 불구하고, 런타임에 코덱 라이브러리를 로드하지 못하거나 `Access Violation` 발생.
- 특히 `cv::imread`는 성공하지만 `cv::imwrite`만 실패하는 현상으로 인해 초기 디버깅에 혼선 발생.

## 2. 문제 분석 (Root Cause Analysis)

### 원인 1: 동적 링크(Dynamic Linking)의 복잡한 의존성
- Windows용 OpenCV 동적 라이브러리는 내부적으로 수많은 3rd-party 코덱 라이브러리에 의존함.
- `vcpkg`를 통해 설치된 동적 라이브러리가 사용자 시스템의 다른 경로에 있는 DLL과 충돌하거나, 필수적인 구성 요소가 누락된 채로 배포됨.

### 원인 2: 런타임 환경과 DLL 로딩 순서
- 애플리케이션 실행 시 DLL을 찾는 순서(Search Path) 문제로 인해 잘못된 버전의 라이브러리가 로드되거나, 정적 초기화 시점에 코덱 등록이 누락됨.

## 3. 해결 전략 (Resolution Strategy)

### 1단계: 정적 링크(Static Linking)로의 전환 (핵심 해결책)
- 모든 의존성을 실행 파일(`preprocess_server.exe`) 하나에 포함시켜 DLL 로딩 문제를 원천적으로 차단.
- **vcpkg 설정**: `x64-windows-static` 트리플렛 사용.
  ```bash
  vcpkg install opencv4[core,imgcodecs,imgproc,jpeg,png]:x64-windows-static
  ```
- **CMake 설정**:
  - `VCPKG_TARGET_TRIPLET`을 `x64-windows-static`으로 명시.
  - MSVC 런타임 라이브러리를 **MultiThreaded (/MT)**로 설정하여 정적 라이브러리와 호환성 확보.
  ```cmake
  set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
  ```

### 2단계: `cv::imencode` 기반 바이너리 직접 쓰기
- 정적 빌드 환경에서도 `cv::imwrite`가 내부적으로 파일 시스템 핸들을 다룰 때 가끔 발생하는 불안정성 제거.
- 이미지를 메모리 버퍼(`std::vector<uchar>`)로 인코딩한 후, 표준 C++ `std::ofstream`을 사용하여 바이너리 파일로 직접 저장.
- 이 방식은 OpenCV의 내부 파일 I/O 로직에 의존하지 않으므로 훨씬 견고함.

```cpp
// 개선된 로직 (AtomicFileWriter)
std::vector<uchar> buffer;
if (cv::imencode(".jpg", image, buffer)) {
    std::ofstream ofs(tempPath, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}
```

### 3단계: PPM Fallback 메커니즘 구축
- 만약의 코덱 실패에 대비하여, 텍스트 기반 포맷인 **PPM(Portable Pixel Map)**으로 저장하는 Fallback 로직 추가.
- 어떤 극한 상황에서도 데이터 유실 없이 결과를 보존하는 강건성(Robustness) 확보.

## 4. 최종 결과 (Result)
1. **정상 동작 확인**: `IMG_1294.jpg`를 이용한 파이프라인 테스트에서 JPG 저장 성공.
2. **배포 간소화**: DLL 파일 없이 `.exe` 단일 파일만으로 실행 가능.
3. **성능**: DLL 로딩 오버헤드가 사라지고 바이너리 크기는 증가했으나 실행 안정성 극대화.

## 5. 교훈 및 시사점 (Lessons Learned)
- Windows 환경에서의 C++ 개발 시 복잡한 외부 라이브러리(OpenCV 등)는 **정적 라이브러리(Static Library)**를 사용하는 것이 유지보수와 안정성 면에서 압도적으로 유리함.
- `vcpkg`와 CMake의 트리플렛(`x64-windows-static`) 연동은 이러한 문제를 해결하는 표준적인 방법임.
- 파일 쓰기 실패와 같은 치명적 오류에 대비한 **Fallback 전략**은 마이크로서비스의 신뢰도를 높이는 핵심 요소임.

---

# Week 4.5: Linux(WSL2) 빌드 및 배포 트러블슈팅

## 1. 문제 현상 (Symptom)
- `vcpkg install` 실패: `egl-registry`, `gettext` 등 기초 라이브러리 빌드 중 에러 발생.
- `CMake` 구성 실패: `ninja`, `g++` 등 빌드 도구를 찾지 못함.
- **Unit Test 실패**: `ValidatePreprocessRequestTest`가 윈도우 경로 하드코딩으로 인해 리눅스에서 실패.

## 2. 원인 분석 (Root Cause Analysis)

### 원인 1: WSL 파일 시스템 성능 및 권한 이슈 (/mnt/c)
- Windows 드라이브(`/mnt/c/...`)에서 리눅스 빌드를 수행할 경우, 파일 I/O 성능이 극도로 저하되고 파일 락(Lock) 문제가 발생하여 `vcpkg` 빌드가 실패함.
- **해결**: 프로젝트를 리눅스 네이티브 파일 시스템(`~/home/user/project`)으로 복사하여 빌드 수행.

### 원인 2: 필수 시스템 빌드 도구 누락
- `vcpkg`가 의존하는 `bison`, `flex`, `autoconf`, `libtool` 등이 기본 Ubuntu 이미지에 포함되지 않음.
- **해결**: `apt-get install`을 통해 해당 도구 및 OpenCV용 X11 헤더 라이브러리 추가 설치.

### 원인 3: Vcpkg Manifest (GUI 의존성)
- 기본 `opencv` 포트는 GTK, OpenGL 등 무거운 GUI 라이브러리를 포함함. 서버 환경에서는 불필요하며 빌드 실패의 주요 원인이 됨.
- **해결**: `vcpkg.json`에서 `default-features: false`로 설정하고 `headless` 모드(jpeg, png, thread 등 필수 기능만 포함)로 최적화.

### 원인 4: 테스트 코드의 OS 종속성
- `tests/test_main.cpp`에 `C:\\Users\\...` 형태의 윈도우 절대 경로가 하드코딩되어 있음.
- **해결**: `std::filesystem`과 `std::ofstream`을 사용하여 **임시 파일을 동적으로 생성**하고 그 경로를 검증하는 방식으로 수정하여 OS 독립성 확보.

## 3. 최종 결과 (Result)
- **WSL2(Ubuntu 22.04) 빌드 성공**: 모든 의존성이 정상적으로 설치되고 빌드 완료.
- **Cross-Platform Test 통과**: Windows와 Linux 양쪽에서 57개 유닛 테스트 모두 통과 (100% Passed).
- **자동화 스크립트 확보**: `verify_linux.sh` 하나로 의존성 설치부터 빌드, 테스트까지 원스톱 검증 가능.

