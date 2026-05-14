# ADR-001 보충: AWS Lambda 서버리스 대안 구현 방식과 한계

> 이 문서는 `ARCHITECTURE_DECISIONS.md`의 ADR-001에서 기각한 대안인
> **"서버리스 아키텍처(AWS Lambda): 각 단계를 Lambda 함수로 분리"**를
> 실제로 구현한다면 어떤 구조가 되는지, 그리고 Mind Palette 프로젝트에서는
> 왜 한계가 커지는지를 설명한다.

## 1. 구현 가능한 서버리스 구조

현재의 Mind Palette 구조는 다음과 같은 마이크로서비스 파이프라인이다.

```text
Frontend
  -> API Gateway(Node.js)
  -> C++ Preprocess Server
  -> Python AI Server
  -> API Gateway
  -> Frontend
```

이를 Lambda 기반으로 바꾸면, 상시 실행 서버를 두는 대신 요청 단계를 여러
Lambda 함수로 나누고, 파일은 S3에 저장하며, 전체 흐름은 Step Functions로
오케스트레이션하는 방식이 가장 현실적이다.

```text
Frontend
  -> API Gateway
  -> Request Lambda
  -> S3 원본 이미지 업로드
  -> Step Functions 워크플로우 시작
      -> Validate Lambda
      -> Preprocess Lambda
      -> Inference Lambda
      -> Persist Result Lambda
  -> DynamoDB/S3 결과 저장
  -> Frontend가 jobId로 polling 또는 WebSocket/SSE로 결과 확인
```

## 2. 단계별 역할 매핑

| 단계 | Lambda 역할 | 현재 구조와의 대응 |
| --- | --- | --- |
| `Request Lambda` | 업로드 URL 발급, `jobId` 생성, 메타데이터 저장 | API Gateway 일부 |
| `Validate Lambda` | 파일 크기, MIME, magic number, 해시 검사 | API Gateway 보안 검증 일부 |
| `Preprocess Lambda` | OpenCV 기반 리사이즈, 이진화, contour 추출, 품질 메트릭 생성 | `preprocess-server/` |
| `Inference Lambda` | 모델 로딩 후 추론, 점수/라벨 계산 | `ai-server/` |
| `Persist Result Lambda` | 결과 JSON 저장, 상태 업데이트 | Gateway orchestration 일부 |

Lambda는 기본적으로 stateless 실행 환경이므로, 현재 `shared_volume/`처럼
로컬 공유 볼륨에 파일을 놓고 경로만 넘기는 방식은 적합하지 않다. 대신
S3 object key를 단계 간 계약으로 넘긴다.

```json
{
  "jobId": "job_123",
  "sourceImage": {
    "bucket": "mind-palette-uploads",
    "key": "uploads/job_123/original.png"
  }
}
```

예상되는 산출물 경로는 다음과 같다.

```text
s3://mind-palette-uploads/uploads/job_123/original.png
s3://mind-palette-results/jobs/job_123/preprocessed.png
s3://mind-palette-results/jobs/job_123/features.json
s3://mind-palette-results/jobs/job_123/result.json
```

## 3. C++ 전처리 Lambda 구현 방식

C++ 전처리 서버를 Lambda로 옮길 경우, Crow 기반 HTTP 서버를 그대로 띄우는
방식보다는 Lambda handler가 C++ 실행 파일을 호출하는 구조가 더 자연스럽다.

```text
Preprocess Lambda container image
  - Amazon Linux base image
  - OpenCV native library
  - C++ preprocess binary
  - Lambda Runtime Interface Client
  - handler:
      1. S3에서 원본 이미지 다운로드
      2. /tmp/input.png에 저장
      3. C++ preprocess binary 실행
      4. 결과 이미지와 features.json을 S3에 업로드
```

OpenCV와 native dependency가 포함되므로 ZIP 패키지보다 Lambda container image
방식이 현실적이다. 다만 이 경우에도 컨테이너 이미지 빌드, ECR 배포, Amazon
Linux 기반 native library 호환성 관리가 필요하다.

## 4. Python AI 추론 Lambda 구현 방식

Python AI 서버도 FastAPI 앱을 장시간 띄우는 방식이 아니라 Lambda handler
중심으로 재구성해야 한다.

```python
model = None

def load_model_once_if_warm():
  global model
  if model is None:
    model = load_model()
  return model

def handler(event, context):
  model = load_model_once_if_warm()
  image = download_from_s3(event["preprocessedImage"])
  result = model.predict(image)
  upload_result_to_s3(result)
  return {
    "jobId": event["jobId"],
    "status": "DONE"
  }
```

전역 변수에 모델을 올려두면 warm start에서는 재사용될 수 있다. 그러나 Lambda
실행 환경은 언제든 새로 생성될 수 있으므로, 모델 재사용은 보장된 계약이
아니다. cold start에서는 런타임 초기화, 라이브러리 로딩, 모델 weight 로딩이
다시 발생한다.

## 5. 필요한 AWS 리소스

완전한 Lambda 기반 파이프라인은 대략 다음 리소스를 필요로 한다.

```text
API Gateway
S3 uploads bucket
S3 results bucket
DynamoDB jobs table
Step Functions state machine
Lambda: request
Lambda: validate
Lambda: preprocess
Lambda: inference
Lambda: persist
CloudWatch Logs
ECR repositories
IAM roles/policies
```

SAM, CDK, Terraform 중 하나로 인프라를 정의할 수 있다. 예를 들면
`PreprocessFunction`, `InferenceFunction`, `AnalysisWorkflow`를 만들고,
Step Functions가 각 Lambda를 순서대로 호출하게 구성한다.

## 6. Mind Palette에서의 주요 한계

### 6.1 Cold Start 지연

Mind Palette의 무거운 단계는 OpenCV 전처리와 AI 추론이다. 이 단계들은
다음 초기화 비용을 가진다.

- OpenCV native library 로딩
- PyTorch, ONNX Runtime, NumPy 등 ML dependency 로딩
- 모델 weight 로딩
- 컨테이너 이미지 기반 Lambda의 실행 환경 초기화

AWS Lambda의 Provisioned Concurrency를 사용하면 cold start를 줄일 수 있지만,
미리 실행 환경을 띄워두는 비용이 발생한다. 이 경우 "요청이 있을 때만 과금"되는
서버리스의 장점이 약해진다.

### 6.2 AI 추론 서버의 모델 재사용성과 충돌

현재 Python AI Server 구조는 서버 시작 시 모델을 한 번 로딩하고, 이후 요청마다
메모리에 올라간 모델을 재사용할 수 있다.

```text
서버 시작
  -> 모델 로딩
  -> 메모리 상주
  -> 요청마다 빠르게 추론
```

Lambda 구조에서는 warm start 환경에서만 이와 비슷한 효과를 기대할 수 있다.

```text
cold start
  -> 런타임 초기화
  -> 라이브러리 로딩
  -> 모델 로딩
  -> 추론
```

따라서 추론 요청의 latency가 안정적으로 유지되어야 하는 서비스에는 상시 실행
모델 서버가 더 예측 가능하다.

### 6.3 GPU/TensorRT 전략과의 불일치

이 프로젝트는 AI 서버에서 EfficientNet 계열 모델과 TensorRT FP16 최적화를
고려한다. 이런 구조는 CUDA/TensorRT가 가능한 GPU 실행 환경과 잘 맞는다.

반면 Lambda는 일반적인 GPU 인스턴스처럼 CUDA/TensorRT 추론 서버를 구성하는
환경이 아니다. GPU 추론이 중요해지면 Lambda보다 다음 선택지가 더 자연스럽다.

- EC2 GPU 인스턴스
- ECS/EKS GPU workload
- SageMaker Endpoint
- AWS Batch GPU job

### 6.4 파일 전달과 I/O 비용 증가

현재 Docker Compose 기반 MSA에서는 Docker 내부 네트워크와 공유 볼륨으로
파일 경로 기반 통신을 구성할 수 있다. Lambda에서는 함수 간 로컬 디스크를
공유할 수 없으므로 S3 또는 EFS를 사용해야 한다.

| 선택지 | 장점 | 한계 |
| --- | --- | --- |
| S3 | 가장 일반적이고 내구성이 높음 | 단계마다 GET/PUT 지연, 요청 비용 발생 |
| EFS | 공유 파일시스템처럼 사용 가능 | VPC/EFS 구성 복잡도, 비용, 성능 튜닝 필요 |
| `/tmp` | 함수 내부 임시 작업에 단순함 | 함수 간 공유 불가, 영속성 보장 불가 |

이미지 파이프라인에서는 원본 이미지, 전처리 이미지, feature JSON, 분석 결과가
계속 이동하므로 I/O 비용과 latency가 누적될 수 있다.

### 6.5 로컬 개발 복잡도 증가

현재 구조는 Docker Compose로 로컬 재현성이 높다.

```bash
docker compose up
```

Lambda 기반에서는 다음 요소를 로컬에서 흉내 내야 한다.

- API Gateway event
- S3 event 및 object storage
- Step Functions state transition
- DynamoDB 상태 저장
- IAM permission
- CloudWatch Logs
- Lambda container runtime

SAM Local, LocalStack, moto 등을 조합할 수 있지만 실제 AWS IAM, S3,
Step Functions 동작과 완전히 같지는 않다. 특히 이미지 파일, 대용량 모델,
native library가 얽히면 로컬 디버깅 난이도가 크게 올라간다.

### 6.6 비용 예측의 어려움

Lambda 하나만 보면 비용 계산이 단순하지만, 이미지 분석 파이프라인으로 확장하면
비용 항목이 늘어난다.

```text
Lambda 실행 시간 x 메모리
Lambda 요청 수
Step Functions state transition 수
S3 PUT/GET 요청 수
S3 저장 용량
DynamoDB read/write
CloudWatch Logs 저장량
ECR 저장 용량
Provisioned Concurrency 비용
VPC 사용 시 NAT Gateway 비용
```

트래픽이 매우 적으면 저렴할 수 있지만, 이미지 처리 시간이 길고 메모리를 크게
잡아야 하거나 Provisioned Concurrency를 켜면 EC2 단일 인스턴스보다 비용 구조가
덜 직관적일 수 있다.

## 7. Lambda가 적합할 수 있는 경우

다음 조건이라면 Lambda 기반도 고려할 수 있다.

- 처리 시간이 짧다.
- 모델이 작고 CPU 추론으로 충분하다.
- 결과를 비동기로 받아도 된다.
- 트래픽이 매우 드물거나 스파이크성이다.
- S3 기반 파일 흐름을 받아들일 수 있다.
- 운영자가 AWS serverless, IAM, CDK/SAM에 익숙하다.

예를 들어 업로드된 이미지를 단순 리사이즈하고 썸네일만 만드는 서비스라면
Lambda가 잘 맞을 수 있다. 그러나 Mind Palette처럼 OpenCV 전처리, AI 추론,
민감 이미지 처리, 결과 일관성, 로컬 재현성이 모두 중요한 시스템에서는 현재의
Docker Compose 기반 MSA가 더 단순하고 예측 가능하다.

## 8. 현실적인 절충안

완전한 Lambda 전환보다 더 현실적인 절충안은 Lambda를 무거운 연산 실행 환경이
아닌 가벼운 오케스트레이션 계층으로 사용하는 것이다.

```text
API Gateway / 가벼운 orchestration: Lambda 가능
무거운 전처리/추론: 기존 C++/Python 컨테이너 유지
비동기 큐: SQS 또는 Step Functions 추가 가능
파일 저장: S3 도입 가능
```

즉 Lambda는 업로드 접수, job 상태 관리, 이벤트 연결에 사용하고, OpenCV 전처리와
AI 추론은 ECS/Fargate, EC2, SageMaker 같은 컨테이너/모델 서버 환경에 두는
방식이 Mind Palette의 요구사항과 더 잘 맞는다.

## 9. 참고 문서

- [AWS Lambda: function timeout](https://docs.aws.amazon.com/lambda/latest/dg/configuration-timeout.html)
- [AWS Lambda: memory and computing power](https://docs.aws.amazon.com/lambda/latest/operatorguide/computing-power.html)
- [AWS Lambda: ephemeral storage](https://docs.aws.amazon.com/lambda/latest/dg/configuration-ephemeral-storage.html)
- [AWS Lambda: container images](https://docs.aws.amazon.com/lambda/latest/dg/lambda-images.html)
- [AWS Step Functions documentation](https://aws.amazon.com/documentation-overview/step-functions/)
- [AWS Step Functions pricing](https://aws.amazon.com/step-functions/pricing/)
