# 검증 파이프라인 최적화 (Latency Strategy) 구현 계획

## Context

현재 `POST /analyze` 엔드포인트의 검증 로직은 **완전히 순차(Sequential)** 실행됩니다.
plan.md(351-353행)에 정의된 두 가지 최적화 항목을 구현합니다:

1. **Parallel Validation**: L2(Magic Byte)와 L5(Resource Limit)를 병렬 체크
2. **Deferred Sanitization**: L6(재인코딩)을 전처리 서버로 이관

### 현재 검증 흐름 (순차)
```
analyzeLimiter(L5) → handleMulterUpload(L1/L3) → validateImageContent(L2) → processAnalysis
```

### 병목 지점
1. `validateImageContent()`에서 `fs.readFile(filePath)` — 파일 전체(최대 5MB)를 읽지만, Magic Byte 검증에는 12바이트면 충분
2. L5에 해상도(Pixel Flood) 제한 없음 — 파일 크기 5MB만 제한
3. L6(재인코딩) 미구현 — Polyglot 파일 공격 방어 부재

---

## 구현 계획

### Step 1: L2 최적화 — 헤더만 읽기 (TDD)

**파일**: `api-gateway/src/routes/analyze.ts` — `validateImageContent()`

**변경**: 파일 전체 읽기 → **처음 12바이트만 읽기** (WebP 검증에 필요한 최대 바이트)

```typescript
// Before: 파일 전체 읽기
const fileBuffer = await fs.readFile(filePath);

// After: 헤더 12바이트만 읽기
const fd = await fs.open(filePath, 'r');
const headerBuffer = Buffer.alloc(12);
await fd.read(headerBuffer, 0, 12, 0);
await fd.close();
```

**테스트** (`security.test.ts` 추가):
- [Red] 대용량(5MB) 파일도 매직 바이트 검증 즉시 완료 확인
- 기존 매직 바이트 테스트 11개 전부 회귀 통과

---

### Step 2: L5 강화 — 해상도 제한 추가 (TDD)

**파일**: `api-gateway/src/routes/analyze.ts`, `api-gateway/src/utils/fileStorage.ts`

**변경**: L2 통과 후 `image-size` 라이브러리로 이미지 메타데이터 파싱 → 최대 **4096×4096 px** 제한

```typescript
import sizeOf from 'image-size';

const dimensions = sizeOf(filePath);
if (dimensions.width > 4096 || dimensions.height > 4096) {
  await fs.unlink(filePath);
  return res.status(400).json({ error: 'Image dimensions exceed limit (max 4096x4096)' });
}
```

**테스트** (`security.test.ts` 추가):
- [Red] 8192×1 이미지 업로드 시 400 반환
- [Green] 512×512 이미지는 정상 통과

**의존성**: `npm install image-size` + `@types/image-size`

---

### Step 3: L6 Deferred Sanitization — 전처리 서버 이관 (TDD)

**설계 결정**: C++ 전처리 서버가 `cv::imread() → 필터 → cv::imwrite()`로 **이미 완전한 재인코딩을 수행**하므로, 별도 `/sanitize` 엔드포인트 없이 기존 전처리 파이프라인이 L6 역할을 겸함.

**파일**: `api-gateway/src/services/analysisService.ts`

**변경**: L6 재인코딩 상태를 **명시적으로 로깅** + 전처리 실패 시 경고

```typescript
if (preprocessRes.data?.processedPath) {
  logger.info('Preprocessing completed (L6 sanitized):', ...);
} else {
  logger.warn('L6 Sanitization skipped: no processedPath', ...);
}
```

**파일**: `api-gateway/src/routes/analyze.ts`

**변경**: 전처리 실패(fallback 원본 사용) 시 L6 미적용 응답 헤더 추가

```typescript
res.set('X-Sanitization-Status', sanitized ? 'applied' : 'skipped');
```

**테스트** (`analysisService.test.ts` 추가):
- [Red] 전처리 성공 시 로그에 'L6 sanitized' 포함
- [Red] 전처리 실패 시 'L6 Sanitization skipped' 경고 로그

---

## 수정 파일 목록

| 파일 | 변경 내용 |
|------|----------|
| `api-gateway/src/routes/analyze.ts` | L2 헤더만 읽기, L5 해상도 검증, L6 응답 헤더 |
| `api-gateway/src/utils/fileStorage.ts` | `checkImageDimensions()` 유틸 추가 |
| `api-gateway/src/services/analysisService.ts` | L6 재인코딩 로깅 명시화 |
| `api-gateway/tests/security.test.ts` | L2 최적화 + L5 해상도 테스트 |
| `api-gateway/tests/analysisService.test.ts` | L6 로깅 검증 테스트 |
| `api-gateway/package.json` | `image-size` 의존성 추가 |

---

## 검증 방법

```bash
cd api-gateway && npm test
```

1. 기존 보안 테스트 전부 통과 (회귀 방지)
2. 새 L5 해상도 + L6 로깅 테스트 통과
3. plan.md 351-353행 체크 완료 처리
