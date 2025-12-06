# Mind Palette 프론트엔드 개발 가이드 (React + TypeScript)

본 가이드는 `JavaScript & TypeScript` 폴더의 학습 내용을 바탕으로, **Mind Palette (아동 인물화 지능측정 AI 시스템)** 프로젝트의 프론트엔드 개발 시 유념해야 할 핵심 사항을 정리한 문서입니다.

---

## 1. 프로젝트 기술 스택 및 전략

| 기술                | 선택 이유 및 활용 전략                                             |
| :---------------- | :-------------------------------------------------------- |
| **React**         | 컴포넌트 기반 UI 개발 (재사용성 극대화)                                  |
| **TypeScript**    | **(강력 추천)** 서버(Node/C++/Python) 간 통신 데이터(JSON)의 타입 안정성 보장 |
| **Redux Toolkit** | 전역 상태 관리 (사용자 로그인 정보, 분석 진행 상태, 결과 리포트 데이터)               |
| **Axios**         | HTTP 통신 라이브러리 (이미지 업로드, 결과 조회, 인터셉터 활용)                   |
| **Material-UI**   | 빠른 UI 프로토타이핑 및 반응형 디자인 적용                                 |

---

## 2. 핵심 개념 적용 가이드

### 2.1. 컴포넌트 (Components) & Props
> **참고**: `리액트/컴포넌트.md`, `리액트/Props.md`

- **아토믹 디자인(Atomic Design) 지향**: 
  - 작게 쪼개서 재사용성을 높입니다.
  - 예: `Button`, `ProgressBar` (Atom) → `UploadForm` (Molecule) → `AnalysisPage` (Organism)
- **Props 활용**: 
  - 데이터 흐름은 **단방향(부모 → 자식)**으로 유지합니다.
  - `Props Drilling`이 3단계 이상 깊어지면 **Redux Toolkit** 도입을 고려합니다.

### 2.2. 상태 관리 (State Management)
> **참고**: `리액트/리덕스 툴킷을 사용한 상태 관리.md`

**언제 `useState`를 쓰고, 언제 `Redux`를 쓰는가?**

| 구분 | 사용처 (Mind Palette 예시) | 관리 도구 |
| :--- | :--- | :--- |
| **Local State** | 폼 입력값(`email`, `password`), 모달 열림/닫힘 여부, 탭 전환 상태 | `useState` |
| **Global State** | **사용자 인증 정보**(로그인 유무), **이미지 분석 진행률**, **최종 분석 결과 데이터** | `Redux Toolkit` |

**Redux Slice 구조 제안**:
- `authSlice.ts`: 로그인 상태, 유저 정보 (`user`, `token`, `isAuthenticated`)
- `analysisSlice.ts`: 현재 분석 중인 이미지 상태 (`uploadProgress`, `processingStage`, `resultData`)
  - *processingStage*: 'idle' -> 'uploading' -> 'preprocessing(C++)' -> 'inferencing(AI)' -> 'complete'

### 2.3. 데이터 통신 (Ajax & Axios)
> **참고**: `리액트/Ajax를 활용한 데이터 송수신.md`

**이미지 업로드 (Multipart/form-data)**:
일반 JSON 전송과 달리, 이미지는 `FormData` 객체를 사용해야 합니다.

```typescript
// src/api/uploadApi.ts
import axios from 'axios';

export const uploadImage = async (file: File) => {
  const formData = new FormData();
  formData.append('image', file); // Node.js Multer 설정과 이름('image')이 일치해야 함

  const response = await axios.post('/api/analyze', formData, {
    headers: {
      'Content-Type': 'multipart/form-data', // 명시적 선언
    },
    onUploadProgress: (progressEvent) => {
      // 업로드 진행률 바(Progress Bar) 구현에 활용
      const percentCompleted = Math.round((progressEvent.loaded * 100) / progressEvent.total);
      console.log(percentCompleted); 
    }
  });
  return response.data;
};
```

**인터셉터(Interceptor) 활용**:
JWT 토큰 인증 방식 사용 시, 모든 요청 헤더에 자동으로 토큰을 실어 보냅니다.

```typescript
// src/api/client.ts
axiosInstance.interceptors.request.use((config) => {
  const token = store.getState().auth.token; // Redux에서 토큰 가져오기
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});
```

### 2.4. React Hooks 활용
> **참고**: `리액트/리액트 훅.md`

- **`useEffect`**:
  - **분석 결과 폴링(Polling)**: AI 분석이 2~3초 소요되므로, 분석 요청 후 `setInterval`로 완료 여부를 주기적으로 확인하다가 완료 시 결과창으로 이동하는 로직에 사용.
- **`useMemo`**:
  - **결과 차트 연산**: 복잡한 지능 점수 계산이나 차트 데이터 가공 시 불필요한 재연산을 방지하여 렌더링 성능 최적화.

---

## 3. TypeScript 활용 전략 (안정성 강화)
> **참고**: `Typescript/` 폴더

JavaScript로 개발 시 서버 응답(`res.data`)의 구조를 몰라 `undefined` 에러가 자주 발생합니다. Interface를 정의하여 이를 방지하세요.

```typescript
// src/types/analysis.ts

// 서버에서 오는 분석 결과 JSON 형태 정의
export interface AnalysisResult {
  score: number;
  category: 'high' | 'medium' | 'low';
  features: {
    head_size: number;
    line_pressure: number;
    colors: string[];
  };
  feedback_text: string;
}

// 컴포넌트에서 사용
const ResultCard = ({ result }: { result: AnalysisResult }) => {
  return <div>점수: {result.score}</div>; // 자동완성 지원 & 오타 방지
};
```

---
## 4. 시니어의 개발 조언 (Mind Palette 특화)

1.  **비동기 처리 UX**: 이미지가 업로드되고 분석되는 3초 동안 사용자가 "멈췄나?"라고 느끼지 않게 **스켈레톤 UI(Skeleton UI)**나 **단계별 진행 상태(Stepper)**를 꼭 보여주세요. (Redux 상태로 관리)
2.  **에러 핸들링**: "서버 에러"라고 퉁치지 말고, "이미지 형식이 잘못되었습니다" 혹은 "분석 서버 응답이 지연됩니다" 처럼 구체적으로 사용자에게 알려주세요. (`try-catch` 구문 활용)
3.  **반응형 차트**: 모바일에서도 결과 차트가 잘 보이도록 `recharts` 등의 라이브러리 사용 시 `ResponsiveContainer`를 꼭 사용하세요.

