# 하네스 엔지니어링 가이드 (Harness Engineering)

> 참고: [Channel.io — 하네스 엔지니어링이란?](https://channel.io/ko/blog/articles/what-is-harness-2611ddf1)  
> 작성 기준 프로젝트: Mind Palette (ai-server)

---

## 개념

하네스(Harness)는 말의 마구에서 유래한 용어로, **AI를 안전하고 예측 가능한 방식으로 제어하면서 최대한 활용하기 위한 구조**입니다.  
자동차의 핸들과 안전벨트처럼, AI 활용 속도를 늦추지 않으면서도 안정성을 확보하는 역할을 합니다.

---

## 핵심 구성 요소 3가지

| 요소 | 역할 |
|------|------|
| **가드레일 (Guardrail)** | 입출력 단계에서 부적절한 동작을 사전 차단 |
| **데이터 거버넌스** | 민감 정보 보호, 역할별 접근 권한 제어, 컴플라이언스 |
| **모니터링 & 피드백 루프** | 실시간 동작 추적, 오류 감지, 지속적 개선 |

---

## Mind Palette 기준 분석

### 구조 개요: 현재 vs 하네스 적용 후

```
[현재]
요청 → 입력검증 → 모델추론 → 결과반환

[하네스 적용 후]
요청 → [입력 가드레일] → 모델추론 → [출력 가드레일] → [감사 로그] → 결과반환
                                              ↓
                                    [모니터링/피드백 루프]
```

---

## 1. 가드레일 (Guardrail)

### 1-1. 입력 가드레일

**현재 코드 (`ai-server/src/routes/analyze.py`):**
```python
def _validate_image_file(file: UploadFile, content: bytes) -> None:
    if len(content) == 0:                              # 빈 파일 체크
        raise HTTPException(...)
    if file.content_type not in _ALLOWED_CONTENT_TYPES: # MIME 타입 체크
        raise HTTPException(...)
    if not _is_valid_image_bytes(content):             # 매직 바이트 체크
        raise HTTPException(...)
```

**하네스 적용 시 추가되어야 할 것:**
```python
_MAX_FILE_SIZE_BYTES = 10 * 1024 * 1024  # 10MB 상한
_MIN_IMAGE_DIMENSION = 50                # 너무 작은 이미지 (픽셀)
_MAX_IMAGE_DIMENSION = 8000              # 너무 큰 이미지
_VALID_AGE_RANGE = (3, 18)              # HFD 검사 대상 아동 나이

def _validate_input_guardrail(file: UploadFile, content: bytes, age: int) -> None:
    # 파일 크기 상한
    if len(content) > _MAX_FILE_SIZE_BYTES:
        raise HTTPException(400, "파일 크기가 허용 한도를 초과합니다.")

    # 이미지 해상도 범위
    img = Image.open(io.BytesIO(content))
    w, h = img.size
    if w < _MIN_IMAGE_DIMENSION or h < _MIN_IMAGE_DIMENSION:
        raise HTTPException(400, "이미지 해상도가 너무 낮습니다.")
    if w > _MAX_IMAGE_DIMENSION or h > _MAX_IMAGE_DIMENSION:
        raise HTTPException(400, "이미지 해상도가 너무 높습니다.")

    # 도메인 규칙: 나이 유효성 (HFD 검사 대상)
    if not (_VALID_AGE_RANGE[0] <= age <= _VALID_AGE_RANGE[1]):
        raise HTTPException(400, f"HFD 검사 대상 나이 범위 초과: {age}세")
```

---

### 1-2. 출력 가드레일 (현재 완전히 없음 — 가장 중요)

현재 코드는 모델 출력을 **검증 없이 그대로 반환**합니다.  
아동 IQ 측정 결과가 잘못 나왔을 때 그대로 출력하면 **임상적 피해**가 발생할 수 있습니다.

**하네스 적용 시:**
```python
_IQ_VALID_RANGE = (40, 160)       # Goodenough-Harris 검사 유효 IQ 범위
_PERCENTILE_VALID_RANGE = (0, 100)
_MIN_CONFIDENCE_THRESHOLD = 0.6   # 모델 신뢰도 최소치

def _validate_output_guardrail(result: dict, outputs: tuple) -> dict:
    """AI 출력이 도메인 규칙에 맞는지 검증한다."""

    # 1. IQ 범위 검증
    iq = result.get("iq")
    if iq is not None:
        if not (_IQ_VALID_RANGE[0] <= iq <= _IQ_VALID_RANGE[1]):
            logger.warning("IQ out of valid range — flagged",
                           iq=iq, valid_range=_IQ_VALID_RANGE)
            result["flagged"] = True
            result["flag_reason"] = f"IQ {iq}은 유효 범위({_IQ_VALID_RANGE}) 밖입니다."

    # 2. Head score 상한 검증 (문항 수 초과 불가)
    head_limits = {"head_a": 19, "head_b": 14, "head_c": 16, "head_d": 11}
    for head, limit in head_limits.items():
        score = result["head_scores"].get(head, 0)
        if score > limit:
            raise OutputIntegrityError(f"{head} 점수 {score}가 최대값 {limit} 초과")

    # 3. 모델 신뢰도 임계값 (확률값이 전부 0.5 근처면 불확실한 예측)
    all_probs = _get_all_probabilities(outputs)
    avg_confidence = np.mean(np.abs(all_probs - 0.5)) * 2  # 0=불확실, 1=확실
    if avg_confidence < _MIN_CONFIDENCE_THRESHOLD:
        result["low_confidence"] = True
        result["confidence_score"] = round(float(avg_confidence), 3)

    return result
```

---

## 2. 데이터 거버넌스

### 2-1. 감사 로그 (Audit Trail) — 현재 없음

현재 로깅은 **운영 로그**지 **감사 로그**가 아닙니다.

| 구분 | 운영 로그 (현재) | 감사 로그 (하네스) |
|------|----------------|-----------------|
| 목적 | 디버깅, 성능 | 책임 추적, 규정 준수 |
| 보존 기간 | 단기 (rotate) | 장기 (법적 보존) |
| 내용 | 에러, 타이밍 | 누가/언제/무엇을 |
| 변조 방지 | 없음 | append-only |

```python
class AuditLogger:
    """변조 불가 감사 로그 — 규정 준수 및 책임 추적용."""

    def log_analysis_request(
        self,
        request_id: str,
        api_key_hash: str,     # 실제 키가 아닌 해시만 저장
        child_age: int,
        child_gender: str,
        filename_hash: str,    # 파일명 해시 (원본 저장 안 함)
        result_summary: dict,  # IQ, 백분위만 (60문항 전체 아님)
        flagged: bool,
    ) -> None:
        audit_entry = {
            "event": "analysis_completed",
            "timestamp": datetime.utcnow().isoformat(),
            "request_id": request_id,
            "requester_hash": api_key_hash,
            # 아동 개인정보: 최소화 원칙
            "child_age": child_age,
            "child_gender": child_gender,
            "file_hash": filename_hash,  # 원본 이미지 식별만 가능
            # 결과: 요약만
            "iq": result_summary.get("iq"),
            "flagged": flagged,
        }
        self._audit_log.write(json.dumps(audit_entry) + "\n")
```

### 2-2. 최소 수집 원칙 — 현재 위반

**현재 코드 문제:**
```python
# 60문항 전체 + IQ + 아동정보 통째로 로그 → 민감 정보 과다 수집
logger.info("Inference completed successfully",
            request_id=...,
            result=analysis_result)
```

**하네스 적용 시:**
```python
# 운영 로그: 요약만
logger.info("Inference completed",
            request_id=request_id,
            raw_score=raw_score,
            iq_band="normal" if 85 <= iq <= 115 else "flagged",  # 구체적 수치 대신 밴드
            duration_ms=duration_ms,
            low_confidence=result.get("low_confidence", False))

# 감사 로그: 별도 기록
audit_logger.log_analysis_request(...)
```

---

## 3. 모니터링 & 피드백 루프

### 3-1. 미들웨어 수준 이상 감지

**현재 미들웨어** (`ai-server/src/main.py`)는 request_id만 붙입니다.

**하네스 적용 시:**
```python
@app.middleware("http")
async def harness_middleware(request: Request, call_next):
    request_id = request.headers.get("X-Request-ID", str(uuid4()))
    start_time = time.perf_counter()

    structlog.contextvars.bind_contextvars(request_id=request_id)

    response = await call_next(request)

    duration_ms = (time.perf_counter() - start_time) * 1000
    status = response.status_code

    # 이상 감지: 응답 시간 급증 = 모델/시스템 이상 신호
    if duration_ms > 5000:
        logger.warning("Slow inference detected",
                       duration_ms=duration_ms, path=request.url.path)
        metrics.increment("slow_inference_count")

    # 에러율 추적
    if status >= 400:
        metrics.increment(f"error_{status}_count")

    response.headers["X-Request-ID"] = request_id
    return response
```

### 3-2. 피드백 루프 — 현재 완전히 없음 (구조적 공백)

```
[현재]  요청 → 추론 → 반환  (끝. 결과가 맞았는지 알 방법 없음)

[하네스]
요청 → 추론 → 반환
                ↓
        [결과 임시 저장 + request_id 반환]
                ↓
        (나중에) 전문가 검토 결과 수신 API
                ↓
        [불일치 감지 → 재학습 데이터 후보 태깅]
                ↓
        [모델 드리프트 리포트 → 재학습 트리거]
```

```python
@router.post("/feedback/{request_id}")
async def receive_expert_feedback(
    request_id: str,
    expert_iq: int,        # 전문가(심리사)가 실제 판정한 IQ
    expert_notes: str = "",
):
    """전문가 피드백을 수신하여 모델 성능 추적에 활용."""
    stored = await result_store.get(request_id)
    if not stored:
        raise HTTPException(404, "결과를 찾을 수 없습니다.")

    ai_iq = stored["iq"]
    discrepancy = abs(ai_iq - expert_iq) if ai_iq else None

    # 불일치 크면 재학습 후보 태깅
    if discrepancy and discrepancy > 15:
        await retraining_queue.enqueue({
            "request_id": request_id,
            "ai_prediction": ai_iq,
            "ground_truth": expert_iq,
            "priority": "high" if discrepancy > 30 else "normal",
        })
        logger.warning("Large AI-expert discrepancy",
                       discrepancy=discrepancy, request_id=request_id)

    metrics.record_accuracy(ai_iq, expert_iq)
    return {"status": "feedback_recorded"}
```

---

## 자가 진단 체크리스트

미래 프로젝트에서 하네스 엔지니어링 적용 여부를 확인할 때 사용하세요.

| 영역 | 체크 항목 |
|------|-----------|
| **입력 가드레일** | 형식/크기/MIME 검증 |
| | 도메인 규칙 검증 (유효 범위 등) |
| | Rate limiting |
| **출력 가드레일** | AI 결과 범위 검증 |
| | 신뢰도 임계값 |
| | 비정상 결과 플래그 처리 |
| **데이터 거버넌스** | 감사 로그 (Audit Trail) — 누가/언제/무엇을 |
| | 최소 수집 원칙 (민감 정보 요약만 저장) |
| | 접근 권한 분리 |
| **모니터링** | 구조화 로깅 + request_id |
| | 성능 측정 (응답 시간) |
| | 에러율/이상 감지 |
| **피드백 루프** | 결과 저장 + 추적 가능성 |
| | 전문가/사용자 피드백 수신 경로 |
| | 모델 드리프트 감지 |

---

## 핵심 인사이트

> 하네스 엔지니어링은 **"AI가 잘못됐을 때 어떻게 잡고, 어떻게 고치는가"** 에 대한 구조입니다.

- **출력 가드레일**과 **피드백 루프**는 AI가 포함된 모든 프로젝트에서 **설계 초기부터** 고려해야 합니다.
- 나중에 추가하면 아키텍처 전체를 건드려야 하는 경우가 많습니다.
- 특히 의료·교육·금융처럼 **도메인 파급력이 큰 AI**일수록 출력 가드레일의 중요도가 입력 가드레일과 동등하거나 더 높습니다.
