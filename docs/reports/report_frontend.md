# Frontend 모듈 기술 보고서

## AI 아동 인물화 인지 분석 웹 애플리케이션 — 프론트엔드 설계 및 구현 상세 보고서

---

**프로젝트명:** Mind Palette  
**모듈명:** Frontend  
**기술 스택:** React 18 · TypeScript · Vite · TailwindCSS · Framer Motion  
**작성일:** 2026-02-14  
**문서 유형:** 기술 보고서 (논문 형태)

---

## 목차

1. [서론](#1-서론)
2. [시스템 개요 및 요구사항](#2-시스템-개요-및-요구사항)
3. [기술 스택 선정 근거](#3-기술-스택-선정-근거)
4. [프로젝트 구조 및 아키텍처](#4-프로젝트-구조-및-아키텍처)
5. [빌드 시스템 및 개발 환경 구성](#5-빌드-시스템-및-개발-환경-구성)
6. [타입 시스템 설계](#6-타입-시스템-설계)
7. [애플리케이션 상태 관리 및 플로우 제어](#7-애플리케이션-상태-관리-및-플로우-제어)
8. [컴포넌트 상세 분석](#8-컴포넌트-상세-분석)
9. [API 통신 계층](#9-api-통신-계층)
10. [시각화 엔진](#10-시각화-엔진)
11. [UI/UX 디자인 시스템](#11-uiux-디자인-시스템)
12. [테스트 전략](#12-테스트-전략)
13. [성능 최적화](#13-성능-최적화)
14. [확장성 및 향후 개선 방향](#14-확장성-및-향후-개선-방향)
15. [결론](#15-결론)

---

## 1. 서론

### 1.1 배경 및 목적

Mind Palette 프로젝트의 Frontend 모듈은 아동이 그린 인물화(사람 그림)를 AI가 분석하여 인지 발달 수준을 평가하는 웹 기반 사용자 인터페이스이다. 본 모듈의 주요 목적은 심리학적 분석이라는 전문적이고 복잡한 결과를 비전문가인 부모(주 사용자층: 30~40대 어머니)가 직관적으로 이해할 수 있도록 시각화하여 전달하는 것이다.

### 1.2 핵심 사용자 시나리오

사용자 플로우는 **6단계 위저드(Wizard) 패턴**으로 설계되었다:

1. **Hero (소개):** 서비스 소개 및 분석 시작 유도
2. **InfoForm (정보 입력):** 아이의 이름, 성별, 생년월일 입력
3. **Guide (가이드):** 정확한 분석을 위한 그림 그리기 안내
4. **Upload (업로드):** 이미지 파일 업로드 (드래그&드롭 / 클릭)
5. **Loading (분석 중):** AI 분석 진행 상태 표시
6. **Result (결과):** 분석 결과 시각화 및 PDF 다운로드

### 1.3 설계 철학

- **단순함(Simplicity):** 기술에 익숙하지 않은 부모도 별도의 학습 없이 사용 가능한 인터페이스
- **신뢰감(Trust):** 전문적인 보고서 형식의 결과 화면으로 분석의 신뢰성 전달
- **감성적 접근(Emotional):** 차가운 데이터 대신 "인지 발달 나무"와 같은 비유적 시각화로 부모의 감성에 호소
- **접근성(Accessibility):** 한국어 기반, 모바일/데스크톱 반응형 레이아웃

---

## 2. 시스템 개요 및 요구사항

### 2.1 기능적 요구사항

| 요구사항 ID | 설명 | 우선순위 |
|---|---|---|
| FR-01 | 아동 정보 입력 (이름, 성별, 생년월일) | 필수 |
| FR-02 | 이미지 파일 업로드 (JPG, PNG 지원) | 필수 |
| FR-03 | 드래그 앤 드롭 업로드 지원 | 필수 |
| FR-04 | 업로드 이미지 미리보기 | 필수 |
| FR-05 | AI 분석 결과 시각화 (나무, 정규분포) | 필수 |
| FR-06 | 분석 결과 PDF 다운로드 | 필수 |
| FR-07 | 결과 공유 기능 (클립보드 URL 복사) | 선택 |
| FR-08 | Mock 데이터 기반 개발 모드 지원 | 필수 |

### 2.2 비기능적 요구사항

| 요구사항 ID | 설명 | 기준 |
|---|---|---|
| NFR-01 | 초기 로딩 시간 | LCP < 2.5초 |
| NFR-02 | 반응형 디자인 | 320px ~ 1920px |
| NFR-03 | 브라우저 호환성 | Chrome, Safari, Firefox 최신 2버전 |
| NFR-04 | 접근성 | 시맨틱 HTML, ARIA 라벨 |
| NFR-05 | 한국어 기본 UI | 모든 텍스트 한국어 |

---

## 3. 기술 스택 선정 근거

### 3.1 React 18 + TypeScript

**선정 이유:**

- **컴포넌트 기반 아키텍처:** 6단계 위저드의 각 단계를 독립적인 컴포넌트로 분리하여 관리 가능
- **TypeScript 정적 타이핑:** `ChildInfo`, `AnalysisResult` 등 도메인 타입을 명시적으로 정의하여 API 응답과 컴포넌트 간 데이터 흐름의 안전성 보장
- **React StrictMode:** 개발 환경에서 잠재적 문제를 사전에 감지
- **함수형 컴포넌트 + Hooks:** `useState`, `useEffect`, `useCallback`, `useRef`를 활용한 선언적 상태 관리

**핵심 구현 패턴:**

```typescript
// React.FC 제네릭 타입을 통한 Props 타입 안전성
export const Hero: React.FC<HeroProps> = ({ onStart }) => {
  // ...
};
```

### 3.2 Vite (빌드 도구)

**선정 이유:**

- **ES Modules 기반 HMR(Hot Module Replacement):** 개발 시 파일 변경 시 즉각적인 반영 (CRA 대비 10~100배 빠른 HMR)
- **Rollup 기반 프로덕션 빌드:** 최적화된 번들 생성
- **네이티브 TypeScript 지원:** 별도 설정 없이 `.tsx` 파일 직접 처리
- **Vitest 테스트 통합:** 동일한 설정 파일에서 빌드와 테스트 구성을 관리

**설정 파일 분석 (`vite.config.ts`):**

```typescript
export default defineConfig({
  plugins: [react()],      // @vitejs/plugin-react: JSX 변환 및 Fast Refresh
  test: {
    globals: true,          // describe, it, expect 전역 사용
    environment: 'jsdom',   // DOM API 시뮬레이션
    setupFiles: './src/setupTests.ts',  // 테스트 전 모의 객체 설정
    css: true,              // CSS 임포트 처리
  },
});
```

### 3.3 TailwindCSS v3

**선정 이유:**

- **유틸리티 퍼스트(Utility-First):** 클래스명 조합으로 신속한 스타일링. 별도 CSS 파일 관리 불필요
- **반응형 프리픽스:** `md:text-6xl`처럼 간결한 반응형 디자인
- **디자인 토큰 커스터마이징:** `tailwind.config.js`에서 프로젝트 고유 색상 정의

**커스텀 컬러 시스템:**

```javascript
colors: {
  primary: '#4F46E5',    // Indigo 600 — 신뢰감, 전문성
  secondary: '#EC4899',  // Pink 500 — 아동, 부드러움
  background: '#F9FAFB', // Gray 50 — 깔끔한 배경
}
```

### 3.4 Framer Motion

**선정 이유:**

- **선언적 애니메이션 API:** `initial`, `animate`, `whileInView` 프로퍼티로 복잡한 애니메이션을 간결하게 표현
- **SVG 애니메이션 지원:** 결과 페이지의 나무 성장 애니메이션, 정규분포 곡선 인디케이터 등 데이터 시각화에 활용
- **성능 최적화:** GPU 가속 CSS 속성(`transform`, `opacity`) 우선 사용

### 3.5 추가 의존성

| 라이브러리 | 용도 | 선정 이유 |
|---|---|---|
| `axios` | HTTP 통신 | 인터셉터, 요청 취소, 자동 JSON 변환 |
| `lucide-react` | 아이콘 | 트리 셰이킹 지원, 일관된 디자인 |
| `html2canvas` | DOM→Canvas 캡처 | PDF 생성 시 결과 화면 캡처 |
| `jsPDF` | PDF 생성 | 클라이언트 사이드 PDF 다운로드 |
| `clsx` | 조건부 클래스 결합 | TailwindCSS와 조합 사용 |
| `tailwind-merge` | 클래스 중복 해소 | 동적 스타일 충돌 방지 |

---

## 4. 프로젝트 구조 및 아키텍처

### 4.1 디렉토리 구조

```
frontend/
├── index.html                # SPA 진입점 (Vite가 처리)
├── package.json              # 프로젝트 매니페스트 및 의존성 정의
├── vite.config.ts            # Vite 빌드 + Vitest 테스트 통합 설정
├── tsconfig.json             # TypeScript 컴파일러 옵션 (ES2020, strict)
├── tailwind.config.js        # TailwindCSS 테마 커스터마이징
├── postcss.config.js         # PostCSS 플러그인 설정
├── Screen design document.md # 화면 설계서 초안
├── README.md                 # 프로젝트 실행 가이드
├── src/
│   ├── main.tsx              # React 앱 마운트 진입점
│   ├── App.tsx               # 최상위 앱 컴포넌트 (상태 관리 + 라우팅)
│   ├── index.css             # 글로벌 CSS (TailwindCSS 디렉티브)
│   ├── setupTests.ts         # 테스트 환경 모의 객체 설정
│   ├── vite-env.d.ts         # Vite 환경 변수 타입 선언
│   ├── components/           # UI 컴포넌트 (위저드 각 단계)
│   │   ├── Hero.tsx          # Step 1: 소개 화면
│   │   ├── InfoForm.tsx      # Step 2: 아동 정보 입력
│   │   ├── Guide.tsx         # Step 3: 그림 그리기 가이드
│   │   ├── Upload.tsx        # Step 4: 이미지 업로드
│   │   ├── Loading.tsx       # Step 5: 분석 중 로딩
│   │   ├── Result.tsx        # Step 6: 분석 결과 표시
│   │   └── __tests__/        # 컴포넌트 단위 테스트
│   ├── api/                  # API 통신 계층
│   │   ├── client.ts         # Axios 인스턴스 (기본 설정, 인터셉터)
│   │   └── uploadApi.ts      # 이미지 업로드 + Mock 데이터 지원
│   ├── types/                # TypeScript 타입 정의
│   │   └── index.ts          # ChildInfo, AnalysisResult 인터페이스
│   ├── routes/               # 라우팅 (향후 확장용, 현재 미사용)
│   ├── services/             # 비즈니스 로직 서비스 (향후 확장용)
│   └── utils/                # 유틸리티 함수 (향후 확장용)
└── dist/                     # 빌드 산출물
```

### 4.2 아키텍처 패턴

본 Frontend는 **Lifting State Up + Wizard Pattern**을 채택하였다.

```
┌─────────────────────────────────────────────────┐
│                   App.tsx                        │
│  ┌─────────────────────────────────────────┐    │
│  │ State: step, childInfo, file, result    │    │
│  │ (Lifting State Up - 모든 상태를 App에서 │    │
│  │  관리하여 컴포넌트 간 공유)              │    │
│  └─────────────────────────────────────────┘    │
│         │         │         │         │         │
│    ┌────┴─┐  ┌────┴──┐  ┌──┴──┐  ┌───┴──┐     │
│    │ Hero │  │InfoForm│  │Guide│  │Upload│      │
│    └──────┘  └───────┘  └─────┘  └──────┘      │
│         │         │                              │
│    ┌────┴───┐ ┌───┴───┐                         │
│    │Loading │ │Result │                          │
│    └────────┘ └───────┘                          │
└─────────────────────────────────────────────────┘
         │
    ┌────┴────────────┐
    │   API Layer      │
    │   (uploadApi.ts) │
    │   Mock / Real    │
    └─────────────────┘
```

**설계 결정 이유:**

- 상태 관리 라이브러리(Redux, Zustand 등)를 사용하지 않은 이유: 6단계 위저드의 단순한 선형 플로우에서 App 컴포넌트의 `useState`만으로 충분히 관리 가능
- 각 단계가 독립적이며 동시에 여러 단계가 표시되지 않으므로 조건부 렌더링(`step === 'hero' && <Hero />`)으로 충분

---

## 5. 빌드 시스템 및 개발 환경 구성

### 5.1 TypeScript 컴파일러 설정

```json
{
  "compilerOptions": {
    "target": "ES2020",                    // 최신 브라우저 대상
    "lib": ["ES2020", "DOM", "DOM.Iterable"], // DOM API 및 이터러블 지원
    "module": "ESNext",                    // ES 모듈 사용
    "moduleResolution": "bundler",         // Vite 번들러 모드
    "jsx": "react-jsx",                    // React 17+ 새로운 JSX 변환
    "strict": true,                        // 엄격한 타입 체크 활성화
    "noUnusedLocals": true,                // 미사용 변수 오류 처리
    "noUnusedParameters": true,            // 미사용 파라미터 오류 처리
    "noFallthroughCasesInSwitch": true,    // switch case 빠짐 방지
    "isolatedModules": true,               // 단일 파일 변환 보장
    "noEmit": true                         // Vite가 번들링하므로 tsc는 타입 체크만 수행
  }
}
```

**주요 설계 결정:**

- `"jsx": "react-jsx"`: `import React from 'react'`를 매 파일마다 작성할 필요 없이 자동으로 JSX 변환 런타임을 주입 (React 17+의 새로운 JSX 변환)
- `"noEmit": true` + `"isolatedModules": true`: TypeScript는 순수하게 타입 검사만 수행하고, 실제 코드 변환과 번들링은 Vite(esbuild/Rollup)가 담당하는 역할 분리
- `"moduleResolution": "bundler"`: Vite와의 호환성을 위해 번들러 방식의 모듈 해석 사용

### 5.2 TailwindCSS 설정

```javascript
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",  // 모든 소스 파일에서 클래스명 추출
  ],
  theme: {
    extend: {
      colors: {
        primary: '#4F46E5',     // 인디고 600 — 메인 액션 색상
        secondary: '#EC4899',   // 핑크 500 — 보조 강조 색상
        background: '#F9FAFB',  // 그레이 50 — 기본 배경
      }
    },
  },
  plugins: [],
};
```

**색상 선정 근거:**

- **Primary (#4F46E5, Indigo):** 의료/교육 분야에서 신뢰감을 전달하는 파란색 계열. 너무 차갑지 않은 인디고로 따뜻함 가미
- **Secondary (#EC4899, Pink):** 아동 관련 서비스에서 부드러움과 친근함을 전달. 로딩 애니메이션의 보조 원에 사용
- **Background (#F9FAFB, Gray 50):** 순백보다 약간 따뜻한 톤으로 눈의 피로도 감소

### 5.3 글로벌 CSS

```css
@import "tailwindcss/base";
@import "tailwindcss/components";
@import "tailwindcss/utilities";

body {
  font-family: 'Pretendard', -apple-system, BlinkMacSystemFont, system-ui, Roboto, sans-serif;
}
```

**폰트 전략:**

- **Pretendard:** 한국어 가독성이 뛰어난 현대적 산세리프 글꼴. 한글의 자간과 행간이 웹 환경에 최적화됨
- **Fallback 체인:** Apple → Windows → Android 순으로 시스템 기본 글꼴로 대체

---

## 6. 타입 시스템 설계

### 6.1 도메인 타입 정의

프로젝트의 핵심 데이터 모델은 `src/types/index.ts`에 중앙 집중적으로 정의되어 있다.

```typescript
/**
 * ChildInfo — 분석 대상 아동의 기본 정보
 * InfoForm 컴포넌트에서 입력받아 App 상태로 보관
 */
export interface ChildInfo {
  name: string;              // 아동 이름 또는 애칭
  gender: 'male' | 'female'; // 성별 (유니온 리터럴 타입으로 제약)
  birthDate: string;         // 생년월일 (ISO date string: YYYY-MM-DD)
}

/**
 * AnalysisResult — AI 분석 결과 데이터 구조
 * API로부터 수신하거나 Mock으로 생성
 */
export interface AnalysisResult {
  score: number;           // 인지 발달 점수 (0~100)
  percentile: number;      // 또래 대비 백분위 (0~100)
  interpretation: string;  // 텍스트 기반 종합 해석
  date: string;            // 분석일 (로케일 형식 문자열)
}
```

### 6.2 타입 설계 원칙

- **Union Literal Type (`'male' | 'female'`):** 문자열 자유 입력 대신 허용값을 컴파일 타임에 제한하여 잘못된 성별 값이 전달되는 것을 원천 차단
- **중앙 집중 관리:** 모든 도메인 타입을 `types/index.ts`에 정의하여 컴포넌트, API 계층, 테스트 모두에서 동일한 타입 참조 보장
- **인터페이스 vs 타입 별칭:** 객체 구조 정의에는 확장 가능성을 고려하여 `interface` 사용

### 6.3 내부 상태 타입

```typescript
// App.tsx 내부에서 정의된 위저드 단계 타입
type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result';
```

6개의 문자열 리터럴 유니온으로 정의하여 잘못된 단계 전환을 타입 레벨에서 방지한다.

---

## 7. 애플리케이션 상태 관리 및 플로우 제어

### 7.1 App.tsx — 최상위 컨트롤러

`App.tsx`는 전체 애플리케이션의 상태와 플로우를 관리하는 컨트롤러 컴포넌트이다.

**상태 목록:**

| 상태 | 타입 | 초기값 | 역할 |
|---|---|---|---|
| `step` | `Step` | `'hero'` | 현재 위저드 단계 |
| `childInfo` | `ChildInfo \| null` | `null` | 아동 정보 |
| `file` | `File \| null` | `null` | 업로드된 이미지 파일 |
| `result` | `AnalysisResult \| null` | `null` | AI 분석 결과 |

### 7.2 상태 전이 다이어그램

```
hero ──onStart──> form ──onSubmit──> guide ──onNext──> upload
                                                         │
                                                    handleUpload
                                                         │
                                                         ▼
                                                      loading
                                                      │     │
                                                  success  error
                                                      │     │
                                                      ▼     ▼
                                                   result  upload (롤백)
                                                      │
                                                  handleReset
                                                      │
                                                      ▼
                                                     hero (전체 초기화)
```

### 7.3 핵심 이벤트 핸들러 분석

**`handleInfoSubmit`:**
```typescript
const handleInfoSubmit = (info: ChildInfo) => {
  setChildInfo(info);   // 아동 정보 저장
  setStep('guide');     // 다음 단계로 전이
};
```
- InfoForm으로부터 유효성 검증이 완료된 `ChildInfo` 객체를 수신하여 상태에 저장

**`handleUpload` (비동기):**
```typescript
const handleUpload = async (uploadedFile: File) => {
  setFile(uploadedFile);     // 파일 참조 저장
  setStep('loading');        // 즉시 로딩 화면 전환 (낙관적 UI 업데이트)

  try {
    const analysisResult = await uploadImage(uploadedFile, childInfo);
    setResult(analysisResult);
    setStep('result');       // 성공 시 결과 화면으로 전이
  } catch (error) {
    console.error('Analysis failed:', error);
    alert('분석 중 오류가 발생했습니다. 잠시 후 다시 시도해주세요.');
    setStep('upload');       // 실패 시 업로드 단계로 롤백
    setFile(null);           // 파일 상태 초기화
  }
};
```

**설계 결정:**

- **낙관적 UI 업데이트(Optimistic UI):** API 호출 전에 즉시 로딩 화면으로 전환하여 사용자에게 즉각적인 피드백 제공
- **에러 핸들링 전략:** 에러 발생 시 사용자를 업로드 단계로 롤백하여 재시도 기회 제공. `alert()` 대신 토스트 메시지로 개선 여지 있음
- **Graceful Degradation:** 네트워크 오류나 서버 장애 시에도 전체 앱이 크래시되지 않고 이전 단계로 복귀

**`handleReset`:**
```typescript
const handleReset = () => {
  setStep('hero');
  setChildInfo(null);
  setFile(null);
  setResult(null);
};
```
- 모든 상태를 초기값으로 되돌려 새로운 분석 세션을 시작

### 7.4 페이지 스크롤 제어

```typescript
useEffect(() => {
  window.scrollTo({ top: 0, behavior: 'smooth' });
}, [step]);
```

단계 전환 시 `smooth` 스크롤로 페이지 최상단으로 이동하여 사용자가 새로운 단계의 시작점을 인지할 수 있게 한다.

---

## 8. 컴포넌트 상세 분석

### 8.1 Hero 컴포넌트

**파일:** `src/components/Hero.tsx`  
**역할:** 서비스의 첫 인상을 결정하는 랜딩 화면  
**Props:** `onStart: () => void`

**구현 특징:**

```typescript
export const Hero: React.FC<HeroProps> = ({ onStart }) => {
  return (
    <section className="min-h-screen flex flex-col items-center justify-center
                         bg-gradient-to-b from-blue-50 to-white ...">
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.8 }}
      >
        {/* 타이틀, 설명, CTA 버튼 */}
      </motion.div>

      <motion.div
        animate={{ y: [0, 10, 0] }}
        transition={{ repeat: Infinity, duration: 2 }}
      >
        <ChevronDown size={32} />  {/* 스크롤 유도 아이콘 */}
      </motion.div>
    </section>
  );
};
```

**디자인 분석:**

- **풀스크린 레이아웃:** `min-h-screen`으로 뷰포트 전체를 차지하여 집중력 확보
- **그래디언트 배경:** `bg-gradient-to-b from-blue-50 to-white` — 부드러운 파란색에서 흰색으로의 전환, 안정감 전달
- **Fade-Up 진입 애니메이션:** 컨텐츠가 아래에서 위로 부드럽게 올라오며 나타남 (`opacity: 0→1`, `y: 20→0`)
- **무한 바운스 아이콘:** 하단의 ChevronDown 아이콘이 무한 반복 바운스하여 스크롤 가능함을 암시
- **한국어 카피라이팅:** "우리 아이의 마음, 그림으로 이해해보세요" — 감성적 접근

### 8.2 InfoForm 컴포넌트

**파일:** `src/components/InfoForm.tsx`  
**역할:** 아동 기본 정보 수집 폼  
**Props:** `onSubmit: (info: ChildInfo) => void`

**구현 특징:**

- **Controlled Component:** 각 입력 필드가 `useState`로 관리되는 `info` 상태 객체에 바인딩
- **성별 선택 UI:** 라디오 버튼 대신 시각적 토글 버튼 사용. 선택된 성별에 따라 `bg-blue-50`(남) / `bg-pink-50`(여)으로 색상 변경
- **HTML5 유효성 검증:** `required` 속성 활용 + `handleSubmit`에서 이중 검증
- **시맨틱 HTML:** `<fieldset>`, `<legend>`, `<label htmlFor>` 사용으로 스크린 리더 접근성 확보
- **Framer Motion `whileInView`:** 뷰포트에 진입할 때 애니메이션 트리거 (스크롤 인터랙션)

```typescript
const handleSubmit = (e: React.FormEvent) => {
  e.preventDefault();
  if (info.name && info.birthDate) {  // 필수 필드 이중 검증
    onSubmit(info);
  }
};
```

### 8.3 Guide 컴포넌트

**파일:** `src/components/Guide.tsx`  
**역할:** 정확한 분석을 위한 그림 그리기 안내  
**Props:** `onNext: () => void`

**구현 특징:**

- **카드 그리드 레이아웃:** 3열 그리드(`md:grid-cols-3`)로 3가지 가이드 포인트를 카드 형태로 제시
  1. 🖊 **전신 그리기:** 머리부터 발끝까지 전체 모습
  2. ☀ **자유로운 표현:** 지우개 사용 가능, 편안한 환경
  3. 😊 **간섭 금지:** 부모의 조언 없이 아이 스스로 그리기
- **아이콘 활용:** Lucide React의 `Pencil`, `Sun`, `Smile` 아이콘으로 시각적 인지 지원
- **Staggered 애니메이션:** 카드가 순차적으로 나타남 (`transition={{ delay: idx * 0.1 }}`)
- **고유 키(Key) 사용:** `key={item.id}`로 안정적인 리스트 렌더링 보장 (인덱스 기반 키 대신)

### 8.4 Upload 컴포넌트

**파일:** `src/components/Upload.tsx`  
**역할:** 이미지 파일 업로드 (드래그&드롭 + 클릭)  
**Props:** `onUpload: (file: File) => void`

**구현 특징:**

```typescript
const handleFile = useCallback((file: File) => {
  if (!file.type.startsWith('image/')) return;  // 이미지 파일만 허용 (클라이언트 검증)

  const reader = new FileReader();
  reader.onload = (e) => {
    setPreview(e.target?.result as string);  // Data URL로 미리보기 생성
  };
  reader.readAsDataURL(file);

  setTimeout(() => onUpload(file), 1000);  // 1초 딜레이 후 실제 업로드 콜백
}, [onUpload]);
```

**핵심 설계 결정:**

- **클라이언트 사이드 파일 검증:** `file.type.startsWith('image/')` — MIME 타입 기반 이미지 파일만 허용
- **FileReader API 활용:** `readAsDataURL`로 서버 통신 없이 즉석 이미지 미리보기 제공
- **1초 딜레이:** 미리보기가 렌더링될 시간을 확보하는 UX 딜레이. 사용자가 업로드된 이미지를 확인한 후 분석이 시작되는 인상을 줌
- **드래그&드롭 구현:** `onDragOver`, `onDragLeave`, `onDrop` 이벤트 핸들러 조합
- **시각적 피드백:** 드래그 상태에 따라 테두리 색상 변경 (`isDragging` 상태)
- **미리보기 취소:** 미리보기 상태에서 X 버튼으로 이미지 제거 가능 (`setPreview(null)`)

```typescript
// useCallback으로 메모이제이션 — onUpload prop 변경 시에만 핸들러 재생성
const onDrop = useCallback((e: React.DragEvent) => {
  e.preventDefault();
  setIsDragging(false);
  if (e.dataTransfer.files?.[0]) {
    handleFile(e.dataTransfer.files[0]);
  }
}, [handleFile]);
```

### 8.5 Loading 컴포넌트

**파일:** `src/components/Loading.tsx`  
**역할:** AI 분석 진행 중 시각적 피드백

**구현 특징:**

- **풀스크린 오버레이:** `fixed inset-0`으로 전체 화면을 덮는 로딩 레이어
- **이중 도형 애니메이션:** 사각형(primary 색상)과 원형(secondary 색상)이 서로 반대 방향으로 회전하며 펄스
  - 외곽 사각형: `scale: [1, 1.2, 1]`, `rotate: [0, 180, 360]`
  - 내부 원형: `scale: [1.2, 1, 1.2]`, `rotate: [0, -180, -360]`
- **부드러운 무한 반복:** `repeat: Infinity`, `ease: "easeInOut"`

**UX 의도:**

- 단순한 스피너 대신 브랜드 색상을 활용한 독특한 로딩 애니메이션으로 대기 시간의 지루함 감소
- "AI가 그림을 분석하고 있습니다" 텍스트와 "잠시만 기다려주세요..." 부가 텍스트로 진행 상황 안내

### 8.6 Result 컴포넌트

**파일:** `src/components/Result.tsx`  
**역할:** AI 분석 결과의 시각화 및 인터랙션 (PDF 다운로드, 공유, 재시도)  
**Props:** `childName`, `childGender`, `childAge`, `imageFile`, `result`, `onReset`

**이 컴포넌트는 프론트엔드 모듈에서 가장 복잡하고 핵심적인 컴포넌트이다.**

**구조 분해:**

```
Result
├── TreeVisual (인지 발달 나무 시각화)
├── DistributionGraph (정규분포 곡선)
├── Report Container (PDF 캡처 대상)
│   ├── 헤더 (제목, 날짜, 아동 정보)
│   ├── 좌측 컬럼
│   │   ├── 원본 그림 표시
│   │   └── 인지 발달 나무
│   └── 우측 컬럼
│       ├── 또래 대비 분포 그래프
│       ├── 종합 해석 텍스트
│       └── 부모님을 위한 팁
├── 면책 조항
└── 액션 버튼 (PDF 저장, 공유, 다시하기)
```

---

## 9. API 통신 계층

### 9.1 Axios 인스턴스 (`client.ts`)

```typescript
const client = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api',  // 환경 변수 기반 URL
  timeout: 10000,  // 10초 타임아웃
});

// 인터셉터: 향후 JWT 인증 토큰 주입 위치
client.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  // TODO: JWT 인증 구현 시 Authorization 헤더 설정
  return config;
});
```

**설계 고려사항:**

- **환경 변수 주입:** `VITE_API_URL` 환경 변수로 개발/스테이징/프로덕션 환경 간 API URL 전환
- **기본값 `/api`:** Vite 프록시 설정과 연동하여 개발 시 CORS 문제 회피
- **인터셉터 확장 포인트:** 향후 인증 토큰, 요청 로깅, 에러 표준화 등 횡단 관심사(Cross-Cutting Concern) 추가 가능

### 9.2 이미지 업로드 API (`uploadApi.ts`)

**Mock/Real 이중 모드 설계:**

```typescript
export const uploadImage = async (
  file: File,
  childInfo: ChildInfo | null
): Promise<AnalysisResult> => {
  const useMock = import.meta.env.VITE_USE_MOCK !== 'false';

  if (!useMock) {
    // 실제 API 호출
    const formData = new FormData();
    formData.append('image', file);
    if (childInfo) {
      formData.append('childInfo', JSON.stringify(childInfo));
    }
    const response = await client.post<AnalysisResult>('/analyze', formData, {
      headers: { 'Content-Type': 'multipart/form-data' },
    });
    return response.data;
  }

  // Mock 모드: 3초 딜레이 후 더미 데이터 반환
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(generateMockResult(childInfo?.name || '아이'));
    }, 3000);
  });
};
```

**Mock 데이터 생성 전략:**

```typescript
const generateMockResult = (childName: string): AnalysisResult => {
  const mockScore = Math.floor(Math.random() * (95 - 70) + 70);  // 70~95점 랜덤
  return {
    score: mockScore,
    percentile: Math.floor(Math.random() * (99 - 60) + 60),
    date: new Date().toLocaleDateString(),
    interpretation: `${childName} 어린이는 그림을 통해 풍부한 상상력을 표현하고 있습니다.\n\n...`
  };
};
```

**설계 결정 근거:**

- **환경 변수 기반 Mock 제어:** `VITE_USE_MOCK`이 명시적으로 `'false'`일 때만 실제 API 호출. 기본값은 Mock 모드로 백엔드 없이 독립적 프론트엔드 개발 가능
- **3초 딜레이:** 실제 AI 분석 시간을 시뮬레이션하여 로딩 화면 UX 검증
- **현실적 Mock 데이터:** 점수는 70~95 범위로 실제에 가까운 분포 생성

---

## 10. 시각화 엔진

### 10.1 TreeVisual — 인지 발달 나무

Result 컴포넌트 내부에 정의된 서브 컴포넌트로, 인지 발달 점수를 **나무에 물이 차오르는** 메타포로 시각화한다.

**구현 원리:**

1. **배경 나무 (윤곽):** SVG path로 나무 형상 정의, `opacity-20`으로 반투명 표시
2. **채워지는 나무 (레벨):** 동일한 SVG path에 그래디언트(`#15803d` → `#4ade80`) 적용
3. **높이 애니메이션:** `height: 0% → level%`로 아래에서 위로 차오르는 효과

```typescript
const TreeVisual = ({ level }: { level: number }) => (
  <motion.div
    initial={{ height: "0%" }}
    animate={{ height: `${level}%` }}
    transition={{ duration: 1.5, ease: "easeOut" }}
  >
    <svg viewBox="0 0 200 300" preserveAspectRatio="xMidYBottom slice">
      <path d="M100,280 L100,250 C100,250 ..." fill="url(#treeGradient)" />
      <defs>
        <linearGradient id="treeGradient" x1="0" x2="0" y1="1" y2="0">
          <stop offset="0%" stopColor="#15803d" />    {/* 진한 초록 */}
          <stop offset="100%" stopColor="#4ade80" />   {/* 밝은 초록 */}
        </linearGradient>
      </defs>
    </svg>
  </motion.div>
);
```

**SVG 기술 상세:**

- `viewBox="0 0 200 300"`: 200×300 좌표 공간에서 나무 형상 정의
- `preserveAspectRatio="xMidYBottom slice"`: 하단 정렬, 비율 유지 클리핑
- 경로(path): 베지어 곡선으로 줄기와 수관을 하나의 연속된 형상으로 표현

### 10.2 DistributionGraph — 정규분포 곡선

또래 집단 내에서 아동의 위치를 정규분포(가우시안) 곡선 위에 표시한다.

**수학적 모델:**

```typescript
// 백분위 → x좌표 매핑 (선형 변환)
const xPos = 30 + (percentile / 100) * 240;

// 가우시안 근사: y좌표 = 140 - e^(-(percentile-50)²/400) × 120
const yPos = 140 - (Math.exp(-Math.pow((percentile - 50) / 20, 2)) * 120);
```

**구현 특징:**

- 벨 커브는 SVG `<path>` 3차 베지어 곡선으로 근사
- 인디케이터 포인트가 좌하단에서 계산된 위치까지 이동하는 애니메이션
- "상위 N%" 텍스트 라벨이 딜레이 후 페이드인
- x축 레이블: "낮음 — 평균 — 높음"

### 10.3 PDF 다운로드 메커니즘

```typescript
const handleDownloadPDF = async () => {
  if (!reportRef.current) return;

  // Step 1: DOM → Canvas 캡처
  const canvas = await html2canvas(reportRef.current, {
    scale: 2,          // 2배 해상도로 캡처 (레티나 대응)
    logging: false,
    useCORS: true       // 외부 이미지 CORS 처리
  });

  // Step 2: Canvas → PNG Data URL
  const imgData = canvas.toDataURL('image/png');

  // Step 3: jsPDF로 A4 PDF 생성
  const pdf = new jsPDF({
    orientation: 'portrait',
    unit: 'mm',
    format: 'a4'
  });

  // Step 4: 이미지를 A4 비율에 맞게 삽입
  const imgWidth = 210;  // A4 가로 = 210mm
  const imgHeight = (canvas.height * imgWidth) / canvas.width;
  pdf.addImage(imgData, 'PNG', 0, 0, imgWidth, imgHeight);

  // Step 5: 파일명을 아이 이름으로 설정하여 다운로드
  pdf.save(`${childName}_그림분석결과.pdf`);
};
```

**기술적 고려사항:**

- **`scale: 2`:** 일반 모니터(1x)에서도 인쇄 품질의 고해상도 PDF 생성
- **`useCORS: true`:** 업로드된 이미지의 Blob URL이 Canvas에서도 접근 가능하도록 CORS 설정
- **클라이언트 사이드 렌더링:** 서버 요청 없이 브라우저에서 직접 PDF 생성. 개인정보가 서버로 전송되지 않음

---

## 11. UI/UX 디자인 시스템

### 11.1 레이아웃 패턴

모든 섹션 컴포넌트는 다음의 공통 레이아웃 패턴을 따른다:

```html
<section className="min-h-screen flex flex-col items-center justify-center px-4 py-20">
  <div className="max-w-{width}">
    {/* 컨텐츠 */}
  </div>
</section>
```

- **최소 높이 100vh:** 각 단계가 최소한 전체 뷰포트를 차지
- **수직/수평 중앙 정렬:** Flexbox 기반 완전 중앙 배치
- **최대 너비 제한:** 가독성을 위한 컨텐츠 폭 제한 (`max-w-md` ~ `max-w-4xl`)
- **패딩:** 좌우 `px-4`(모바일 여백), 상하 `py-20`(섹션 간격)

### 11.2 버튼 디자인 시스템

| 유형 | 클래스 | 용도 |
|---|---|---|
| Primary CTA | `bg-primary text-white rounded-full shadow-lg` | 주요 액션 (시작, 다음) |
| Secondary | `bg-slate-900 text-white rounded-lg` | 폼 제출 |
| Ghost | `bg-white border text-slate-700 rounded-full` | 보조 액션 (공유, 다시하기) |

### 11.3 인터랙션 패턴

- **Hover 확대:** `transform hover:scale-105` — CTA 버튼에 미세한 확대 효과
- **포커스 링:** `focus:ring-2 focus:ring-primary` — 키보드 탐색 시 시각적 피드백
- **Transition:** `transition-all` — 모든 속성 변경에 부드러운 전환 적용

---

## 12. 테스트 전략

### 12.1 테스트 환경 구성

**프레임워크:** Vitest (Vite 네이티브 테스트 러너)

```typescript
// vite.config.ts
test: {
  globals: true,           // describe, it, expect 전역
  environment: 'jsdom',    // DOM 시뮬레이션
  setupFiles: './src/setupTests.ts',
  css: true,
}
```

**테스트 전 설정 (`setupTests.ts`):**

```typescript
import '@testing-library/jest-dom';  // DOM 매처 확장 (toBeInTheDocument, toHaveClass 등)
import { vi } from 'vitest';

// IntersectionObserver 모의 객체
// Framer Motion의 whileInView가 IntersectionObserver에 의존하므로 테스트 환경에서 모킹 필수
class IntersectionObserverMock {
  observe = vi.fn();
  disconnect = vi.fn();
  unobserve = vi.fn();
  takeRecords = vi.fn();
}

vi.stubGlobal('IntersectionObserver', IntersectionObserverMock);
```

**IntersectionObserver 모킹 이유:**

- `jsdom`은 `IntersectionObserver`를 제공하지 않음
- Framer Motion의 `whileInView` 프로퍼티가 `IntersectionObserver`를 내부적으로 사용
- 모킹 없이 테스트 실행 시 `ReferenceError: IntersectionObserver is not defined` 발생

### 12.2 테스트 라이브러리

| 라이브러리 | 용도 |
|---|---|
| `@testing-library/react` | 컴포넌트 렌더링 테스트 |
| `@testing-library/jest-dom` | DOM 상태 매처 확장 |
| `@testing-library/user-event` | 사용자 인터랙션 시뮬레이션 |

---

## 13. 성능 최적화

### 13.1 현재 적용된 최적화

| 기법 | 적용 위치 | 효과 |
|---|---|---|
| `useCallback` | Upload의 파일 핸들러 | 불필요한 핸들러 재생성 방지 |
| Tree Shaking | TailwindCSS Purge | 미사용 CSS 제거 |
| `React.StrictMode` | main.tsx | 개발 시 이중 렌더링으로 부수효과 감지 |
| 조건부 렌더링 | App.tsx | 현재 단계의 컴포넌트만 마운트 |
| SVG 인라인 | TreeVisual, DistributionGraph | 외부 이미지 로딩 없이 즉각 렌더링 |

### 13.2 향후 최적화 가능 영역

- **코드 스플리팅:** `React.lazy()` + `Suspense`로 Result 컴포넌트 지연 로딩 (html2canvas, jsPDF 번들 분리)
- **이미지 압축:** 업로드 전 클라이언트 사이드 이미지 리사이즈/압축
- **Service Worker:** 오프라인 지원 및 캐싱 전략
- **메모이제이션:** `React.memo()`를 Guide 등 정적 컴포넌트에 적용

---

## 14. 확장성 및 향후 개선 방향

### 14.1 아키텍처 확장

- **라우팅 도입:** React Router 적용으로 멀티 페이지 지원 (이력 조회, 설정 등)
- **상태 관리 라이브러리:** 기능 확장 시 Zustand 또는 Jotai 도입 검토
- **국제화(i18n):** react-i18next로 다국어 지원

### 14.2 기능 확장

- **분석 이력 저장:** LocalStorage 또는 서버 사이드 사용자 계정 기반 이력 관리
- **비교 분석:** 동일 아동의 시간 경과에 따른 발달 추이 그래프
- **전문가 연결:** 분석 결과 기반 전문 상담사 연결 기능
- **접근성 강화:** WCAG 2.1 AA 수준의 접근성 기준 충족

### 14.3 인프라 개선

- **CI/CD 파이프라인:** GitHub Actions를 통한 자동 빌드/테스트/배포
- **E2E 테스트:** Playwright 또는 Cypress를 활용한 사용자 플로우 자동 테스트
- **모니터링:** Sentry.io 연동으로 프로덕션 에러 추적

---

## 15. 결론

Mind Palette Frontend 모듈은 React 18, TypeScript, Vite, TailwindCSS, Framer Motion을 기반으로 한 현대적인 싱글 페이지 웹 애플리케이션이다. **6단계 위저드 패턴**의 직관적인 사용자 경험과, **SVG 기반 데이터 시각화**(인지 발달 나무, 정규분포 곡선), 그리고 **클라이언트 사이드 PDF 생성** 기능을 통해 심리학적 전문 분석 결과를 비전문가인 부모가 쉽게 이해하고 보관할 수 있도록 설계되었다.

특히 **환경 변수 기반 Mock/Real 이중 모드 API 계층**은 백엔드 개발과 독립적인 프론트엔드 개발을 가능하게 하여, 마이크로서비스 아키텍처에서의 병렬 개발 효율성을 극대화한다.

현재 아키텍처는 소규모 MVP에 적합한 **Lifting State Up** 패턴을 채택하고 있으나, 향후 기능 확장에 따라 상태 관리 라이브러리 도입, 라우팅 시스템 구축, 코드 스플리팅 등의 점진적 개선이 권장된다.

---

**부록: 파일 목록 및 코드 라인 수**

| 파일 | 라인 수 | 주요 역할 |
|---|---|---|
| `App.tsx` | 82 | 최상위 상태 관리 + 위저드 제어 |
| `Hero.tsx` | 46 | 랜딩 화면 |
| `InfoForm.tsx` | 99 | 아동 정보 입력 폼 |
| `Guide.tsx` | 57 | 그림 그리기 가이드 |
| `Upload.tsx` | 98 | 이미지 업로드 (D&D) |
| `Loading.tsx` | 44 | 로딩 애니메이션 |
| `Result.tsx` | 246 | 결과 시각화 + PDF |
| `client.ts` | 19 | Axios 인스턴스 |
| `uploadApi.ts` | 55 | API 통신 + Mock |
| `types/index.ts` | 14 | 타입 정의 |
| `setupTests.ts` | 13 | 테스트 모킹 |
| **합계** | **773** | |
