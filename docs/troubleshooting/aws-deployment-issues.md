# AWS 배포 트러블슈팅 로그

본 문서는 Mind Palette 프로젝트를 AWS에 배포하는 과정에서 발생했던 문제들과 해결 방법들을 기록한 로그 파일입니다. 유사한 상황이 발생했을 때 해결을 위한 참고 자료로 활용합니다.

## Issue 1: Let's Encrypt 인증서 컨테이너 마운트 시 심볼릭 링크 깨짐 문제

- **증상**: Nginx 컨테이너에서 `ssl_certificate` 경로를 찾지 못하여 Nginx가 기동에 실패하거나 HTTPS 통신이 되지 않음.
- **원인 분석**: 
  1. Certbot은 인증서를 `/etc/letsencrypt/archive/`에 저장하고, 실제 사용할 때는 `/etc/letsencrypt/live/[도메인]/` 폴더 아래 위치한 파일들로 심볼릭 링크(Symbolic Link)를 걸어서 제공함.
  2. `docker-compose.yml`에서 `- /etc/letsencrypt/live/mindpalette.kr/fullchain.pem:/etc/nginx/ssl/fullchain.pem:ro` 처럼 특정 파일(이 경우 원본 파일이 아닌 심볼릭 링크)만을 마운트하면 컨테이너 내부에는 심볼릭 링크가 가리키는 `archive/` 디렉토리가 없으므로 링크가 깨짐.
- **해결 방안**:
  `docker-compose.yml`에서 `/etc/letsencrypt` 디렉토리 전체를 볼륨으로 마운트하여 컨테이너 내부에 `live` 와 `archive` 디렉토리 모두를 복제.
  ```yaml
  # docker-compose.yml
  volumes:
    - /etc/letsencrypt:/etc/letsencrypt:ro
  ```
  ```nginx
  # nginx.conf
  ssl_certificate     /etc/letsencrypt/live/mindpalette.kr/fullchain.pem;
  ssl_certificate_key /etc/letsencrypt/live/mindpalette.kr/privkey.pem;
  ```

## Issue 2: Certbot Standalone 방식에서 80번 포트 충돌

- **증상**: `sudo certbot certonly --standalone -d ...` 명령어 실행 시 "Port 80 is in use" 또는 에러 메시지와 함께 인증서 발급이 실패함.
- **원인 분석**: Certbot의 standalone 모드는 도메인 소유를 확인하기 위해 자체적으로 아주 작은 웹 브라우저를 임시로 띄우며, 이때 `80` 포트를 사용함. 이미 Nginx 컨테이너가 80 포트를 점유하고 있으면 충돌이 발생함.
- **해결 방안**: 인증서를 발급 또는 갱신하는 시점에는 80번 포트를 비워주어야 함.
  ```bash
  # 발급 전 Nginx 중단
  sudo docker compose stop nginx
  
  # 발급/갱신 명령어 수행
  
  # 완료 후 Nginx 재가동
  sudo docker compose start nginx
  ```

## Issue 3: ADMIN_PROFILE_KEY Environment Variable Warning

- **증상**: `sudo docker compose up -d` 수행 시 `WARN[0000] The "ADMIN_PROFILE_KEY" variable is not set. Defaulting to a blank string.` 경고 발생.
- **원인 분석**: 서버에 배포된 `.env` 파일 안에 `ADMIN_PROFILE_KEY` 값이 누락되어 있음. 이 변수는 성능 테스트 시 사용되는 API 키임.
- **해결 방안**: 서버의 `.env` 파일에 `ADMIN_PROFILE_KEY={원하는 비밀키}` 항목을 추가하고 `sudo docker compose up -d` 재실행.

## Issue 4: Docker Compose Buildx Warning

- **증상**: `sudo docker compose up -d` 수행 시 `WARN[0000] Docker Compose is configured to build using Bake, but buildx isn't installed` 경고 발생.
- **원인 분석**: 최신 Docker Compose v2는 `buildx` 라는 고급 빌드 툴킷을 기본으로 사용하려고 하지만 Ubuntu에 해당 플러그인이 설치되어 있지 않음.
- **해결 방안**: 무시해도 레거시 방식 빌드가 적용되어 기능상 무방하나, 경고를 제거하고 빌드 성능을 올리려면 패키지 설치.
  ```bash
  sudo apt-get install docker-buildx-plugin
  ```
