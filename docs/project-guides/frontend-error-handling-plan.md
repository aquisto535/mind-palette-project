# 프론트엔드 에러 화면 구현 계획 (ADR-033 연동)

> **작성일**: 2026-04-05
> **관련 ADR**: ADR-033 Two-Tier Fail-Fast (HSV + Confidence Score)

## Context

ADR-033 두 계층 필터링(C++ HSV + Python Confidence Score)이 완성되어 서버는 이제 400/422/503 등을 정확히 구분해 반환합니다. 그러나 **프론트엔드가 모든 에러를 `alert()` 하나로 처리**하여 ADR-033 422 에러가 사용자에게 전달되지 않습니다. `client.ts`에 ADR-033 인터셉터 골격은 이미 존재하나 비어 있는 상태이며, 에러 전용 컴포넌트도 없습니다.

이번 구현으로 **HTTP 상태코드별 에러 화면**을 추가하여 422(비연필 그림)·400(파일 오류)·503(서버 문제) 각각에 맞는 메시지와 액션을 제공합니다.

---

## 구현 범위

### 새로 만들 파일

| 파일 | 역할 |
|------|------|
| `frontend/src/types/errors.ts` | 에러 타입 정의 (`AnalysisError`) |
| `frontend/src/components/Error.tsx` | 에러 전용 화면 컴포넌트 |

### 수정할 파일

| 파일 | 변경 내용 |
|------|-----------|
| `frontend/src/api/client.ts` | 응답 인터셉터에서 커스텀 `AnalysisError` 생성 (status + message) |
| `frontend/src/App.tsx` | Step에 `'error'` 추가, `handleUpload` catch 블록에서 상태코드별 분기, Error 컴포넌트 렌더링 |

---

## 현재 파일 상태 (확인됨)

| 파일 | 현재 상태 |
|------|-----------|
| `frontend/src/api/client.ts` | 422 인터셉터 골격만 존재, `Promise.reject(error)` 그대로 반환 |
| `frontend/src/App.tsx` | `alert()` 사용, `Step`에 `'error'` 없음 |
| `frontend/src/types/index.ts` | `ChildInfo`, `AnalysisResult`만 정의 (AnalysisError 없음) |
| `frontend/src/components/` | `Error.tsx` 없음 (`Guide.tsx` 스타일 참고 가능) |

---

## 상세 구현 계획

### Step 1: 에러 타입 정의 (`frontend/src/types/errors.ts` 신규)

```typescript
export interface AnalysisError {
  status: number;           // HTTP 상태코드 (400, 422, 429, 503, 0=네트워크)
  message: string;          // 사용자에게 보여줄 한국어 메시지
  actionType: 'retry-upload' | 'retry-guide' | 'retry-later';
}
```

에러 메시지 매핑 (client.ts 인터셉터 내부에서 생성):

| status | message | actionType |
|--------|---------|------------|
| 400 | "파일이 올바른 이미지 형식이 아닙니다. 다시 선택해주세요." | `retry-upload` |
| 422 | "연필로 그린 전신 인물화를 올려주세요." | `retry-guide` |
| 429 | "요청이 너무 많습니다. 잠시 후 다시 시도해주세요." | `retry-later` |
| 503 | "서버가 바쁩니다. 잠시 후 다시 시도해주세요." | `retry-later` |
| 기타 | "분석 중 오류가 발생했습니다. 잠시 후 다시 시도해주세요." | `retry-later` |

### Step 2: client.ts 인터셉터 완성

`frontend/src/api/client.ts`의 응답 인터셉터에서 `AnalysisError`를 throw:

```typescript
import { AnalysisError } from '../types/errors';

const ERROR_MAP: Record<number, Omit<AnalysisError, 'status'>> = {
  400: { message: '파일이 올바른 이미지 형식이 아닙니다. 다시 선택해주세요.', actionType: 'retry-upload' },
  422: { message: '연필로 그린 전신 인물화를 올려주세요.', actionType: 'retry-guide' },
  429: { message: '요청이 너무 많습니다. 잠시 후 다시 시도해주세요.', actionType: 'retry-later' },
  503: { message: '서버가 바쁩니다. 잠시 후 다시 시도해주세요.', actionType: 'retry-later' },
};

// 응답 인터셉터
client.interceptors.response.use(
  (response) => response,
  (error) => {
    const status: number = error.response?.status ?? 0;
    if (status === 401) localStorage.removeItem('token');
    const mapped = ERROR_MAP[status] ?? {
      message: '분석 중 오류가 발생했습니다. 잠시 후 다시 시도해주세요.',
      actionType: 'retry-later',
    };
    return Promise.reject({ status, ...mapped } as AnalysisError);
  }
);
```

### Step 3: Error.tsx 컴포넌트 신규 작성

`frontend/src/components/Error.tsx`:

- props: `error: AnalysisError`, `onRetry: () => void`, `onGuide: () => void`
- `actionType === 'retry-guide'`일 때만 "올바른 그림 안내 보기" 버튼 표시
- 나머지는 "다시 업로드" 버튼만 표시
- `Guide.tsx` 스타일(framer-motion, Tailwind) 참고

```tsx
interface ErrorProps {
  error: AnalysisError;
  onRetry: () => void;   // upload step으로 복귀
  onGuide: () => void;   // guide step으로 이동
}
```

### Step 4: App.tsx 수정

1. `Step` 타입에 `'error'` 추가:
   ```typescript
   type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result' | 'error';
   ```

2. `error` 상태 추가:
   ```typescript
   const [analysisError, setAnalysisError] = useState<AnalysisError | null>(null);
   ```

3. `handleUpload` catch 블록 교체 (alert 제거):
   ```typescript
   catch (error) {
     setAnalysisError(error as AnalysisError);
     setStep('error');
     setFile(null);
   }
   ```

4. Error 컴포넌트 렌더링 추가:
   ```tsx
   {step === 'error' && analysisError ? (
     <Error
       error={analysisError}
       onRetry={() => { setAnalysisError(null); setStep('upload'); }}
       onGuide={() => { setAnalysisError(null); setStep('guide'); }}
     />
   ) : null}
   ```

5. `handleReset`에서 `analysisError` 초기화 추가.

---

## 실행 흐름 (변경 후)

```
upload → loading → (에러 발생)
                       ↓
                   error step (Error.tsx)
                       ├─ 422: [안내 보기] → guide step
                       │        [다시 업로드] → upload step
                       └─ 기타: [다시 업로드] → upload step
```

---

## 구현 순서

1. `frontend/src/types/errors.ts` 신규 생성 (의존성 없음)
2. `frontend/src/api/client.ts` 수정 (`errors.ts` import)
3. `frontend/src/components/Error.tsx` 신규 생성 (`Guide.tsx` 스타일 참고)
4. `frontend/src/App.tsx` 수정 (모든 변경 통합)

---

## 검증 방법

1. `VITE_USE_MOCK=false` 환경에서 비연필 이미지 업로드 → `error` step 표시, 422 메시지 확인
2. "올바른 그림 안내 보기" 클릭 → `guide` step으로 전환
3. "다시 업로드" 클릭 → `upload` step으로 전환
4. 서버 종료 후 업로드 → 503 메시지 표시, "다시 업로드" 버튼만 표시
5. `VITE_USE_MOCK=true` 모드에서는 에러 화면 표시 안 됨 (Mock 항상 성공) — 정상
