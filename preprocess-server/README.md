# preprocess-server

Mind Palette의 **C++ 전처리 서버**(예정)입니다.

## 빌드 (Visual Studio / Windows)

### 1) 권장: vcpkg(manifest)로 의존성 자동 설치
- `preprocess-server/vcpkg.json`에 의존성이 정의되어 있습니다.
- `preprocess-server/vcpkg-configuration.json`에서 **vcpkg 레지스트리 baseline(커밋)** 을 고정하여 재현성을 확보합니다.
- Visual Studio에서 폴더를 열고 **CMake 구성/빌드**를 하면 vcpkg가 자동으로 설치를 진행합니다.

> 참고: **OpenCV는 아직 사용하지 않습니다.**
> 현재 단계(헬스 체크/라우팅 테스트)에서는 OpenCV가 필요 없어서 의존성에서 제외했습니다.
> 이미지 전처리 기능(TDD로 첫 전처리 테스트 추가) 단계에서 OpenCV를 다시 도입합니다.

## 테스트
- `unit_tests` 타겟을 빌드/실행하세요.

