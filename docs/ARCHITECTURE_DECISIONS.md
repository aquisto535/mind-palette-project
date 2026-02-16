# 🏛️ Architecture Decision Records (ADR)

> **목적**: 이 문서는 Mind Palette 프로젝트에서 내린 **기술적 의사결정의 배경과 근거**를 기록합니다.  
> 면접 시 "왜 이 기술을 선택했는가?"라는 질문에 명확하고 논리적인 답변을 제공하기 위한 문서입니다.

---

## 📋 의사결정 목록 (Decision Index)

| ID | 제목 | 날짜 | 상태 |
|----|------|------|------|
| ADR-001 | 마이크로서비스 아키텍처 채택 (Node.js + C++ + Python) | 2025-11 | ✅ Accepted |
| ADR-002 | Kubernetes 대신 Docker Compose 선택 | 2025-11 | ✅ Accepted |
| ADR-003 | 내부 통신은 HTTP, 외부 통신은 HTTPS | 2026-01 | ✅ Accepted |
| ADR-004 | Python AI 서버 프레임워크로 FastAPI 채택 | 2026-01 | ✅ Accepted |
| ADR-005 | C++ 전처리 서버 프레임워크로 Crow 채택 | 2025-11 | ✅ Accepted |
| ADR-006 | Atomic Write 패턴으로 파일 무결성 보장 | 2026-01 | ✅ Accepted |
| ADR-007 | TDD 및 Tidy First 방법론 전면 도입 | 2025-11 | ✅ Accepted |
| ADR-008 | gRPC 대신 HTTP REST API 사용 | 2025-11 | ✅ Accepted |
| ADR-009 | AI 프레임워크로 PyTorch 채택 | 2025-11 | ✅ Accepted |
| ADR-010 | Transfer Learning 모델로 EfficientNet-B2 채택 | 2026-02 | ✅ Accepted |
| ADR-011 | C++ 전처리 파이프라인 결과물 명세 | 2026-02 | ✅ Accepted |
| ADR-012 | C++ 전처리 서버 개발 로드맵 (Week 2-4) | 2026-02 | ✅ Accepted |
| ADR-013 | Git Workflow로 GitHub Flow (Feature Branch + PR) 채택 | 2026-02 | ✅ Accepted |
| ADR-014 | AWS EC2와 Docker 연결 구조 (Port Mapping & Bridge Network) | 2026-02 | ✅ Accepted |
| ADR-015 | 하이브리드 배포 전략 (Local GPU 개발 vs EC2 CPU 배포) | 2026-02 | ✅ Accepted |
---

## ADR-001: 마이크로서비스 아키텍처 채택 (Node.js + C++ + Python)

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
아동 인물화 지능 측정 시스템은 다음의 이질적인 요구사항을 충족해야 합니다:
- **빠른 파일 업로드/다운로드 처리** (I/O 집약적)
- **고성능 이미지 전처리** (OpenCV, CPU 멀티스레딩)
- **딥러닝 추론** (PyTorch, GPU 가속)

### 의사결정 (Decision)
단일 모놀리식 서버가 아닌, **3개의 독립적인 마이크로서비스로 아키텍처를 분리**합니다:
1. **Node.js API Gateway**: 사용자 요청 처리 및 서비스 오케스트레이션
2. **C++ Preprocessing Server**: OpenCV 기반 고성능 이미지 전처리
3. **Python AI Server**: PyTorch 기반 딥러닝 추론

### 근거 (Rationale)

#### 1️⃣ 언어별 최적 도메인 활용
- **Node.js**: 비동기 I/O에 최적화되어 파일 업로드/다운로드를 효율적으로 처리.
- **C++**: 메모리 제어와 멀티스레딩으로 OpenCV 연산을 2~3배 가속.
- **Python**: ML/AI 생태계(PyTorch, NumPy)의 표준 언어로 빠른 모델 개발 가능.

#### 2️⃣ 장애 격리 (Fault Isolation)
- C++ 서버에서 메모리 누수가 발생해도 Python AI 서버는 영향을 받지 않음.
- 각 서비스를 독립적으로 재시작/배포 가능.

#### 3️⃣ 독립적 확장 (Independent Scaling)
- AI 서버만 GPU 인스턴스로 확장 가능 (Node.js와 C++은 CPU 인스턴스 유지).
- 트래픽이 급증하면 API Gateway만 수평 확장 가능.

#### 4️⃣ 기술 스택 학습 시너지
- 각 언어의 **"왜 이 언어를 쓰는가?"**에 대한 명확한 답변을 면접에서 제시 가능.
- 단일 언어 프로젝트보다 풍부한 기술 스택 경험.

### 대안 및 트레이드오프 (Alternatives)
- **모놀리식 Python 서버 (Flask/FastAPI)**: 전처리도 Python(PIL/OpenCV-Python)으로 처리
  - ❌ **Rejected**: 이미지 처리 성능이 C++ 대비 현저히 낮음. GIL(Global Interpreter Lock)로 인한 멀티스레딩 비효율.
- **서버리스 아키텍처 (AWS Lambda)**: 각 단계를 Lambda 함수로 분리
  - ❌ **Rejected**: Cold Start 지연 시간, 로컬 개발 복잡도 증가, 비용 예측 어려움.

### 결론 (Consequences)
- ✅ **장점**: 최적의 성능, 명확한 책임 분리, 독립적 확장.
- ⚠️ **단점**: 네트워크 통신 오버헤드 증가, 분산 시스템 디버깅 복잡도.
- **완화 전략**: Docker Compose로 로컬 개발 환경 일관성 확보, Shared Volume으로 파일 경로 기반 통신.

---

## ADR-002: Kubernetes 대신 Docker Compose 선택

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
마이크로서비스 아키텍처를 채택하면서 **컨테이너 오케스트레이션 도구**를 선택해야 했습니다.

### 의사결정 (Decision)
**Docker Compose**를 선택하고, Kubernetes는 도입하지 않습니다.

### 근거 (Rationale)

#### 1️⃣ 프로젝트 규모에 적합한 도구 선택
- **예상 사용자**: 초기 MVP는 동시 접속 50명 이하.
- **서버 규모**: 단일 서버(AWS EC2 하나 또는 로컬 머신)에서 충분히 운영 가능.
- **Kubernetes는 Over-Engineering**: 수십 대의 서버를 관리하는 환경을 위한 도구이므로, 이 프로젝트에는 불필요한 복잡도만 추가.

#### 2️⃣ 개발 생산성 극대화
```bash
# Docker Compose: 한 줄로 전체 시스템 실행
docker-compose up -d

# Kubernetes: 수십 개의 YAML 파일 작성 필요
kubectl apply -f namespace.yaml
kubectl apply -f deployment.yaml
kubectl apply -f service.yaml
kubectl apply -f ingress.yaml
kubectl apply -f configmap.yaml
```
- Docker Compose는 `docker-compose.yml` 단일 파일로 관리 가능.
- 로컬 개발 환경과 프로덕션 환경 간 차이 최소화.

#### 3️⃣ 학습 비용 대비 효용
- Kubernetes 학습 시간: 2~4주 (ConfigMap, Service Mesh, Ingress 등).
- 그 시간에 **AI 모델 정확도 향상** 또는 **UX 개선**에 투자하는 것이 더 가치 있음.

#### 4️⃣ 비용 효율성
- Kubernetes 클러스터 최소 구성 비용: 월 $100~$200 (Control Plane + Worker Nodes).
- Docker Compose on EC2: 월 $20~$50 (단일 인스턴스).

### 대안 및 트레이드오프 (Alternatives)
- **Kubernetes**: 자동 스케일링, 롤링 업데이트, 서비스 메시 등 고급 기능 제공.
  - ❌ **Rejected**: 현재 프로젝트 규모에서는 이러한 기능이 불필요하며, 관리 오버헤드가 이득보다 큼.

### 결론 (Consequences)
- ✅ **장점**: 빠른 개발 속도, 낮은 러닝 커브, 비용 절감, 로컬 환경 재현 용이.
- ⚠️ **단점**: 수평 확장 시 수동 작업 필요 (향후 사용자 급증 시 Kubernetes로 마이그레이션 고려).
- **포트폴리오 가치**: "프로젝트 규모에 맞는 적절한 도구를 선택하는 엔지니어링 판단력"을 증명.

---

## ADR-003: 내부 통신은 HTTP, 외부 통신은 HTTPS

### 상태
✅ **Accepted** (2026-01)

### 컨텍스트 (Context)
프론트엔드(React)는 Netlify에서 HTTPS로 배포되며, 백엔드(API Gateway, C++, Python)는 AWS EC2에 배포될 예정입니다.

### 의사결정 (Decision)
**Mixed Security Architecture**를 채택합니다:
1. **Frontend ↔ API Gateway**: HTTPS (Nginx Reverse Proxy + Let's Encrypt SSL)
2. **API Gateway ↔ C++ ↔ Python**: HTTP (Docker 내부 네트워크, Plain Text)

### 근거 (Rationale)

#### 1️⃣ 보안과 성능의 균형
| 구간 | 프로토콜 | 이유 |
|------|----------|------|
| **Frontend ↔ Gateway** | HTTPS | • 공개 인터넷을 통한 통신<br>• Mixed Content 경고 방지<br>• 사용자 이미지 암호화 필수 |
| **Gateway ↔ C++ ↔ Python** | HTTP | • Docker 내부 격리 네트워크 (외부 접근 불가)<br>• SSL 핸드셰이크 오버헤드 제거 (10~50ms 절약)<br>• 인증서 관리 복잡도 제거 |

#### 2️⃣ AWS VPC/Docker Network의 신뢰성
- Docker Compose의 내부 네트워크(`bridge` 모드)는 외부에서 접근 불가.
- AWS Security Group으로 EC2 인스턴스의 내부 포트(8081, 8082)는 외부 차단.
- **결론**: 내부 통신은 "신뢰할 수 있는 폐쇄망"으로 간주 가능.

#### 3️⃣ 산업 표준 사례
- **Netflix**: 마이크로서비스 간 통신은 내부망 HTTP 사용 (Service Mesh는 관찰성 목적).
- **Uber**: API Gateway 뒤의 내부 서비스는 비암호화 gRPC 사용.

#### 4️⃣ Over-Engineering 방지
- 내부 통신까지 HTTPS를 적용하면:
  - 각 서비스마다 인증서 생성/갱신 필요 (Let's Encrypt 90일마다).
  - Nginx 설정 복잡도 증가.
  - **효과**: 보안은 향상되지 않으면서 관리 비용만 증가.

### 대안 및 트레이드오프 (Alternatives)
- **전 구간 HTTPS**: 내부 통신까지 SSL 암호화.
  - ❌ **Rejected**: Docker 내부망은 이미 격리되어 있으므로, 성능 손실만 발생하고 보안 이득 없음.
- **전 구간 HTTP**: API Gateway도 HTTP로 노출.
  - ❌ **Rejected**: Mixed Content 경고 발생, 사용자 데이터 암호화되지 않음.

### 결론 (Consequences)
- ✅ **장점**: 보안 요구사항 충족, 내부 통신 성능 최적화, 관리 복잡도 최소화.
- ⚠️ **주의사항**: AWS Security Group 설정 철저히 검증 (내부 포트 외부 차단).
- **배포 구성**:
  ```yaml
  # docker-compose.yml
  services:
    api-gateway:
      ports:
        - "3000:3000"  # Nginx가 리버스 프록시로 HTTPS → HTTP 변환
    cpp-server:
      expose:
        - "8081"  # Docker 내부망에만 노출 (외부 접근 불가)
    python-server:
      expose:
        - "8082"  # Docker 내부망에만 노출 (외부 접근 불가)
  ```

---

## ADR-004: Python AI 서버 프레임워크로 FastAPI 채택

### 상태
✅ **Accepted** (2026-01)

### 컨텍스트 (Context)
Python AI 서버를 구축하기 위해 웹 프레임워크를 선택해야 했습니다.  
주요 후보: **Flask**, **FastAPI**, **Django REST Framework**

### 의사결정 (Decision)
**FastAPI**를 선택합니다.

### 근거 (Rationale)

#### 1️⃣ 비동기 처리 성능 (ASGI)
```python
# Flask (WSGI - 동기)
@app.route('/analyze', methods=['POST'])
def analyze():
    result = model.predict(image)  # 추론 중에는 다른 요청 처리 불가
    return jsonify(result)

# FastAPI (ASGI - 비동기)
@app.post("/analyze")
async def analyze(image: UploadFile):
    result = await model.predict(image)  # 추론 중에도 다른 요청 처리 가능
    return result
```
- **Flask는 WSGI(Web Server Gateway Interface)** 기반으로 동기 처리만 가능 → 추론 중 블로킹.
- **FastAPI는 ASGI(Asynchronous Server Gateway Interface)** 기반 → 동시 요청 처리 효율 5~10배 향상.

#### 2️⃣ 자동 문서화 (Swagger UI / ReDoc)
- **FastAPI**: `http://localhost:8000/docs`로 자동 생성된 API 문서 제공.
- **Flask**: Swagger 문서를 수동으로 작성해야 함 (Flask-RESTX 등 확장 필요).
- **포트폴리오 가치**: 면접관에게 API 문서를 즉시 보여줄 수 있음.

#### 3️⃣ 타입 안전성 (Pydantic)
```python
from pydantic import BaseModel

class AnalysisRequest(BaseModel):
    image_path: str
    confidence_threshold: float = 0.8  # 기본값 설정

@app.post("/analyze")
def analyze(request: AnalysisRequest):
    # 타입 검증 자동 수행, 잘못된 입력 시 422 에러 자동 반환
    pass
```
- 잘못된 데이터 타입이 들어오면 **자동으로 400/422 에러 반환** (수동 검증 코드 불필요).
- Python의 타입 힌트(Type Hints)를 런타임에서 강제하여 버그 사전 차단.

#### 4️⃣ 현대적 생태계 트렌드
- **2024~2026년 가장 빠르게 성장하는 Python 웹 프레임워크** (GitHub Stars 70k+).
- **채용 시장 수요**: FastAPI 경험은 AI/MLOps 엔지니어 채용 공고에서 빈번히 언급됨.

#### 5️⃣ 학습 비용 대비 효용
- **Django REST Framework**: 기능은 풍부하지만 ORM, Admin 등 불필요한 기능 많음 → 학습 시간 과다.
- **Flask**: 가볍지만 비동기 처리, 문서화, 검증 등을 모두 수동 구현해야 함.
- **FastAPI**: 필수 기능만 학습하면 즉시 프로덕션 수준 API 구축 가능 (학습 시간 1~2주).

### 대안 및 트레이드오프 (Alternatives)
- **Flask**: 가장 널리 알려진 프레임워크, 단순한 구조.
  - ❌ **Rejected**: 비동기 처리 불가, 수동 검증 필요, 문서화 부족 → AI 서빙에 비효율적.
- **Django REST Framework**: 대규모 애플리케이션에 적합.
  - ❌ **Rejected**: 학습 비용 높음, 이 프로젝트에는 과도한 기능 (ORM, Admin Panel 불필요).

### 결론 (Consequences)
- ✅ **장점**: 빠른 개발 속도, 자동 문서화, 타입 안전성, 비동기 성능, 채용 시장 수요 부합.
- ⚠️ **단점**: 상대적으로 신생 프레임워크 (하지만 빠르게 안정화되고 있음).
- **학습 자료**: [FastAPI 공식 문서](https://fastapi.tiangolo.com/) (한국어 번역 완료).

---

## ADR-005: C++ 전처리 서버 프레임워크로 Crow 채택

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
C++ 전처리 서버를 HTTP REST API로 노출하기 위해 웹 프레임워크를 선택해야 했습니다.  
주요 후보: **Crow**, **cpp-httplib**, **Pistache**, **Boost.Beast**

### 의사결정 (Decision)
**Crow**를 선택합니다.

### 근거 (Rationale)

#### 1️⃣ Express.js 스타일의 직관적 API
```cpp
#include "crow_all.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/preprocess").methods("POST"_method)
    ([](const crow::request& req){
        // 이미지 전처리 로직
        return crow::response(200, "OK");
    });

    app.port(8081).multithreaded().run();
}
```
- Node.js 경험을 그대로 C++로 전환 가능 → 학습 곡선 완화.

#### 2️⃣ 성능과 복잡도의 균형
- **Boost.Beast**: 최고 성능이지만 코드 작성 복잡도 높음 (100줄 → 500줄).
- **cpp-httplib**: 헤더 온리 라이브러리로 간단하지만, Crow보다 기능 부족 (라우팅, 미들웨어 등).
- **Crow**: Boost.Asio 기반 비동기 성능 + Express.js 수준 간결함.

#### 3️⃣ 멀티스레딩 지원
```cpp
app.port(8081).multithreaded().run();  // 스레드 풀 자동 생성
```
- CPU 멀티코어 활용으로 이미지 배치 처리 성능 극대화.
- OpenCV 연산과 네트워킹 처리를 병렬화.

#### 4️⃣ MFC 경험의 현대화
- 기존 MFC 프로젝트 경험을 **현대 C++ 네트워킹 패턴**으로 확장.
- 포트폴리오에서 "레거시 기술(MFC)을 현대 기술(Crow, OpenCV)로 마이그레이션하는 능력" 어필 가능.

### 대안 및 트레이드오프 (Alternatives)
- **Mongoose (C 라이브러리)**: 임베디드 친화적이지만 REST API 지원 약함.
  - ❌ **Rejected**: 라우팅 기능 부족, C++ STL 사용 불가.
- **Boost.Beast**: 최고 성능, 표준 라이브러리.
  - ❌ **Rejected**: 코드 복잡도가 프로젝트 규모 대비 과도함.

### 결론 (Consequences)
- ✅ **장점**: 빠른 개발 속도, Express.js 스타일, 멀티스레딩 지원, MFC 경험 현대화.
- ⚠️ **단점**: Boost.Beast 대비 벤치마크 성능은 약간 낮음 (하지만 프로젝트에서 체감 불가).

---

## ADR-006: Atomic Write 패턴으로 파일 무결성 보장

### 상태
✅ **Accepted** (2026-01)

### 컨텍스트 (Context)
C++ 전처리 서버가 결과 파일을 저장하는 동안, Python AI 서버가 동시에 해당 파일을 읽으려 할 때 **Race Condition(경쟁 상태)** 발생 위험이 있습니다.

```
[C++ 서버]                    [Python 서버]
파일 쓰기 시작 (50%)
                              파일 읽기 시도 → ❌ 불완전한 데이터 읽음
파일 쓰기 완료 (100%)
```

### 의사결정 (Decision)
**Atomic Write 패턴**을 구현합니다:
1. 임시 파일(`.tmp`)에 쓰기.
2. 쓰기 완료 후 `rename()`으로 원자적으로 파일명 변경.

### 근거 (Rationale)

#### 1️⃣ 파일 시스템의 원자적 연산 활용
```cpp
// C++ 전처리 서버
std::string temp_path = result_path + ".tmp";
std::string final_path = result_path;

// 1. 임시 파일에 쓰기
cv::imwrite(temp_path, processed_image);

// 2. 원자적 이동 (Linux/Windows 모두 지원)
std::filesystem::rename(temp_path, final_path);
```
- `rename()` 시스템 콜은 **원자적(Atomic) 연산**이므로, 파일이 "절반만 존재"하는 상태가 불가능.

#### 2️⃣ 데이터 무결성 보장
- Python 서버는 `final_path`가 존재하면 **항상 완전한 데이터**를 읽음.
- "파일 손상" 또는 "부분 데이터" 문제 원천 차단.

#### 3️⃣ 복잡한 락(Lock) 메커니즘 불필요
- 대안: 파일 락(File Locking), 세마포어(Semaphore).
  - ❌ **Rejected**: 구현 복잡도 높음, 데드락 위험, 크로스 플랫폼 이슈.
- Atomic Write는 **추가 동기화 코드 없이 안전성 보장**.

#### 4️⃣ 산업 표준 패턴
- **Git**: 객체 저장 시 `.tmp` → `rename` 방식 사용.
- **PostgreSQL**: WAL(Write-Ahead Log) 파일 쓰기 시 동일 패턴 적용.

### 대안 및 트레이드오프 (Alternatives)
- **File Locking**: `flock()` 또는 `LockFile()` 사용.
  - ❌ **Rejected**: Windows/Linux 간 API 차이, 데드락 위험.
- **Message Queue**: Redis/RabbitMQ로 작업 완료 신호 전송.
  - ❌ **Rejected**: 외부 의존성 추가, 과도한 인프라 복잡도.

### 결론 (Consequences)
- ✅ **장점**: 간단하고 안전한 구현, 외부 의존성 없음, 크로스 플랫폼 호환.
- ⚠️ **주의사항**: 임시 파일 정리 로직 필요 (장애 시 `.tmp` 파일 남을 수 있음).

---

## ADR-007: TDD 및 Tidy First 방법론 전면 도입

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
프로젝트가 10개월간 진행되며, 코드베이스가 커질수록 **기술 부채(Technical Debt)** 누적 위험이 증가합니다.

### 의사결정 (Decision)
**TDD(Test-Driven Development)**와 **Tidy First(Kent Beck)** 방법론을 프로젝트 전반에 적용합니다.

### 근거 (Rationale)

#### 1️⃣ 회귀 버그(Regression Bug) 방지
- 새로운 기능 추가 시 기존 기능이 깨지는 것을 자동 테스트로 즉시 탐지.
- **CI/CD 파이프라인에서 테스트 실패 시 배포 차단** → 프로덕션 장애 사전 차단.

#### 2️⃣ 리팩터링 안전성 보장
```
[Tidy First 원칙]
1. 구조 개선 (Structural Change) → 커밋
2. 기능 추가 (Behavioral Change) → 커밋

❌ 금지: 구조 개선 + 기능 추가를 동시에 커밋
```
- 테스트가 있으면 리팩터링 후 "코드가 여전히 동작하는지" 즉시 검증 가능.

#### 3️⃣ 채용 시장 가치
- **TDD 경험**은 시니어 엔지니어의 필수 역량으로 간주됨.
- 면접에서 "어떻게 코드 품질을 유지하는가?"라는 질문에 명확한 답변 가능.

#### 4️⃣ 자기 문서화(Self-Documenting)
```javascript
// 테스트 코드가 곧 명세서
describe('POST /analyze', () => {
  it('should return 400 if no image is uploaded', async () => {
    const response = await request(app).post('/analyze');
    expect(response.status).toBe(400);
  });
});
```
- 새로운 팀원이 프로젝트에 합류해도 테스트를 읽으면 API 동작 방식을 즉시 이해 가능.

### 대안 및 트레이드오프 (Alternatives)
- **테스트 없이 빠른 개발**: 초기 속도는 빠르지만 장기적으로 기술 부채 누적.
  - ❌ **Rejected**: 10개월 프로젝트에서 중반 이후 "레거시 코드" 상태가 되어 유지보수 불가능해짐.

### 결론 (Consequences)
- ✅ **장점**: 안정적인 코드베이스, 리팩터링 안전성, 채용 시장 가치, 자기 문서화.
- ⚠️ **단점**: 초기 개발 속도 10~20% 감소 (하지만 중장기적으로는 오히려 빠름).
- **적용 도구**:
  - Frontend/Node.js: Jest, Supertest
  - Python: PyTest
  - C++: GoogleTest (GTest)

---

## ADR-008: gRPC 대신 HTTP REST API 사용

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
마이크로서비스 간 통신 프로토콜을 선택해야 했습니다.  
주요 후보: **HTTP REST**, **gRPC**, **GraphQL**

### 의사결정 (Decision)
**HTTP REST API**를 선택합니다.

### 근거 (Rationale)

#### 1️⃣ 개발 생산성 우선
| 항목 | HTTP REST | gRPC |
|------|-----------|------|
| **학습 곡선** | 낮음 (이미 익숙) | 높음 (Protobuf, 코드 생성) |
| **디버깅** | 브라우저/Postman으로 즉시 테스트 | 전용 클라이언트(grpcurl) 필요 |
| **문서화** | Swagger/OpenAPI 자동 생성 | Protobuf 정의 수동 관리 |

- gRPC 학습 시간: 1~2주 (Protobuf 문법, 코드 생성, 에러 핸들링).
- 그 시간에 **AI 모델 정확도 향상**에 집중하는 것이 더 가치 있음.

#### 2️⃣ 프로젝트 규모에 맞는 선택
- **gRPC의 장점**: 바이너리 프로토콜로 HTTP보다 20~30% 빠름, 스트리밍 지원.
- **현실**: 이 프로젝트의 병목은 **AI 추론(2초)**이지, 네트워크 통신(10ms)이 아님.
- **결론**: 20ms를 10ms로 줄이는 것보다, 2초를 1초로 줄이는 것(ONNX Runtime)이 우선.

#### 3️⃣ 범용성 및 호환성
- HTTP REST는 모든 언어/플랫폼에서 표준 지원.
- 향후 Flutter 모바일 앱 추가 시, gRPC는 추가 설정 필요하지만 HTTP는 즉시 호환.

#### 4️⃣ 인프라 단순성
- gRPC는 HTTP/2 기반이므로, 일부 로드 밸런서/프록시에서 추가 설정 필요.
- HTTP REST는 Nginx, AWS ALB 등 모든 인프라에서 즉시 동작.

### 대안 및 트레이드오프 (Alternatives)
- **gRPC**: 고성능 RPC, 스트리밍 지원.
  - ❌ **Rejected**: 학습 비용 대비 실질적 성능 이득이 미미함 (병목이 네트워크가 아님).
- **GraphQL**: 클라이언트가 필요한 데이터만 요청 가능.
  - ❌ **Rejected**: 이 프로젝트는 이미지 분석이라는 단일 유스케이스만 존재하므로 오버헤드.

### 결론 (Consequences)
- ✅ **장점**: 빠른 개발 속도, 낮은 학습 곡선, 범용성, 디버깅 용이.
- ⚠️ **단점**: gRPC 대비 약간의 네트워크 오버헤드 (하지만 체감 불가).
- **향후 확장성**: 실제로 네트워크 병목이 발생하면 그때 gRPC로 전환 가능 (API Gateway 패턴으로 클라이언트 영향 최소화).

---

## ADR-009: AI 프레임워크로 PyTorch 채택

### 상태
✅ **Accepted** (2025-11)

### 컨텍스트 (Context)
딥러닝 기반 이미지 분석 모델을 구축하기 위해 AI 프레임워크를 선택해야 했습니다.
이 문제의 본질은 **"방대한 행렬 연산과 미분 계산을 인간이 얼마나 편하게 작성하고, 기계(GPU)가 얼마나 빠르게 실행하게 할 것인가"**를 결정하는 것입니다.

주요 후보: **PyTorch**, **TensorFlow**, **JAX**, **ONNX Runtime**

#### 프레임워크별 철학 및 비유 (제1원칙 관점)
| 프레임워크 | 비유 (Analogy) | 철학 (Philosophy) | 작동 방식 |
|:---:|:---:|---|---|
| **PyTorch** | **오픈 키친의 셰프** 👨‍🍳 | *"요리는 하면서 맛을 보는 것이다."* <br> (직관과 자유) | **Dynamic Graph** <br> 코드 한 줄마다 즉시 실행. 만들고 부수기 쉬움. |
| **TensorFlow** | **식품 공장** 🏭 | *"요리는 완벽한 설계도 후에 대량 생산하는 것이다."* <br> (규격과 양산) | **Static/Eager** <br> 파이프라인 설계 후 실행. 안정성과 배포 중시. |
| **JAX** | **분자 요리 연구소** 🧪 | *"요리는 화학 반응식의 최적화다."* <br> (순수 수학과 속도) | **Function Transformation** <br> 코드를 수학 공식으로 변환하여 TPU에서 극한 가속. |
| **ONNX Runtime** | **배달 튜닝 오토바이** 🛵 | *"요리는 안 한다. 식지 않게 배달만 한다."* <br> (오직 실행) | **Inference Optimizer** <br> 학습 기능 없이 이미 만들어진 모델의 실행 속도만 최적화. |

#### 기술적 비교표 (2026년 기준)
| 프레임워크 | 개발사 | 계산 그래프 | 학습 곡선 | 성능 (10B 미만) | 배포 생태계 | 주요 용도 |
|---------|-------|----------|---------|--------------|----------|---------|
| **PyTorch** | Meta | 동적 (Eager) | ⭐⭐⭐⭐⭐ (쉬움) | ✅ 최고 (torch.compile) | 성장 중 | 연구, 프로토타입 |
| **TensorFlow** | Google | 정적+동적 | ⭐⭐⭐ (보통) | 높음 | ⭐⭐⭐⭐⭐ (성숙) | 프로덕션, 모바일 |
| **JAX** | Google Research | 함수형 (JIT) | ⭐⭐ (어려움) | 최고 (10B+, TPU) | 제한적 | 고성능 연구 |

**숨겨진 제약 조건 (Hidden Constraints):**
1.  **인적 자원 비용**: TensorFlow(레거시 유지보수 어려움)나 JAX(전문가 부족)에 비해 PyTorch는 인력 확보가 가장 용이함.
2.  **하드웨어 종속성**: JAX는 TPU 환경에서 최대 성능을 발휘하며, 일반 GPU 환경에서는 설정이 까다로울 수 있음.
3.  **학습과 배포의 분리**: 학습 도구(PyTorch)와 배포 도구(ONNX Runtime)를 분리하는 것이 현재 업계의 **"승리하는 공식(Winning Standard)"**.

### 의사결정 (Decision)
**PyTorch 2.0+**를 선택하여 모델을 개발하고, 프로덕션 배포 시에는 **ONNX Runtime**으로 변환합니다.

### 근거 (Rationale)

#### 1️⃣ 연구 친화적 개발 환경 (Research-Friendly)
```python
# PyTorch: Pythonic하고 직관적
model = nn.Sequential(
    nn.Conv2d(3, 64, 3),
    nn.ReLU(),
    nn.MaxPool2d(2)
)
output = model(input)  # 즉시 실행 가능

# TensorFlow 2.x: Keras API로 개선되었지만 여전히 복잡
model = tf.keras.Sequential([
    tf.keras.layers.Conv2D(64, 3, activation='relu'),
    tf.keras.layers.MaxPooling2D(2)
])
```
- **Dynamic Computation Graph**: 코드 실행 흐름 그대로 그래프가 생성되어 디버깅이 Python 표준 디버거(pdb)로 즉시 가능.
- **TensorFlow**: Static Graph(tf.function) 사용 시 디버깅이 어렵고, eager mode는 성능 저하.

#### 2️⃣ C++ 개발자에게 친숙한 명시적 제어
| 특성 | PyTorch | TensorFlow |
|------|---------|------------|
| **메모리 관리** | `tensor.detach()`, `with torch.no_grad()` 등 명시적 제어 | 자동 최적화가 강하지만 블랙박스 |
| **연산 그래프** | 코드 흐름 = 그래프 (WYSIWYG) | `tf.function` 데코레이터로 추상화 |
| **타입 추론** | `torch.Tensor` 명시적 타입 | `tf.Tensor`와 NumPy 혼용 시 혼란 |

**C++ 개발자 관점**: PyTorch는 "Python으로 작성하지만 C++처럼 동작"하는 느낌. TensorFlow는 추상화 레이어가 많아 제어권이 약함.

#### 3️⃣ 커뮤니티 및 최신 연구 접근성
- **학술 논문**: 2023~2026년 CVPR, NeurIPS 등 주요 컨퍼런스의 **70% 이상이 PyTorch 코드 제공** (2026년 기준).
- **Pre-trained Models**: Hugging Face Transformers, torchvision, timm(PyTorch Image Models) 등 최신 모델이 PyTorch 우선 지원.
- **TensorFlow**: Google Research 중심, 하지만 최신 연구 코드 전환에 시간 소요.
- **커뮤니티 규모**: PyTorch와 TensorFlow 모두 매우 큰 커뮤니티를 보유하나, 연구 분야에서는 PyTorch가 사실상 표준.

#### 4️⃣ PyTorch 2.0 성능 혁신 (2026년 기준)
```python
# PyTorch 2.0+ torch.compile() 사용
import torch

model = MyModel()
# 단 한 줄로 30-40% 성능 향상
compiled_model = torch.compile(model)

# 기존 PyTorch 코드 그대로 사용 가능
output = compiled_model(input)
```

**2026년 최신 벤치마크 결과:**
| 프레임워크 | 대규모 모델 학습 속도 | 추론 지연 시간 | 10B 파라미터 미만 모델 |
|-----------|-----------------|------------|-------------------|
| **PyTorch 2.0+** | 기준 | 기준 | ✅ **최적** (torch.compile) |
| **JAX** | 35-40% 빠름 (TPU) | 20-30% 빠름 | PyTorch 2.0과 동등 |
| **TensorFlow 2.x** | PyTorch와 유사 | PyTorch와 유사 | 약간 느림 |

- **PyTorch 2.0의 `torch.compile()`**: JIT 컴파일로 JAX와의 성능 격차를 크게 축소.
- **10B 파라미터 미만 모델**: PyTorch 2.0이 JAX보다 개발 시간 30-40% 단축으로 **총 소유 비용(TCO) 우수**.
- **대규모 모델(10B+)**: JAX가 TPU에서 여전히 35-40% 빠르지만, 이 프로젝트는 중소 규모 모델 대상.

#### 5️⃣ 학습 곡선 및 생산성
- **PyTorch**: NumPy 경험이 있다면 1~2주면 모델 구축 가능. ⭐⭐⭐⭐⭐ (매우 낮음)
- **TensorFlow**: Keras API(고수준)와 저수준 API의 혼용으로 학습 시간 2~3배. ⭐⭐⭐ (중간)
- **JAX**: 함수형 프로그래밍 패러다임이 낯설어 러닝 커브 가파름. ⭐⭐ (높음)
- **개발 생산성**: PyTorch의 직관적 API로 JAX 대비 30-40% 빠른 개발 속도 (2026년 업계 설문 기준).

#### 6️⃣ 프로덕션 배포 전략 (ONNX 연계)
```python
# 1. PyTorch로 개발 (빠른 프로토타이핑)
model = MyModel()
model.train()

# 2. ONNX로 변환 (프로덕션 배포)
torch.onnx.export(model, dummy_input, "model.onnx")

# 3. ONNX Runtime으로 추론 (2~5배 빠름)
import onnxruntime as ort
session = ort.InferenceSession("model.onnx", providers=['CUDAExecutionProvider', 'CPUExecutionProvider'])
output = session.run(None, {"input": input_data})
```

**배포 생태계 비교 (2026년):**
| 프레임워크 | 프로덕션 도구 | 모바일/엣지 | 성숙도 | 추론 최적화 |
|-----------|------------|-----------|-------|----------|
| **PyTorch** | TorchServe, ONNX 변환 | 성장 중 | ⭐⭐⭐⭐ | ONNX/TensorRT |
| **TensorFlow** | TF Serving, TF Lite | ✅ 최고 | ⭐⭐⭐⭐⭐ | XLA, TF Lite |
| **JAX** | 커스텀 솔루션 필요 | 제한적 | ⭐⭐ | JIT 컴파일 |

- **장점**: 개발은 PyTorch의 편의성, 배포는 ONNX의 성능 → "Best of Both Worlds".
- **TensorFlow도 ONNX 지원**: 하지만 변환 과정에서 연산자 호환성 이슈 빈번.
- **2026년 개선 사항**: PyTorch의 TorchServe가 성숙해지고, ONNX Runtime의 하드웨어 가속 지원 확대.
- **프로젝트 적합성**: 웹 기반 서비스로 모바일 배포 불필요 → TensorFlow의 TF Lite 장점 불필요.

#### 7️⃣ GPU 활용 단순성
```python
# PyTorch: 한 줄로 GPU 전환
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model.to(device)
input_tensor.to(device)

# TensorFlow: GPU가 자동 감지되지만 세밀한 제어 어려움
# (XLA 컴파일, 메모리 프리 할당 등)
```
- **명시적 제어**: C++ 개발자에게 친숙한 수동 메모리 관리.
- **멀티 GPU**: `torch.nn.DataParallel`, `DistributedDataParallel` 지원.
- **CUDA 통합**: PyTorch는 CUDA 코드와의 상호운용성이 우수.

### 대안 및 트레이드오프 (Alternatives)

#### TensorFlow 2.x (Google)
- ✅ **장점**: 
  - **프로덕션 생태계** ⭐⭐⭐⭐⭐: TensorFlow Serving(확장 가능한 모델 서빙), TensorFlow Lite(모바일/엣지), TensorFlow.js(웹).
  - **성숙한 배포 도구**: TFX(파이프라인), TensorBoard(시각화), Model Garden(사전 학습 모델).
  - **엔터프라이즈 지원**: Google Cloud의 검증된 인프라, 광범위한 문서 및 기업 지원.
  - **교차 플랫폼**: 서버, 모바일, 웹, 임베디드 모든 환경 지원.
  - **Keras 통합**: 고수준 API로 빠른 시작 가능.
- ❌ **Rejected**: 
  - **초기 MVP 불일치**: 모바일/엣지 배포 불필요 (React 웹 우선) → TF Lite의 주요 장점 활용 불가.
  - **복잡한 API**: Keras(고수준)와 저수준 API 혼용 시 복잡도 증가, 학습 곡선 ⭐⭐⭐ (중간).
  - **디버깅 어려움**: `tf.function` 데코레이터로 그래프 모드 전환 시 오류 추적 어려움.
  - **연구 코드 변환 비용**: 최신 논문의 70%가 PyTorch 코드 제공 → TensorFlow로 포팅에 추가 시간.
  - **연구 커뮤니티 축소**: 학계에서 PyTorch 선호도 증가 (2026년 기준).
  - **API 버전 변화**: TF 1.x → 2.x 마이그레이션 경험으로 인한 우려 (호환성 이슈).

#### JAX (Google Research)
- ✅ **장점**: 
  - **최고 성능**: TPU에서 대규모 모델(10B+ 파라미터) 학습 시 35-40% 빠름.
  - **자동 미분**: `grad`, `jit`, `vmap` 등 강력한 함수형 변환.
  - **추론 최적화**: PyTorch 대비 20-30% 낮은 지연 시간 (2026년 벤치마크).
  - **비용 절감**: 월 1,000만 예측 서비스 기준, TPU 활용 시 인프라 비용 절감 가능.
- ❌ **Rejected**: 
  - **높은 학습 곡선** ⭐⭐: 함수형 프로그래밍 패러다임이 명령형에 익숙한 C++ 개발자에게 생소함.
  - **작은 생태계**: Pre-trained 모델 및 커뮤니티가 PyTorch/TensorFlow 대비 부족 (중간 규모).
  - **배포 인프라 부족**: 전용 서빙 도구가 없어 커스텀 솔루션 개발 필요 → 유지보수 비용 증가.
  - **프로젝트 목표 불일치**: 이 프로젝트는 "최첨단 성능"보다 "빠른 개발 및 안정적 운영" 우선.
  - **하드웨어 제약**: TPU 중심 최적화 (GPU 지원은 개선 중이나 여전히 제한적).
  - **10B 미만 모델**: PyTorch 2.0의 `torch.compile()`로 성능 격차 축소 → JAX의 복잡도 대비 이득 미미.

#### ONNX Runtime 직접 사용
- ✅ **장점**: 추론 성능 최적화, 크로스 플랫폼 배포.
- ❌ **Rejected**: 
  - 모델 학습 기능 없음 (추론 전용).
  - 학습은 PyTorch로 하고 배포는 ONNX로 하는 것이 현실적.

### 결론 (Consequences)

#### ✅ 장점
1. **개발 생산성**: 
   - Pythonic한 API로 빠른 학습 및 프로토타이핑 (NumPy 경험자 1~2주면 숙달).
   - 동적 계산 그래프로 Python 디버거(pdb, IDE breakpoint) 직접 사용 가능.
   - JAX 대비 30-40% 빠른 개발 속도 (2026년 업계 설문).

2. **연구 생태계**:
   - 최신 논문의 70% 이상이 PyTorch 코드 제공 (2026년 CVPR, NeurIPS 등).
   - Hugging Face, timm, torchvision 등 풍부한 사전 학습 모델.
   - 커뮤니티 규모: 매우 큼 (학계 표준).

3. **성능** (2026년 기준):
   - **PyTorch 2.0 `torch.compile()`**: 30-40% 성능 향상, JAX와 격차 축소.
   - **10B 미만 모델**: PyTorch가 개발 시간 고려 시 총 소유 비용(TCO) 우수.
   - **ONNX 변환**: 프로덕션 배포 시 추론 속도 2~5배 개선 가능.

4. **제어권**:
   - C++ 개발자에게 친숙한 명시적 메모리 관리 (`detach()`, `no_grad()` 등).
   - GPU 전환 간단 (`model.to(device)` 한 줄).

#### ⚠️ 단점
1. **프로덕션 생태계**: 
   - TensorFlow Serving 같은 전용 인프라 부족 (TorchServe는 성장 중).
   - **대책**: FastAPI + ONNX Runtime 조합으로 충분히 대체 가능.

2. **모바일 배포**:
   - TensorFlow Lite 대비 모바일 지원 제한적.
   - **프로젝트 영향**: 웹 기반 서비스로 모바일 배포 불필요 → 영향 없음.

3. **대규모 모델**:
   - 10B+ 파라미터 모델에서 JAX 대비 TPU 활용 시 35-40% 느림.
   - **프로젝트 영향**: 이미지 분석 모델은 중소 규모 → 영향 없음.

#### 🚀 향후 확장성
1. **단기 (MVP)**:
   - PyTorch로 빠른 개발 및 반복.
   - 초기에는 PyTorch 네이티브 추론 사용.

2. **중기 (프로덕션 최적화)**:
   - ONNX Runtime 전환으로 **추론 지연 시간 50% 단축** 목표.
   - CPU/GPU 인스턴스에서 비용 대비 성능 최적화.

3. **장기 (성능 극대화)**:
   - **TensorRT** (NVIDIA GPU 전용): ONNX 대비 추가 20-30% 속도 개선.
   - **양자화(Quantization)**: INT8/FP16 변환으로 메모리 및 속도 최적화.
   - **모델 경량화**: 지식 증류(Knowledge Distillation), 프루닝(Pruning) 적용 가능.

#### 📊 총평 (2026년 관점)
- **PyTorch는 연구와 중소 규모 프로덕션의 최적 선택**: 학계 표준, 빠른 개발, PyTorch 2.0으로 성능 격차 축소.
- **TensorFlow는 대규모 엔터프라이즈 및 모바일**: 성숙한 인프라, 교차 플랫폼 지원.
- **JAX는 대규모 연구 및 TPU 활용**: 최고 성능이지만 높은 학습 곡선과 배포 복잡도.
- **이 프로젝트**: 빠른 MVP 개발, 중소 규모 모델, 웹 중심 → **PyTorch가 명확한 승자**.

#### 🌐 2026년 AI 프레임워크 트렌드
1. **PyTorch 지배력 강화**:
   - 학계: 주요 컨퍼런스(CVPR, NeurIPS)의 70% 이상 PyTorch 사용.
   - 산업계: 스타트업 및 AI 연구 조직의 압도적 선호.
   - PyTorch 2.0의 `torch.compile()`: 성능과 편의성 양립.

2. **TensorFlow 재편성**:
   - Google이 JAX와 통합 전략 추진 (Keras 3.0이 멀티 백엔드 지원).
   - 프로덕션 시장에서는 여전히 강세 (TF Serving, TF Lite).
   - 신규 연구 프로젝트에서는 채택률 감소.

3. **JAX 성장**:
   - 대규모 언어 모델(LLM) 학습에서 입지 강화 (DeepMind, Google Brain).
   - 하지만 여전히 전문가 대상, 중소 프로젝트에는 과도.
   - Flax, Optax 등 생태계 확장 중.

4. **ONNX 표준화 가속**:
   - 프레임워크 간 상호운용성 증가.
   - PyTorch → ONNX → TensorRT 파이프라인이 프로덕션 표준.
   - ONNX Runtime의 하드웨어 가속 지원 확대 (NPU, Apple Silicon 등).

5. **전문화 프레임워크 등장**:
   - **MLX** (Apple Silicon 전용): M-series 칩 최적화.
   - **Mojo** (Modular): Python 문법 + C++ 성능.
   - 하지만 범용성에서는 PyTorch/TensorFlow 우위 유지.

#### 📚 참고 자료 및 레퍼런스 (2026년)
- **성능 벤치마크**: [PyTorch vs JAX: ML Production Performance & Cost Analysis (2026)](https://drcodes.com/posts/pytorch-vs-jax-ml-production-performance-cost-analysis)
  - 대규모 모델 학습: JAX 35-40% 빠름 (TPU).
  - 10B 미만 모델: PyTorch 2.0으로 격차 축소.
  - 추론 지연: JAX 20-30% 낮음.

- **프레임워크 비교**: [PyTorch 2.0 vs TensorFlow 3.0 vs JAX (2026)](https://www.index.dev/skill-vs-skill/ai-pytorch2-vs-tensorflow3-vs-jax)
  - PyTorch 2.0의 `torch.compile()`: 30-40% 엔지니어링 시간 단축.
  - 총 소유 비용(TCO): 중소 모델에서 PyTorch 우수.

- **학습 가이드**: [TensorFlow vs PyTorch: Which Framework Should You Learn in 2025?](https://www.udacity.com/blog/2025/06/tensorflow-vs-pytorch-which-framework-should-you-learn-in-2025.html)
  - PyTorch: 연구, 프로토타이핑, 최신 트렌드.
  - TensorFlow: 프로덕션, 모바일, 엔터프라이즈.

- **공식 문서**:
  - PyTorch 공식: https://pytorch.org/docs/
  - TensorFlow 공식: https://www.tensorflow.org/
  - JAX 공식: https://jax.readthedocs.io/

### 채용 시장 관점
- **2024~2026년 AI 엔지니어 채용 공고**: PyTorch 언급 빈도 > TensorFlow (약 60% vs 40%).
- **스타트업 선호**: PyTorch (빠른 실험), **대기업 선호**: TensorFlow (안정성).
- **이 프로젝트**: 포트폴리오 목적이므로 "최신 트렌드"인 PyTorch 선택이 유리.

---

## 🎯 의사결정 요약 (Summary)

| 기술 선택 | 채택 이유 (3줄 요약) |
|-----------|---------------------|
| **마이크로서비스 아키텍처** | 언어별 최적 도메인 활용 + 장애 격리 + 독립 확장 |
| **Docker Compose** | 프로젝트 규모에 적합 + 빠른 개발 + 비용 효율 |
| **HTTPS(외부) + HTTP(내부)** | 보안 충족 + 성능 최적화 + 관리 단순화 |
| **FastAPI** | 비동기 성능 + 자동 문서화 + 타입 안전성 + 채용 수요 |
| **Crow** | Express.js 스타일 + 성능-복잡도 균형 + MFC 경험 현대화 |
| **Atomic Write** | 파일 무결성 보장 + 락 메커니즘 불필요 + 간단한 구현 |
| **TDD + Tidy First** | 회귀 버그 방지 + 리팩터링 안전성 + 채용 시장 가치 |
| **HTTP REST** | 빠른 개발 + 낮은 학습 곡선 + 범용성 + 디버깅 용이 |
| **PyTorch 2.0** | torch.compile 성능 + 학계 표준 70% + ONNX 변환 + 빠른 개발 |
| **EfficientNet-B2** | 파라미터 효율(9.2M) + ONNX/TensorRT 호환 + 스케치 도메인 Fine-tuning 적합 |

---

## 📌 사용 가이드 (How to Use This Document)

### 면접 시 활용법
**질문**: "왜 Kubernetes 대신 Docker Compose를 선택하셨나요?"  
**답변**:
> "저희 프로젝트는 초기 MVP 단계로 동시 접속 50명 이하, 단일 서버 환경입니다. Kubernetes는 수백 대의 서버를 관리하는 도구로, 현재 규모에는 Over-Engineering입니다. 대신 Docker Compose로 `docker-compose up -d` 한 줄로 전체 시스템을 실행할 수 있도록 하여 개발 생산성을 극대화했습니다. 향후 사용자가 급증하면 동일한 Docker 이미지를 Kubernetes로 마이그레이션할 수 있는 확장성도 확보했습니다."

### 이력서/포트폴리오 링크
```markdown
## 주요 성과
- **기술적 의사결정 문서화**: 8개의 ADR(Architecture Decision Records)을 작성하여 모든 기술 선택의 근거를 명확히 기록. ([링크](https://github.com/yourrepo/docs/ARCHITECTURE_DECISIONS.md))
```

---

## ADR-010: Transfer Learning 모델로 EfficientNet-B2 채택

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
Python AI 서버에서 아동 인물화 지능 측정을 위한 딥러닝 모델을 구축해야 합니다.  
주요 제약사항:
- **도메인 특수성**: 아동 인물화 = 스케치/드로잉 형태 (사진과 다른 도메인)
- **데이터 제한**: 의료/아동 도메인 특성상 대규모 데이터셋 확보 어려움
- **하드웨어 제약**: RTX 3050 Ti (4GB VRAM)
- **성능 목표**: 추론 속도 < 2초, ONNX/TensorRT 변환 필수
- **출력 형태**: Multi-head 분류 (신체 부위별 + 세부 특징 점수)

주요 후보: **EfficientNet**, **ConvNeXt**, **Vision Transformer (ViT)**, **ResNet**

### 의사결정 (Decision)
**EfficientNet-B2**를 1순위 Transfer Learning 백본(Backbone) 모델로 채택합니다.

### 분석 방법론
- **Context7**: PyTorch 공식 문서 및 torchvision pretrained 모델 조사
- **Sequential Thinking**: 제1원칙(First Principles) 기반 체계적 분석

### 근거 (Rationale)

#### 1️⃣ Compound Scaling으로 파라미터 효율성 최고
| 모델 | 파라미터 | ImageNet Top-1 | 추론속도 (512x512) |
|------|----------|----------------|-------------------|
| **EfficientNet-B2** | 9.2M | 80.1% | ~50ms |
| ConvNeXt-Tiny | 28.6M | 82.1% | ~80ms |
| ResNet-50 | 25.6M | 76.1% | ~40ms |
| ViT-B/16 | 86.6M | 77.9% | ~120ms |

- EfficientNet-B2는 ResNet-50의 **36% 파라미터**로 **4% 더 높은 정확도** 달성.
- 깊이(Depth), 너비(Width), 해상도(Resolution)를 균형있게 스케일링하는 Compound Scaling 기법 적용.

#### 2️⃣ Transfer Learning의 본질 (제1원칙)
```
┌─────────────────────────────────────────────────────────────────┐
│               EfficientNet-B2 (사전학습)                          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  🔒 Feature Extractor (동결/재사용)                         │   │
│  │  - Conv 레이어들                                           │   │
│  │  - ImageNet 120만장으로 학습된 "눈"                         │   │
│  │  - 엣지, 텍스처, 패턴 인식 능력 보유                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                         ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  🔓 Classifier Head (새로 학습)                            │   │  ← 여기만 새로 구현!
│  │  - Multi-head 분류기                                       │   │
│  │  - 머리/몸통/팔다리 점수 출력                                │   │
│  │  - 아동 인물화 데이터로 학습                                 │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

- **핵심 원리**: 낮은 레이어(앞쪽)는 엣지/텍스처 같은 **범용적 특징** 학습 → 스케치 도메인에도 적용 가능
- **비유**: "이미 훈련된 '눈'을 빌려와서, 아동 인물화를 판단하는 '두뇌'만 새로 학습시키는 것"

#### 3️⃣ 적은 데이터 Fine-tuning에 강점
```python
# Stage 1: Feature Extractor 동결 (5~10 epochs)
for param in model.features.parameters():
    param.requires_grad = False

# 새로운 Multi-head 분류기
model.classifier = nn.ModuleDict({
    'head': nn.Linear(1408, 64),       # 머리 점수
    'body': nn.Linear(1408, 64),       # 몸통 점수
    'limbs': nn.Linear(1408, 64),      # 팔/다리 점수
    'total': nn.Linear(1408, 1)        # 종합 점수
})

# Stage 2: 상위 블록 언동결 (낮은 Learning Rate)
for param in model.features[-3:].parameters():
    param.requires_grad = True
```

#### 4️⃣ ONNX/TensorRT 변환 검증됨
- EfficientNet 계열은 ONNX/TensorRT 변환 파이프라인이 안정적으로 검증됨.
- FP16 양자화 시 정확도 손실 최소 (~0.1%).

#### 5️⃣ 하드웨어 제약 충족
| 지표 | EfficientNet-B2 | ViT-B/16 | 목표 |
|------|-----------------|----------|------|
| **VRAM 사용량** | ~1.5GB | ~4GB+ | < 4GB ✅ |
| **추론 속도** | ~50ms | ~120ms | < 2000ms ✅ |

### 대안 및 트레이드오프 (Alternatives)

#### ConvNeXt-Tiny (2순위)
- ✅ **장점**: 2022년 최신 CNN 아키텍처, ViT 수준 성능, 기하학적 특징 추출 강점.
- ❌ **Rejected (1순위에서)**: 파라미터 28.6M으로 EfficientNet-B2(9.2M) 대비 3배.
- **적합 시나리오**: 데이터가 충분하고 SOTA 성능이 중요한 경우.

#### Vision Transformer (ViT-B/16)
- ✅ **장점**: Self-Attention으로 전역 패턴 인식, 대규모 데이터에서 CNN 압도.
- ❌ **Rejected**: 
  - **과적합 위험**: 적은 데이터셋에서 학습 불안정 ⚠️ 치명적.
  - **VRAM 제약**: 4GB VRAM에서 배치 크기 제한.
  - **로컬 특징 약함**: 엣지/윤곽선 인식에 불리 (스케치 도메인에 부적합).

#### ResNet-18/34/50
- ✅ **장점**: 가장 안정적, 풍부한 레퍼런스, 빠른 추론 속도.
- ❌ **Rejected**: 최신 모델 대비 성능 한계 (Top-1 3~5% 열위).
- **적합 용도**: 학습/실험용 베이스라인으로만 권장.

### 결론 (Consequences)
- ✅ **장점**:
  - 파라미터 효율성 (ResNet-50의 36%).
  - 적은 데이터 Fine-tuning 안정성.
  - ONNX/TensorRT 변환 검증됨.
  - 4GB VRAM 제약 충족.
  - Multi-head 분류 구조 용이.
- ⚠️ **단점**:
  - ImageNet 사전학습 → 스케치 도메인과 약간의 갭 존재 (데이터 증강으로 완화).
  - ConvNeXt 대비 1~2% 낮은 정확도 (하지만 속도/효율성으로 보완).
- **학습 자료**: 
  - [상세 분석 문서](tech-references/AI/ai_model_recommendation.md)
  - [PyTorch EfficientNet 공식 문서](https://pytorch.org/vision/stable/models/efficientnet.html)

---

## ADR-011: C++ 전처리 파이프라인 결과물 명세

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
C++ 전처리 서버가 Python AI 서버에 전달할 이미지 형식을 결정해야 합니다.  
주요 제약사항:
- **AI 모델**: EfficientNet-B2 (ImageNet 사전학습, 3채널 RGB 입력)
- **도메인**: 연필 스케치 (색상 없음, 선 정보가 핵심)
- **성능 목표**: 전처리 < 100ms

### 의사결정 (Decision)
**Binarized 이미지를 RGB 3채널로 변환하여 출력**합니다.

```
파이프라인: Preprocess → Canny → Morphology → Binarize → RGB 변환
```

### 근거 (Rationale)

#### 1️⃣ C++ vs Python 역할 분담 (제1원칙)
| 담당 | 역할 | 이유 |
|------|------|------|
| **C++ 전처리** | 기하학적 처리 | 속도 최적화 (OpenCV) |
| **Python AI** | 의미론적 처리 | 모델 의존적 연산 (PyTorch) |

- **C++ 담당**: 리사이즈, 노이즈 제거, 에지 검출, 이진화
- **Python 담당**: ImageNet 정규화, Tensor 변환, 추론
- **경계 원칙**: 정규화는 모델 의존적이므로 Python에서 처리

#### 2️⃣ 도메인 특수성: 연필 스케치
```
아동 인물화 특성:
- 연필 + 지우개로 그림 (색상 정보 없음)
- 선/윤곽선이 핵심 정보
- 흰 종이 배경 (복잡한 배경 제거 불필요)
```

**결론**: 색상 보존보다 **선 정보 강화**가 중요 → Binarize 선택

#### 3️⃣ EfficientNet-B2 입력 호환성
```python
# Python AI 전처리 (Context7 확인)
transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
```

- EfficientNet-B2는 **RGB 3채널** 입력 필요
- Binarized 이미지(1채널)를 `cv::cvtColor(binarized, result, cv::COLOR_GRAY2BGR)`로 변환
- 결과: 흰 배경(255, 255, 255) + 검은 선(0, 0, 0)

#### 4️⃣ GrabCut 제외 결정
| 알고리즘 | 처리 시간 | 필요성 |
|----------|----------|--------|
| GrabCut | 4,233ms | ❌ 불필요 (단순 배경) |
| Canny | ~5ms | ✅ 에지 검출 |
| Binarize | ~2ms | ✅ 선 강화 |

- **GrabCut**: 복잡한 배경 제거용 → 흰 종이 배경에는 과도함
- **성능**: 목표 100ms 대비 40배 초과 → 제외
- **테스트 유지**: 기술 역량 증명을 위해 테스트 코드에는 보존

### 파이프라인 상세

```cpp
// server.h - ProcessImageFile()

// Step 1: Preprocess (Letterbox 512×512 + Denoise + Grayscale)
cv::Mat preprocessed = processor.Preprocess(img);

// Step 2: Canny Edge Detection (threshold 50/150)
cv::Mat edges = processor.DetectEdges(preprocessed, 50, 150);

// Step 3: Morphology Enhancement (MORPH_CLOSE, kernelSize=3)
cv::Mat enhanced = processor.EnhanceContours(edges, 3);

// Step 4: Adaptive Binarization
cv::Mat binarized = processor.Binarize(preprocessed);

// Step 5: Convert to RGB 3-channel
cv::cvtColor(binarized, result, cv::COLOR_GRAY2BGR);
```

### 결과물 명세

| 항목 | 값 | 이유 |
|------|-----|------|
| **파일 형식** | JPEG (품질 95) | 압축 효율, 범용 호환성 |
| **해상도** | 512×512 | EfficientNet-B2 입력 크기 |
| **리사이즈** | Letterbox (검은 패딩) | 비율 왜곡 방지 |
| **채널** | 3채널 RGB | AI 모델 호환성 |
| **내용** | 흰 배경 + 검은 선 | 선 정보 극대화 |

### 대안 및 트레이드오프 (Alternatives)

#### 옵션 A: 컬러 RGB 출력 (Preprocess 결과)
- ✅ **장점**: 색상 정보 보존, 색연필/크레파스 그림에 적합
- ❌ **Rejected**: 연필 스케치 도메인에서는 색상 정보 불필요

#### 옵션 C: 둘 다 출력 (하이브리드)
- ✅ **장점**: AI 모델이 멀티 입력 활용 가능
- ❌ **Rejected**: 구현 복잡도 증가, 현재 단계에서 과도한 엔지니어링

### 결론 (Consequences)
- ✅ **장점**:
  - 연필 스케치 도메인에 최적화
  - EfficientNet-B2 입력 호환
  - 처리 시간 < 100ms 달성
  - 역할 분담 명확화 (C++ = 기하학, Python = 의미론)
- ⚠️ **단점**:
  - 색연필/크레파스 그림에는 부적합 (향후 옵션 A로 전환 가능)
- **관련 문서**: 
  - [GrabCut 제외 의사결정](troubleshooting/Week3_Issues.md)
  - [ADR-010: EfficientNet-B2 채택](#adr-010-transfer-learning-모델로-efficientnet-b2-채택)

---

## ADR-012: C++ 전처리 서버 개발 로드맵 (Week 2-4)

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
C++ 전처리 서버 개발을 단계별로 진행하며, 각 주차별 목표와 기술적 근거를 정의해야 합니다.  
주요 고려사항:
- **점진적 복잡도 증가**: 기본 → 고급 기능 순서로 구현
- **포트폴리오 가치**: Modern C++ 스킬, 디자인 패턴, 동시성 역량 증명
- **TDD 방법론**: 모든 기능은 테스트 우선 개발

### 의사결정 (Decision)
4주 간의 단계별 개발 로드맵을 정의합니다.

---

### Week 2: OpenCV 전처리(기본) + API

#### 목표
- OpenCV 도입 및 기본 이미지 처리 기능 구현
- REST API 엔드포인트 설계 및 구현

#### 기술적 배경

| 요소 | 선택 | 근거 |
|------|------|------|
| **빌드 시스템** | vcpkg + CMake | Windows 환경에서 OpenCV 의존성 관리 용이 |
| **이미지 크기** | 512×512 | EfficientNet-B2 입력 해상도 매칭 |
| **노이즈 제거** | GaussianBlur + medianBlur | 고주파 노이즈 + salt-and-pepper 노이즈 동시 제거 |
| **그레이스케일** | cvtColor(BGR2GRAY) | 후속 Canny 에지 검출 전처리 |

#### 제1원칙 분석
```
문제: 이미지 전처리를 어떻게 시작할 것인가?

분해:
1. 입력 정규화 → 일관된 크기로 리사이즈
2. 노이즈 제거 → 후속 처리 품질 향상
3. 채널 변환 → 에지 검출 준비

결론: 기본 전처리 3단계 (Resize → Denoise → Grayscale)
```

---

### Week 3.5: 디자인 패턴 적용 및 아키텍처 리팩터링

#### 목표
- 유지보수성 향상을 위한 디자인 패턴 적용
- Week 4 멀티스레딩 구현을 위한 기반 설계

#### 기술적 배경

| 패턴 | 적용 대상 | 근거 |
|------|----------|------|
| **Strategy Pattern** | 필터 시스템 | OCP(개방-폐쇄 원칙) 준수, 새 필터 추가 시 기존 코드 수정 불필요 |
| **Composite Pattern** | 파이프라인 | 필터들을 동적으로 조합하여 파이프라인 구성 |
| **Factory Pattern** | 파이프라인 생성 | 반복되는 생성 코드 제거, 사전 정의 파이프라인 |
| **Producer-Consumer** | 태스크 큐 | Week 4 Thread Pool 기반 설계 |

---

#### 1️⃣ Strategy Pattern 선택 이유

**문제**: 현재 `ImageProcessor`에 여러 필터 메서드가 있음
```cpp
// 현재 구조 (문제점)
class ImageProcessor {
    cv::Mat DetectEdges(...);     // Canny
    cv::Mat EnhanceContours(...); // Morphology
    cv::Mat Binarize(...);        // Threshold
    // 새 필터 추가하려면? → 클래스 수정 필요 (OCP 위반)
};
```

**해결**: 각 필터를 독립적인 객체로 분리
```cpp
// Strategy Pattern 적용 후
class IFilter { virtual cv::Mat apply(...) = 0; };
class CannyFilter : public IFilter { ... };
class NewFilter : public IFilter { ... };  // 기존 코드 수정 없이 확장!
```

| 장점 | 설명 |
|------|------|
| **OCP 준수** | 새 필터 추가 시 기존 코드 수정 불필요 |
| **단일 책임** | 각 필터가 하나의 역할만 담당 |
| **테스트 용이** | 필터별 독립 테스트 가능 |

---

#### 2️⃣ Composite Pattern 선택 이유

**문제**: 필터들을 순차적으로 조합해야 함
```cpp
// 현재: 하드코딩된 순서
cv::Mat result = processor.Preprocess(img);
result = processor.DetectEdges(result);
result = processor.Binarize(result);
// 순서 변경하려면? → 코드 수정 필요
```

**해결**: 필터들을 동적으로 조합하는 파이프라인
```cpp
// Composite Pattern 적용 후
FilterPipeline pipeline;
pipeline.add(std::make_unique<ResizeFilter>(512));
pipeline.add(std::make_unique<CannyFilter>(50, 150));
// 순서 변경? → 코드 수정 없이 add 순서만 변경
cv::Mat result = pipeline.execute(input);
```

| 장점 | 설명 |
|------|------|
| **동적 조합** | 런타임에 필터 순서 변경 가능 |
| **재사용성** | 같은 필터를 여러 파이프라인에서 재사용 |
| **유연성** | 새 파이프라인 구성이 쉬움 |

---

#### 3️⃣ Factory Pattern 선택 이유

**문제**: 자주 사용하는 필터 조합을 반복 생성해야 함
```cpp
// 매번 이렇게 생성?
FilterPipeline pipeline;
pipeline.add(std::make_unique<ResizeFilter>(512));
pipeline.add(std::make_unique<DenoiseFilter>());
pipeline.add(std::make_unique<GrayscaleFilter>());
// ... 반복되는 보일러플레이트
```

**해결**: 사전 정의된 파이프라인을 팩토리로 생성
```cpp
// Factory Pattern 적용 후
auto pipeline = PipelineFactory::createSketchPipeline();  // 한 줄로!
cv::Mat result = pipeline.execute(input);
```

| 장점 | 설명 |
|------|------|
| **중복 제거** | 반복되는 생성 코드 제거 |
| **일관성** | 같은 이름의 파이프라인은 항상 같은 구성 |
| **유지보수** | 파이프라인 구성 변경은 팩토리만 수정 |

---

#### 4️⃣ Producer-Consumer Pattern 선택 이유

**문제**: Week 4에서 멀티스레딩 구현 예정
```cpp
// 현재: 단일 스레드, 순차 처리
for (auto& image : images) {
    processImage(image);  // 하나씩 처리 (느림)
}
```

**해결**: 작업 큐를 미리 설계해두기
```cpp
// Producer-Consumer 준비
ITaskQueue<ImageTask> queue;  // 인터페이스 정의
// Week 4에서 ThreadPool과 연동
ThreadPool pool(4);
pool.submitTo(queue);  // 4개 스레드가 큐에서 작업 가져감
```

| 장점 | 설명 |
|------|------|
| **Week 4 준비** | 멀티스레딩 기반 설계 미리 완료 |
| **인터페이스 분리** | 단일 스레드 테스트 가능 (`SyncTaskQueue`) |
| **확장성** | 나중에 `AsyncTaskQueue`로 교체 |

---

#### 포트폴리오 가치
- **SOLID 원칙 적용**: OCP, SRP, DIP 준수 증명
- **GoF 디자인 패턴**: Strategy, Composite, Factory 적용
- **Modern C++ 스킬**: `std::unique_ptr`, `virtual`, interface 설계
- **확장성**: 새로운 필터 추가 시 기존 코드 수정 없이 확장

---

### Week 4: 멀티스레딩/성능/품질

#### 목표
- Thread Pool 구현으로 병렬 처리 성능 향상
- Atomic Write로 파일 무결성 보장
- 성능 벤치마크 및 품질 게이트 구축

#### 기술적 배경

| 요소 | 선택 | 근거 |
|------|------|------|
| **Thread Pool** | `std::thread` + `condition_variable` | 표준 라이브러리만 사용, 외부 의존성 없음 |
| **Atomic Write** | `.tmp` → `rename` 패턴 | 저장 중 프로세스 종료 시 파일 손상 방지 |
| **성능 목표** | 전처리 < 100ms | AI 추론 시간 고려, 전체 파이프라인 < 2초 |

#### Thread Pool 선택 이유
```
대안 분석:
1. std::async          → 스레드 수 제어 불가, 오버헤드 발생 가능
2. OpenMP             → 추가 의존성, 복잡한 설정
3. Intel TBB          → 과도한 엔지니어링, 학습 비용 높음
4. 직접 구현 (std::thread) → ✅ 표준 라이브러리, 스킬 증명, 제어력

선택: 직접 구현 (std::thread)
```

#### Atomic Write 패턴
```cpp
// 위험: 직접 저장 (프로세스 종료 시 파일 손상)
cv::imwrite("output.jpg", img);  // 저장 중 크래시 → 손상된 파일

// 안전: Atomic Write 패턴
cv::imwrite("output.jpg.tmp", img);           // 1. 임시 파일에 저장
std::filesystem::rename("output.jpg.tmp",     // 2. 원자적 이름 변경
                        "output.jpg");
```

#### 성능 벤치마크 계획
| 스레드 수 | 예상 처리량 | 비고 |
|----------|------------|------|
| 1 | Baseline | 단일 스레드 |
| 4 | ~3-4x | 일반 데스크톱 |
| 8 | ~6-7x | 고성능 CPU |

#### 포트폴리오 가치
- **동시성 역량**: Thread Pool, mutex, condition_variable 활용
- **데이터 무결성**: Atomic Write 패턴 적용
- **성능 최적화**: 벤치마크 기반 성능 분석 및 튜닝

---

### 전체 로드맵 요약

```
Week 2: 기초 다지기
└─ OpenCV 도입, REST API, 기본 전처리 (Resize/Denoise/Gray)

Week 3: 기능 확장
└─ GrabCut, Canny, Morphology, Binarize, RGB 변환

Week 3.5: 아키텍처 개선
└─ Strategy Pattern, Composite Pattern, OCP 준수

Week 4: 성능 최적화
└─ Thread Pool, Atomic Write, 벤치마크, 품질 게이트
```

- **관련 문서**: 
  - [plan.md](../plan.md) - 전체 개발 계획
  - [ADR-011: 전처리 파이프라인 명세](#adr-011-c-전처리-파이프라인-결과물-명세)

---

## ADR-013: Git Workflow로 GitHub Flow (Feature Branch + PR) 채택

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
프로젝트의 Git 브랜치 전략을 결정해야 합니다.  
주요 고려사항:
- **1인 개발** (현재) + 협업 가능성 (미래)
- **TDD 중심 개발**: Red → Green → Refactor 사이클
- **포트폴리오 목적**: 명확한 커밋 이력과 사고 과정 가시화 필요
- **Phase별 점진적 개발**: Phase 3(C++), Phase 4(Python) 순차 진행
- **CI/CD 통합**: GitHub Actions 이미 구축됨

주요 후보: **Git Flow**, **GitHub Flow**, **Trunk-Based Development**

### 의사결정 (Decision)
**GitHub Flow (Feature Branch + Pull Request)** 방식을 채택합니다.

### 근거 (Rationale)

#### 1️⃣ 세 가지 워크플로우 비교

| 기준 | Git Flow | **GitHub Flow** | Trunk-Based |
|------|----------|-----------------|-------------|
| **브랜치 타입** | 5가지 (main, develop, feature, release, hotfix) | **2가지 (main, feature/*)** | 1가지 (main + 초단기 feature) |
| **학습 곡선** | 복잡 (10일+) | **간단 (1-2일)** ⭐ | 어려움 (숙련 필요) |
| **TDD 호환성** | 보통 | **완벽** ⭐ | 충돌 가능 |
| **1인 개발 적합성** | 과도함 | **최적** ⭐ | 부담스러움 |
| **포트폴리오 가시성** | 보통 | **우수** ⭐ | 낮음 (커밋 이력 추적 어려움) |
| **향후 협업 확장성** | 우수 | **우수** ⭐ | 보통 |
| **CI/CD 통합** | 복잡 | **자연스러움** ⭐ | 매우 자연스러움 |
| **릴리즈 관리** | ✅ 우수 (별도 브랜치) | 보통 (태그 기반) | 어려움 |

#### 2️⃣ TDD와의 완벽한 궁합
```
[RED] 테스트 작성
   ↓ (커밋: test: add filter tests)
[GREEN] 최소 구현
   ↓ (커밋: feat: implement filter)
[REFACTOR] 리팩터링
   ↓ (커밋: refactor: extract helper)
[PR] 리뷰 및 머지
```
→ **TDD 사이클 하나 = Feature Branch 하나**로 자연스럽게 매핑됨

#### 3️⃣ 포트폴리오 효과
- **PR 이력**: "이 사람이 어떻게 문제를 해결하는가"를 명확히 보여줌
- **Self-Review**: 리뷰어가 없어도 PR 설명으로 사고 과정 문서화
- **Draft PR → Ready for Review**: 진행 상황을 투명하게 공개

#### 4️⃣ 미래 확장성
- 협업자 추가 시 추가 학습 없음 (오픈소스 기여 방식과 동일)
- 대부분의 현대 스타트업이 사용하는 표준 방식
- GitHub 네이티브 워크플로우 (플랫폼 기능 최대 활용)

### 대안 및 트레이드오프 (Alternatives)

#### Git Flow (거부됨)
- ✅ **장점**: 
  - 여러 버전 동시 지원 가능 (v1.0 유지보수 + v2.0 개발)
  - 릴리즈 프로세스가 매우 체계적
  - 대규모 팀에 적합
- ❌ **Rejected**: 
  - **과도한 브랜치 관리**: `develop`, `release/*`, `hotfix/*` 브랜치가 1인 개발에는 불필요
  - **Phase 순차 개발**: Phase 3, Phase 4를 순차 개발하므로 동시 버전 관리 불필요
  - **배포 전략 불일치**: Docker Compose로 단일 스택 배포하므로 `release` 브랜치 별도 관리 실익 없음
  - **학습 및 관리 비용**: 5가지 브랜치 타입 학습 및 관리 오버헤드

#### Trunk-Based Development (거부됨)
- ✅ **장점**: 
  - 병합 충돌 최소화
  - 배포 속도 극대화 (하루 여러 번 배포)
  - CI/CD와 완벽한 통합
- ❌ **Rejected**: 
  - **높은 숙련도 요구**: Feature Flag, 빈번한 통합 등 고급 기술 필요
  - **TDD 사이클 충돌**: "몇 시간 내 머지" 권장하지만, TDD Red → Green → Refactor는 하루 이상 소요 가능
  - **포트폴리오 가시성 감소**: 너무 빈번한 커밋으로 "무엇을 했는지" 추적 어려움
  - **강력한 자동화 테스트 필수**: 모든 변경에 대한 완벽한 회귀 테스트 필요 (현재 구축 중)

### 결론 (Consequences)

#### ✅ 장점
1. **학습 곡선**: 1-2일이면 숙달 가능
2. **TDD 호환성**: Red/Green/Refactor → PR로 자연스럽게 매핑
3. **포트폴리오 가치**: PR 이력이 명확한 스토리텔링 제공
4. **협업 확장성**: 표준 프로세스로 팀 확장 용이
5. **CI/CD 통합**: GitHub Actions와 완벽한 궁합
6. **적정 복잡도**: 과하지도 부족하지도 않은 균형

#### ⚠️ 단점
1. **여러 버전 동시 지원 어려움**: 
   - **프로젝트 영향**: 현재 Phase 순차 개발로 불필요 → 영향 없음
2. **릴리즈 관리 제한적**: 
   - **대책**: Git 태그 + GitHub Releases로 충분히 대체 가능

#### 🚀 워크플로우 실행 예시
```bash
# 1. 브랜치 생성
git checkout -b feature/filter-strategy-pattern

# 2. [RED] 테스트 작성
git commit -m "test(preprocess): add filter tests (failing)"
git push -u origin feature/filter-strategy-pattern

# 3. GitHub에서 Draft PR 생성
# Title: "[WIP] Filter Strategy Pattern"
# Checklist: [x] Red, [ ] Green, [ ] Refactor

# 4. [GREEN] 최소 구현
git commit -m "feat(preprocess): implement filter strategy"
git push

# 5. [REFACTOR] 리팩터링
git commit -m "refactor(preprocess): extract filter factory"
git push

# 6. Ready for Review → Merge

# 7. 정리
git checkout main
git pull origin main
git branch -d feature/filter-strategy-pattern
```

#### 📊 채용 시장 관점
- **현업 표준**: 대부분의 스타트업 및 중소 기업에서 GitHub Flow 사용
- **오픈소스**: GitHub 기반 오픈소스 프로젝트의 사실상 표준
- **포트폴리오**: 면접관이 가장 친숙한 워크플로우 → 코드 리뷰 용이

### 관련 문서
- [Git Workflow 가이드](project-guides/git-workflow-guide.md)
- [Git Workflow 실습 가이드](project-guides/git-workflow-integration.md)

---

## ADR-014: AWS EC2와 Docker 연결 구조 (Port Mapping & Bridge Network)

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
AWS EC2 인스턴스 내에서 Docker 컨테이너들이 어떻게 실행되고, 외부 네트워크와 어떻게 연결되는지에 대한 구조적 이해가 필요했습니다. 특히, "EC2도 서버고 Docker도 서버인데 서로 어떻게 연결되는가?"에 대한 혼란을 해소하고, 보안과 효율성을 고려한 연결 방식을 확정해야 했습니다.

### 의사결정 (Decision)
**Port Mapping**과 **Docker Bridge Network**를 조합하여 다음과 같은 연결 구조를 채택합니다:

1.  **외부 연결 (EC2 ↔ Docker)**:
    - AWS Security Group에서 **443 포트(HTTPS)**만 개방합니다.
    - EC2의 443 포트를 **Nginx 컨테이너**의 443 포트로 포워딩합니다 (`ports: - "443:443"`).
    - 그 외의 모든 포트는 외부에서 접근 불가능하게 막습니다.

2.  **내부 연결 (Docker ↔ Docker)**:
    - **Docker Bridge Network**라는 가상의 사설망을 사용합니다.
    - Node.js, C++ 전처리 서버, Python AI 서버는 이 내부망에 연결됩니다.
    - 이들은 호스트(EC2)의 포트를 점유하지 않으며, 오직 **Nginx를 통해서만** 외부 트래픽을 받을 수 있습니다.

### 아키텍처 다이어그램 (Architectural Diagram)

```mermaid
graph TD
    User(("👤 사용자")) -->|"1. URL 접속 (HTTPS)"| Internet["인터넷"]
    
    subgraph AWS_Cloud ["☁️ AWS 클라우드"]
        subgraph EC2_Instance ["🖥️ EC2 리눅스 컴퓨터"]
            style EC2_Instance fill:#f9f9f9,stroke:#333,stroke-width:4px
            
            subgraph Security_Group ["🛡️ 방화벽(Security Group)"]
                direction TB
                Port443["포트 443 (HTTPS) 열림"]
            end

            subgraph Docker_Engine ["🐳 도커 (실행 환경)"]
                style Docker_Engine fill:#e3f2fd,stroke:#2496ed,stroke-width:2px
                
                subgraph Docker_Compose ["Docker Compose 묶음"]
                    style Docker_Compose fill:#e8f5e9,stroke:#4caf50,stroke-width:2px,stroke-dasharray: 5 5
                    
                    Nginx["👮 Nginx (리버스 프록시)<br/>외부 요청을 받는 문지기"]
                    
                    Node["🚀 Node.js Gateway<br/>(메인 서버)"]
                    Cpp["⚙️ C++ 전처리<br/>(내부 작업자 1)"]
                    Py["🧠 Python AI<br/>(내부 작업자 2)"]
                    
                    %% 내부 통신
                    Port443 -->|"전달"| Nginx
                    
                    Nginx -->|"2. 내부 전달 (HTTP)"| Node
                    Node -->|"3. 작업 요청"| Cpp
                    Node -->|"3. 작업 요청"| Py
                end
            end
        end
    end
```

### 근거 (Rationale)

#### 1️⃣ 보안성 (Security via Isolation)
- **제1원칙**: "보안 사고는 공격 표면(Attack Surface)을 줄일수록 예방된다."
- 내부 서버(Node.js, C++, Python)는 오직 Docker 내부망에만 존재하므로, 외부 해커가 직접 접속할 경로가 원천적으로 차단됩니다.
- 오직 검증된 **Nginx(Reverse Proxy)**만이 유일한 출입구 역할을 수행합니다.

#### 2️⃣ 이식성 (Portability via Abstraction)
- `docker-compose.yml` 파일 하나에 전체 네트워크 토폴로지가 정의됩니다.
- EC2 인스턴스가 바뀌거나 로컬 개발 환경으로 이동해도, 네트워크 설정은 동일하게 유지됩니다 ("Write Once, Run Anywhere").

#### 3️⃣ 관리 단순성 (Simplicity)
- 복잡한 AWS VPC 설정이나 라우팅 테이블 변경 없이, Docker 수준에서 논리적인 망 분리가 가능합니다.
- 포트 충돌 문제(Port Conflict)를 걱정할 필요가 없습니다 (내부망에서는 각 컨테이너가 독립 IP를 가짐).

### 대안 및 트레이드오프 (Alternatives)
- **Host Network Mode**: 컨테이너가 호스트의 네트워크 스택을 공유.
  - ❌ **Rejected**: 포트 충돌 위험이 있고, 보안 격리가 약해짐.
- **Port Mapping for All Services**: 모든 서비스의 포트(3000, 8081, 8082)를 외부로 노출.
  - ❌ **Rejected**: 보안상 매우 취약하며, 불필요한 공격 표면을 노출함.
- **Cloudflare Workers & Vercel**: "유명 웹 개발자(노마드코더)가 추천하는 무료 백엔드 툴"이라서 고려.
  - ❌ **Rejected (Fundamental Mismatch)**:
    - **실체**: JS 런타임(Workers)이나 Serverless 함수(Vercel)로, 가볍고 빠른 웹 요청 처리에 최적화됨.
    - **제약 1 (OS 부재)**: 리눅스 OS 제어권이 없어 **C++ OpenCV**나 **Python PyTorch** 같은 네이티브 라이브러리 설치 불가.
    - **제약 2 (리소스 제한)**: Vercel 함수 용량 250MB 제한(PyTorch 모델 초과), Workers CPU 시간 10ms 제한(이미지 분석 2~5초 불가).
    - **결론**: 본 프로젝트의 무거운 AI 연산(Deep Learning Inference)에는 **AWS EC2(지속 실행형 컴퓨터)**가 필수불가결함.

### 결론 (Consequences)
- ✅ **장점**: 강력한 보안 격리, 환경 일관성, 구성 관리의 단순화.
- ⚠️ **단점**: 디버깅 시 내부 컨테이너에 직접 접속하려면 `docker exec` 등을 사용해야 함 (약간의 불편함).

---

---

## ADR-015: 하이브리드 배포 전략 (Local GPU 개발 vs EC2 CPU 배포)

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
개발 환경(Local Windows)에는 NVIDIA GPU(RTX 3060 등)가 있어 CUDA 가속이 가능하지만, 배포 환경(AWS EC2)은 비용 문제로 인해 **GPU 인스턴스(g4dn.xlarge, 월 $300+)** 사용이 부담스러운 상황입니다. 

"로컬에서 개발한 CUDA 코드를 EC2 일반 인스턴스(t3.medium, 월 $30)에 올리면 GPU를 못 쓰는 것 아닌가?"라는 우려와 함께, **비용 효율적인 AI 서빙 전략** 수립이 필요했습니다.

### 의사결정 (Decision)
**하이브리드 전략(Hybrid Strategy)**을 채택합니다:

1.  **개발(Development) & 학습(Training)**: 
    - **Local GPU 활용**: Windows/WSL2 환경에서 NVIDIA GPU를 사용하여 빠른 모델 학습 및 프로토타이핑 수행.
    - **Docker Support**: `nvidia-docker`를 사용하여 로컬에서는 GPU 컨테이너를 구동.

2.  **배포(Deployment) & 추론(Inference)**:
    - **EC2 CPU 활용**: 초기 단계(Phase 1~4)에서는 값비싼 GPU 인스턴스 대신, 저렴한 **t3.medium (vCPU 2, RAM 4GB)** 인스턴스를 기본으로 사용.
    - **ONNX Runtime 최적화**: PyTorch 모델을 ONNX로 변환하여 CPU 추론 속도를 극대화 (약 2~3배 향상 예상).
    - **확장 및 비용 최적화 계획 (Future Work)**: 
      - 현재는 안정성을 위해 4GB 램을 확보하지만, 최적화 수준에 따라 **t3.small (RAM 2GB)**로 다운그레이드 고려.
      - 테스트 환경에는 **Spot Instance** (70~90% 할인) 적극 도입.
      - 장기적으로 **Oracle Cloud Free Tier (ARM 4 Core, 24GB RAM)**로의 마이그레이션 가능성 열어둠.

### 근거 (Rationale)

#### 1️⃣ 비용 효율성 (Cost Efficiency)
- **EC2 g4dn.xlarge (GPU)**: 시간당 약 $0.526 (서울 리전) → 월 **약 45~50만 원**.
- **EC2 t3.medium (CPU)**: 시간당 약 $0.052 → 월 **약 4~5만 원** (초기 확정).
- **EC2 t2.micro (Free Tier)**: RAM 1GB로 PyTorch 모델 구동 불가 (OOM 발생).
- 초기 스타트업/토이 프로젝트 단계에서 10배 넘는 비용 차이는 감당하기 어려움.

#### 2️⃣ 기술적 타당성 (Technical Feasibility)
- **제1원칙**: "소프트웨어 최적화로 하드웨어 제약을 극복한다."
- 이미지 분석(EfficientNet-B0/B2)은 **상대적으로 가벼운 모델**입니다.
- **ONNX Runtime + Quantization(INT8)** 적용 시, CPU에서도 **장당 200~500ms** 이내 처리가 가능할 것으로 예상됩니다 (목표: < 1초).

#### 3️⃣ 로컬 GPU의 활용 (Development Velocity)
- 개발자는 로컬 GPU의 강력한 성능을 이용해 모델을 빠르게 학습시키고 튜닝할 수 있습니다.
- 배포 시에는 모델 파일(.onnx)만 서버로 전송하므로, 개발 생산성은 유지하면서 배포 비용은 절감됩니다.

### 대안 및 트레이드오프 (Alternatives)
- **AWS g4dn 인스턴스 사용**: 처음부터 GPU 서버 대여.
  - ❌ **Rejected**: 비용 과다. 트래픽이 적은 초기에는 낭비.
- **AWS Lambda (Serverless Inference)**: 요청 시에만 과금.
  - ❌ **Rejected**: Cold Start 문제(모델 로딩 10초+), PyTorch/Opencv 라이브러리 용량 제한(250MB).

### 결론 (Consequences)
- ✅ **장점**: 압도적인 비용 절감, 유연한 확장성.
- ⚠️ **단점**: CPU 추론 최적화(ONNX 변환) 작업이 추가로 필요함.
- **실행 계획**: Phase 4(Python AI Server) 단계에서 `torch.onnx.export` 및 `onnxruntime` 적용 필수.

---

## ADR-016: Antigravity Awesome Skills 도입 및 코드베이스 현대화

### 상태
✅ **Accepted** (2026-02)

### 컨텍스트 (Context)
프로젝트 규모가 커짐에 따라 TypeScript, React, Node.js, C++ 등 멀티 언어 환경에서의 품질 관리와 모범 사례(Best Practices) 준수가 중요해졌습니다. 단순한 기능 구현을 넘어, 시니어 수준의 엔지니어링 표준을 유지하기 위해 전문화된 가이드라인 시스템 도입이 필요했습니다.

### 의사결정 (Decision)
**Antigravity Awesome Skills**를 도입하고, 이를 핵심 기술 스택에 맞춰 커스터마이징하여 적용합니다.

1.  **스킬 선택 및 설치**:
    - `typescript-expert`: 고급 타입 패턴 및 성능 최적화
    - `react-best-practices`: 렌더링 최적화 및 구조화
    - `nodejs-best-practices`: 백엔드 아키텍처 및 보안
    - `cpp-pro`: 현대적 C++ 표준 준수 (C++17로 커스터마이징)

2.  **프로젝트 규칙 우선 순위**:
    - 프로젝트 고유의 `code-style-guide.md` (TDD, Tidy First)를 최우선으로 하며, 스킬셋은 이를 보완하는 가이드라인으로 활용함.

### 개선 내역 (Improvements)

#### 1️⃣ api-gateway (Node.js/TypeScript)
- **타입 안전성**: `any` 타입을 제거하고 `Error`, `unknown` 및 `instanceof` 기반의 런타임 타입 가드를 도입함.
- **로깅 표준화**: `console.log` 사용을 금지하고 기존 `winston` 로거로 통합하여 운영 환경 가시성을 확보함.
- **비동기 비차단(Non-blocking)**: `fs.writeFileSync` 등 동기 메서드를 `fs.promises.writeFile`로 전환하여 이벤트 루프 차단을 방지함.
- **Node.js 현대화**: 내장 모듈 임포트 시 `node:` 프리픽스를 명시적으로 사용함.

#### 2️⃣ frontend (React/TypeScript)
- **렌더링 최적화**: `&&` 연산자를 이용한 조건부 렌더링을 삼항 연산자(`? : null`)로 전환하여 falsy 값(0, "")의 의도치 않은 렌더링 버그를 방지함.

#### 3️⃣ 공통 (tsconfig)
- **엄격한 타입 체크**: `noUncheckedIndexedAccess`, `noImplicitOverride` 옵션을 활성화하여 런타임 에러 가능성을 컴파일 단계에서 차단함.

### 근거 (Rationale)
- **품질 일관성**: AI 어시스턴트가 코드를 작성할 때 일관된 시니어 수준의 코드를 출력하도록 강제함.
- **안정성**: `strict` 옵션 강화를 통해 잠재적인 undefined 접근을 사전에 차단함.
- **성능**: 이벤트 루프 비차단 및 React 렌더링 최적화 규칙 적용으로 엔드투엔드 성능 개선.

### 결론 (Consequences)
- ✅ **장점**: 기술 부채 예방, 시니어 수준의 코드 품질 유지, 타입 안전성 극대화.
- ⚠️ **단점**: `noUncheckedIndexedAccess` 도입으로 인해 배열 인덱스 접근 시 추가적인 검증 코드가 필요함 (약간의 코드량 증가).

---

**마지막 업데이트**: 2026-02-16  

**작성자**: 정태민 
**참고 문서**: 
- [implementation_plan.md](../.gemini/antigravity/brain/25394dd5-b6fa-4aac-8d14-3ffee0e6e7f0/implementation_plan.md)
- [walkthrough.md](../.gemini/antigravity/brain/25394dd5-b6fa-4aac-8d14-3ffee0e6e7f0/walkthrough.md)
- [code-style-guide.md](../.agent/rules/code-style-guide.md)
- [ADR-013: GitHub Flow 채택](#adr-013-git-workflow로-github-flow-feature-branch--pr-채택)

