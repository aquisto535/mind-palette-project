# Mind Palette AWS 배포 가이드

본 문서는 Mind Palette 프로젝트를 AWS EC2(Ubuntu) 환경에 배포하기 위한 전체 과정을 처음부터 끝까지 정리한 가이드입니다. 

## 1. 사전 준비 (Prerequisites)

- **AWS EC2 인스턴스**: Ubuntu 24.04 LTS (c5.large 권장)
- **도메인**: `mindpalette.kr` (Cafe24 등에서 구매 및 관리)
- **보안 그룹 (Inbound Rules)**:
  - `22` (SSH) - 관리자 IP
  - `80` (HTTP) - Anywhere (0.0.0.0/0)
  - `443` (HTTPS) - Anywhere (0.0.0.0/0)
  - `8081`, `8082`, `3000` 등 내부 포트는 로컬 호스트 테스트가 필요하지 않은 이상 외부로 개방하지 마세요. (Nginx가 프록시 역할을 수행합니다)

## 2. 도메인 연결 (DNS 설정)

1. Cafe24(또는 DNS 제공자) 로그인
2. **도메인 관리 > DNS 관리** 이동
3. `A 레코드` 추가 
   - 호스트: `@` (비워둠), 값: `EC2 탄력적 IP(Elastic IP)`
   - 호스트: `www`, 값: `EC2 탄력적 IP(Elastic IP)`

## 3. 서버 초기 환경 설정

EC2 인스턴스에 SSH로 접속한 뒤, 필수 패키지들을 설치합니다.

```bash
# 시스템 업데이트
sudo apt-get update && sudo apt-get upgrade -y

# Docker 설치
sudo apt-get install -y apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce docker-compose-plugin docker-buildx-plugin

# Git 및 Certbot 설치
sudo apt-get install -y git certbot
```

## 4. 프로젝트 클론 및 환경 변수 설정

GitHub에서 프로젝트 코드를 받아오고, 환경 변수(`.env`)를 구성합니다.

```bash
# 1. SSH 키 생성 및 GitHub 등록 (필요시)
ssh-keygen -t rsa -b 4096
# 공개키(~/.ssh/id_rsa.pub)를 Github Deploy Keys에 등록

# 2. 레포지토리 클론
git clone git@github.com:aquisto535/mind-palette-project.git
cd mind-palette-project

# 3. 환경 변수 파일 생성
cp .env.example .env
nano .env
```

`.env` 파일에 필요한 변수 지정 (특히 `ADMIN_PROFILE_KEY` 등 누락 시 경고 발생).

## 5. SSL 인증서 발급 (Certbot Standalone)

HTTPS 통신을 위한 Let's Encrypt 인증서를 발급받습니다.
**주의**: Standalone 모드는 80번 포트를 일시적으로 사용하므로 80번 포트가 비어 있어야 합니다.

```bash
# Nginx 컨테이너가 실행 중이라면 중단
sudo docker compose stop nginx 

# 인증서 발급
sudo certbot certonly --standalone -d mindpalette.kr -d www.mindpalette.kr -m [본인이메일주소] --agree-tos
```

성공 시 `/etc/letsencrypt/live/mindpalette.kr/` 경로에 키 파일들이 생성됩니다. 

## 6. 컨테이너 빌드 및 실행

모든 준비가 완료되었으므로, Docker Compose를 이용해 백그라운드에서 서비스를 빌드하고 띄웁니다.

```bash
sudo docker compose up -d --build
```

**상태 확인**:
```bash
sudo docker compose ps
sudo docker compose logs -f
```

## 7. 업데이트 및 자동 배포 (수동)

로컬에서 코드가 업데이트되었을 때 서버에 반영하는 수동 명령어입니다. (추후 GitHub Actions로 자동화 가능)

```bash
cd ~/mind-palette-project
git pull origin main
sudo docker compose up -d --build
```

## 8. 인증서 자동 갱신

Certbot 설치 시 시스템 타이머가 자동으로 등록되어 만료 30일 전 자동으로 갱신을 시도합니다. 단, Standalone 모드로 발급받은 경우 자동 갱신 시점에 Nginx와의 80번 포트 충돌이 발생할 수 있으므로 갱신 훅(Hook)을 추가해야 합니다.

```bash
# 갱신 전 Nginx 컨테이너를 내리고, 갱신 후 Nginx 컨테이너를 올립니다.
sudo certbot renew --pre-hook "docker-compose -f /home/ubuntu/mind-palette-project/docker-compose.yml stop nginx" --post-hook "docker-compose -f /home/ubuntu/mind-palette-project/docker-compose.yml start nginx"
```
