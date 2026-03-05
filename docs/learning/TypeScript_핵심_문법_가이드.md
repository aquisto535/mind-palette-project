# Mind Palette 프로젝트 TypeScript 핵심 문법 가이드

> 이 문서는 Mind Palette 프로젝트의 프론트엔드 코드에서 실제로 사용된 TypeScript의 주요 문법과 패턴을 설명합니다.

## 📚 목차
1. [Interface (인터페이스)](#1-interface-인터페이스)
2. [Type Alias (타입 별칭)](#2-type-alias-타입-별칭)
3. [Union Type (유니온 타입)](#3-union-type-유니온-타입)
4. [Generic (제네릭)](#4-generic-제네릭)
5. [Optional Chaining (옵셔널 체이닝)](#5-optional-chaining-옵셔널-체이닝)
6. [Type Assertion (타입 단언)](#6-type-assertion-타입-단언)
7. [React Functional Component Type](#7-react-functional-component-type)

---

## 1. Interface (인터페이스)

**역할**: 객체의 구조(Shape)를 정의하는 가장 기본적인 방법입니다. 확장성이 좋아 주로 사용됩니다.

**프로젝트 예시 (`src/types/index.ts`)**:
```typescript
export interface ChildInfo {
  name: string;      // 문자열 타입
  gender: 'male' | 'female'; // 유니온 타입 (아래 참조)
  birthDate: string; // 문자열 타입 (YYYY-MM-DD)
}
```

**사용 예시 (`src/components/InfoForm.tsx`)**:
```typescript
// Props의 타입을 인터페이스로 정의
interface InfoFormProps {
  onSubmit: (info: ChildInfo) => void;
}
```

---

## 2. Type Alias (타입 별칭)

**역할**: `interface`와 비슷하지만, 객체뿐만 아니라 원시값, 유니온 타입, 튜플 등 다양한 타입을 정의할 때 사용됩니다.

**프로젝트 예시 (`src/App.tsx`)**:
```typescript
// 문자열 리터럴 타입을 사용하여 가능한 상태 값들을 제한
type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result';
```
> `Step` 타입은 정의된 문자열 중 하나만 가질 수 있습니다. 오타 방지에 매우 효과적입니다.

---

## 3. Union Type (유니온 타입)

**역할**: "A 또는 B"와 같이 여러 타입 중 하나가 될 수 있음을 의미합니다. `|` 기호를 사용합니다.

**프로젝트 예시**:

1. **문자열 선택지 제한 (`src/types/index.ts`)**:
   ```typescript
   gender: 'male' | 'female';
   ```
   `gender` 변수에는 오직 `'male'` 또는 `'female'` 문자열만 들어갈 수 있습니다.

2. **초기값 null 처리 (`src/App.tsx`)**:
   ```typescript
   // ChildInfo 객체이거나, 아직 데이터가 없는 null 상태
   const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
   ```

---

## 4. Generic (제네릭)

**역할**: 타입을 파라미터처럼 받아서 재사용성을 높이는 기능입니다. "타입의 변수"라고 생각하면 쉽습니다. `<Type>` 형태로 사용합니다.

**프로젝트 예시**:

1. **useState 훅 (`src/App.tsx`)**:
   ```typescript
   // useState 함수에게 "이 상태는 Step 타입만 취급해"라고 알려줌
   const [step, setStep] = useState<Step>('hero');
   ```

2. **Promise 비동기 반환 (`src/api/uploadApi.ts`)**:
   ```typescript
   // 이 비동기 함수는 완료되면 AnalysisResult 타입의 데이터를 반환한다고 약속
   export const uploadImage = async (...): Promise<AnalysisResult> => { ... }
   ```

---

## 5. Optional Chaining (옵셔널 체이닝)

**역할**: 객체의 속성에 접근할 때, 해당 객체가 `null`이나 `undefined`여도 에러를 발생시키지 않고 `undefined`를 반환하게 합니다. `?.` 연산자를 사용합니다.

**프로젝트 예시 (`src/api/uploadApi.ts`)**:
```typescript
// childInfo가 null일 수 있으므로 ?. 사용
// childInfo가 있으면 name을 반환, 없으면 undefined 반환 (뒤의 || 연산자로 기본값 처리)
return generateMockResult(childInfo?.name || '아이');
```

---

## 6. Type Assertion (타입 단언)

**역할**: 개발자가 컴파일러보다 해당 타입을 더 잘 알고 있다고 확신할 때, 타입을 강제로 지정하는 방법입니다. `as` 키워드를 사용합니다.

**프로젝트 예시 (`src/components/Upload.tsx`)**:
```typescript
const reader = new FileReader();
reader.onload = (e) => {
  // e.target.result의 타입을 string으로 확정지음
  setPreview(e.target?.result as string);
};
```

**주의**: 타입 단언은 실제 런타임 데이터를 바꾸지는 않으므로, 잘못 사용하면 런타임 에러가 발생할 수 있습니다. 꼭 필요한 경우에만 사용하세요.

---

## 7. React Functional Component Type

**역할**: React의 함수형 컴포넌트 타입을 명시합니다. 주로 `React.FC` (Function Component) 제네릭 타입을 사용합니다.

**프로젝트 예시 (`src/components/InfoForm.tsx`)**:
```typescript
// InfoForm은 InfoFormProps를 Props로 받는 함수형 컴포넌트다
export const InfoForm: React.FC<InfoFormProps> = ({ onSubmit }) => {
  // ...
};
```

> **참고**: 최신 React 커뮤니티에서는 `React.FC` 대신 아래처럼 직접 Props 타입을 명시하는 방식을 선호하기도 합니다.
> ```typescript
> export const InfoForm = ({ onSubmit }: InfoFormProps) => { ... }
> ```
> (우리 프로젝트는 명시적인 타입 확인을 위해 `React.FC`를 사용 중입니다.)

---

## 📝 요약: TypeScript 사용 패턴

1. **데이터 정의**: `interface` (확장성 고려)
2. **상태 값 제한**: `Type Alias` + `Union Type` (문자열 리터럴)
3. **함수/훅 타입 유연성**: `Generic` (`useState<Type>`)
4. **안전한 접근**: `Optional Chaining` (`obj?.prop`)
5. **타입 강제**: `Type Assertion` (`as string`)

이 가이드를 참고하여 프로젝트의 TypeScript 코드를 이해하고 활용해 보세요!

