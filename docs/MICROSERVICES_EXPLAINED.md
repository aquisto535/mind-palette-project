# 마이크로서비스 구조 - 구체적인 이해

## 1. "한 부분을 바꿔도 다른 부분에 영향이 없다"는 것의 의미

### 예시 1: C++ 전처리 서버를 Python으로 교체하고 싶다면?

**기존 구조 (현재):**
```
React → API Gateway → C++ 전처리 서버 → AI 서버
```

**변경 시나리오:** "C++는 너무 복잡해. Python OpenCV로 바꾸고 싶어!"

**필요한 작업:**
1. Python으로 새 전처리 서버 작성 (포트 8081 유지)
2. `/preprocess` 엔드포인트와 요청/응답 형식만 동일하게 유지
   ```json
   요청: { "imagePath": "/shared/uploads/image.jpg" }
   응답: { "processedPath": "/shared/processed/image_clean.jpg" }
   ```

**영향받지 않는 부분:**
- ✅ React 프론트엔드 (전혀 수정 불필요)
- ✅ API Gateway (전혀 수정 불필요)
- ✅ AI 서버 (전혀 수정 불필요)

**왜 가능한가?**
- API Gateway는 "누가 처리하는지" 모릅니다
- 단지 `http://localhost:8081/preprocess`로 요청을 보낼 뿐
- 응답만 약속된 형식이면 됩니다

---

### 예시 2: 프론트엔드를 Vue.js로 교체하고 싶다면?

**변경 시나리오:** "React 대신 Vue.js로 UI 만들고 싶어!"

**필요한 작업:**
1. Vue.js로 새 프론트엔드 작성
2. API Gateway의 엔드포인트만 호출
   ```typescript
   POST /analyze (multipart/form-data)
   GET /health
   ```

**영향받지 않는 부분:**
- ✅ API Gateway (전혀 수정 불필요)
- ✅ C++ 전처리 서버 (전혀 수정 불필요)
- ✅ AI 서버 (전혀 수정 불필요)

---

## 2. 반대 예시: "모놀리스 구조"라면?

만약 모든 기능이 하나의 코드베이스에 있다면:

```python
# 하나의 거대한 Flask 앱

@app.route('/analyze', methods=['POST'])
def analyze():
    # 1. 파일 업로드 처리 (API Gateway 역할)
    file = request.files['image']

    # 2. OpenCV로 전처리 (C++ 서버 역할)
    image = cv2.imread(file)
    processed = preprocess_pipeline(image)

    # 3. AI 모델 추론 (AI 서버 역할)
    result = model.predict(processed)

    return jsonify(result)
```

**문제점:**

| 시나리오 | 마이크로서비스 | 모놀리스 |
|---------|--------------|---------|
| **C++ 전처리가 빠르다고 교체하려면?** | Python 전처리 서버만 삭제하고 C++ 서버 추가 | 전체 앱을 Python→C++ 포팅 (불가능에 가까움) |
| **프론트엔드를 Angular로 바꾸려면?** | 새 Angular 앱 만들고 같은 API 호출 | 템플릿 엔진까지 뜯어고쳐야 함 |
| **AI 모델만 업데이트하려면?** | AI 서버만 재시작 | 전체 서버 재시작 (파일 업로드도 중단됨) |
| **트래픽 폭주 시 전처리만 스케일 아웃하려면?** | C++ 서버만 3대로 증설 | 전체 앱을 3배로 복제 (비효율) |

---

## 3. 인터페이스(계약)만 지키면 내부는 자유롭다

### 실제 코드로 보는 "계약"

**API Gateway → C++ 서버 계약 (analysisService.ts):**

```typescript
// api-gateway/src/services/analysisService.ts (57-59번 줄)
const preprocessRes = await axios.post(`${PREPROCESS_SERVER_URL}/preprocess`, {
  imagePath: file.path
});

// 응답 처리 (61-63번 줄)
if (preprocessRes.data?.processedPath) {
  processedImagePath = preprocessRes.data.processedPath;
  logger.info('Preprocessing completed:', { processedPath: processedImagePath });
}
```

API Gateway는:
- C++로 구현되었는지, Python으로 구현되었는지 **모릅니다**
- 단지 "8081 포트의 `/preprocess`에 `imagePath`를 보내면 `processedPath`가 온다"만 알고 있습니다
- 65-69번 줄을 보면, 전처리가 실패해도 원본 이미지로 계속 진행합니다 (느슨한 결합)

---

**C++ 서버의 실제 구현 (server.h):**

```cpp
// preprocess-server/src/core/server.h (147-175번 줄)
CROW_ROUTE(app, "/preprocess").methods(crow::HTTPMethod::POST)([](const crow::request& req){
    // 1. 요청 검증
    auto validation = ValidatePreprocessRequest(req);  // imagePath 있는지 확인

    // 2. 이미지 처리
    auto processedOpt = ProcessImageFile(imagePath);   // OpenCV 파이프라인

    // 3. 결과 저장
    std::string outputPath = GenerateOutputPath(imagePath);
    SaveProcessedImage(*processedOpt, outputPath);

    // 4. 응답 반환
    return CreatePreprocessResponse(outputPath, duration);  // { "processedPath": "..." }
});
```

---

## 4. 구체적인 교체 시나리오

### 시나리오: C++가 너무 복잡해서 Python Flask로 바꾸기

**1단계: Python 전처리 서버 작성**

```python
# preprocess_server.py
from flask import Flask, request, jsonify
import cv2
import os

app = Flask(__name__)

@app.route('/preprocess', methods=['POST'])
def preprocess():
    data = request.get_json()
    image_path = data.get('imagePath')

    # 이미지 로드
    img = cv2.imread(image_path)

    # 전처리 파이프라인 (C++와 동일한 로직)
    resized = cv2.resize(img, (512, 512))
    blurred = cv2.GaussianBlur(resized, (5, 5), 0)
    gray = cv2.cvtColor(blurred, cv2.COLOR_BGR2GRAY)
    edges = cv2.Canny(gray, 50, 150)

    # 결과 저장
    output_path = image_path.replace('/uploads/', '/processed/').replace('.jpg', '_clean.jpg')
    cv2.imwrite(output_path, edges)

    return jsonify({"processedPath": output_path})

if __name__ == '__main__':
    app.run(port=8081)
```

**2단계: C++ 서버 중단 및 Python 서버 시작**

```bash
# C++ 서버 중단
kill $(lsof -t -i:8081)

# Python 서버 시작
python preprocess_server.py
```

**3단계: 끝!**

- ✅ API Gateway 코드 수정 **0줄**
- ✅ 프론트엔드 코드 수정 **0줄**
- ✅ AI 서버 영향 **0%**

API Gateway는 여전히 같은 엔드포인트에 같은 요청을 보내고, 같은 응답을 받으므로, **C++인지 Python인지 알 필요가 없습니다**.

---

## 5. 마이크로서비스의 핵심 원칙

### "계약(Contract)"만 지키면 된다

```
[API Gateway가 보는 세계]

전처리 서버 = 블랙박스 🎁

입력: { "imagePath": "/uploads/image.jpg" }
출력: { "processedPath": "/processed/image_clean.jpg" }

내부 구현:
- C++ + OpenCV? ✅ OK
- Python + Pillow? ✅ OK
- Rust + image-rs? ✅ OK
- GPU로 처리? ✅ OK
- 원본 그대로 반환? ✅ OK (테스트용)

단, 응답 형식만 지켜라!
```

---

## 6. 실전 예시: 서버별 독립적인 스케일링

### 시나리오: 트래픽이 10배 증가했다!

**모놀리스 구조라면:**
```
서버 1대 (프론트+백엔드+전처리+AI 모두 포함)
    ↓
서버 10대 복제 (불필요한 전처리, AI도 10배 복제 → 비용 폭증)
```

**마이크로서비스 구조라면:**

```
병목 지점 분석:
- API Gateway: CPU 10% (여유 있음)
- C++ 전처리: CPU 90% (병목!)  ← 여기만 증설
- AI 서버: GPU 30% (여유 있음)

해결책:
C++ 전처리 서버만 1대 → 5대로 증설
API Gateway에서 로드밸런싱
```

**docker-compose.yml 예시:**

```yaml
services:
  api-gateway:
    image: mind-palette/api-gateway
    ports:
      - "3000:3000"

  preprocess-1:  # 전처리 서버 1호
    image: mind-palette/preprocess
    ports:
      - "8081:8081"

  preprocess-2:  # 전처리 서버 2호
    image: mind-palette/preprocess
    ports:
      - "8082:8081"

  preprocess-3:  # 전처리 서버 3호
    image: mind-palette/preprocess
    ports:
      - "8083:8081"

  ai-server:
    image: mind-palette/ai
    ports:
      - "5000:5000"
```

API Gateway는 Round-robin으로 8081, 8082, 8083에 분산 요청합니다.

---

## 7. 정리: "영향이 없다"의 3가지 차원

| 차원 | 의미 | 예시 |
|------|------|------|
| **기술 교체** | 한 서버의 기술 스택을 바꿔도 다른 서버는 몰라도 됨 | C++ → Python, React → Vue |
| **스케일링** | 병목 구간만 선택적으로 증설 가능 | 전처리 서버만 5대로 증설 |
| **장애 격리** | 한 서버가 죽어도 다른 서버는 동작 | AI 서버 다운 시, 프론트엔드와 API Gateway는 정상 |

---

## 8. Mind Palette의 실제 계약(Contract) 정의

### Contract 1: Frontend ↔ API Gateway

**Endpoint:** `POST /analyze`

**요청:**
```
Content-Type: multipart/form-data

필드:
- image: File (이미지 파일, 최대 5MB)
```

**응답:**
```json
{
  "score": 85,
  "percentile": 92,
  "interpretation": "창의성이 뛰어난 그림입니다...",
  "date": "2026-02-16",
  "details": {
    "creativity": 90,
    "expression": 85,
    "observational": 88
  }
}
```

---

### Contract 2: API Gateway ↔ C++ Preprocessing Server

**Endpoint:** `POST /preprocess`

**요청:**
```json
{
  "imagePath": "/shared/uploads/1708089600_abc123.jpg"
}
```

**응답:**
```json
{
  "processedPath": "/shared/processed/1708089600_abc123_clean.jpg"
}
```

---

### Contract 3: API Gateway ↔ Python AI Server (예정)

**Endpoint:** `POST /predict`

**요청:**
```json
{
  "imagePath": "/shared/processed/1708089600_abc123_clean.jpg"
}
```

**응답:**
```json
{
  "score": 85,
  "percentile": 92,
  "interpretation": "AI 분석 결과...",
  "details": {
    "creativity": 90,
    "expression": 85,
    "observational": 88
  }
}
```

---

## 9. 실전 시나리오: 장애 대응

### 시나리오: C++ 전처리 서버가 다운되었다!

**모놀리스 구조:**
```
전체 서버 다운 → 사용자는 아무것도 못함 → 404 에러
```

**마이크로서비스 구조 (현재 Mind Palette):**

```typescript
// api-gateway/src/services/analysisService.ts (65-69번 줄)
catch (error: unknown) {
  logger.warn('Preprocessing failed, using original image:', { error });
  // 전처리 실패 시에도 일단 원본으로 계속 진행
}
```

**결과:**
1. C++ 서버 다운 감지
2. 경고 로그 남김
3. 원본 이미지로 AI 서버에 요청
4. 사용자는 여전히 결과를 받음 (품질은 약간 낮을 수 있음)

**이것이 "장애 격리(Fault Isolation)"입니다.**

---

## 10. 비유로 정리

### 레스토랑 비유

**모놀리스 레스토랑:**
```
주방장 1명이 모든 것을 담당:
- 주문 받기
- 재료 손질
- 요리
- 서빙
- 계산

문제:
- 주방장이 아프면 레스토랑 문 닫음
- 손님이 많아지면 주방장을 여러 명 고용해야 함 (비효율)
- 요리법을 바꾸려면 주방장을 재교육해야 함
```

**마이크로서비스 레스토랑 (Mind Palette):**
```
역할 분담:
- 웨이터 (API Gateway): 주문 받고 음식 전달
- 재료 손질팀 (C++ 전처리): 야채 썰고 고기 준비
- 요리팀 (AI 서버): 실제 요리
- 계산대 (Frontend): 손님 응대

장점:
- 재료 손질팀이 느리면 그 팀만 증원
- 요리법을 바꿔도 웨이터는 그대로
- 요리팀이 쉬는 날엔 간단한 메뉴만 제공 (원본 이미지 사용)
```

---

이 문서는 Mind Palette 프로젝트의 마이크로서비스 아키텍처가 왜 유연하고 확장 가능한지를 구체적인 코드와 시나리오로 설명합니다.
