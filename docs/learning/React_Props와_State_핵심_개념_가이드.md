# React Props와 State 핵심 개념 가이드

> Mind Palette 프로젝트를 통해 학습한 React의 핵심 데이터 관리 패턴

## 📚 목차
1. [핵심 개념 요약](#핵심-개념-요약)
2. [State (상태) 이해하기](#state-상태-이해하기)
3. [Props (속성) 이해하기](#props-속성-이해하기)
4. [App.tsx 구조 분석](#apptsx-구조-분석)
5. [컴포넌트 호출 관계 및 데이터 흐름](#컴포넌트-호출-관계-및-데이터-흐름)
6. [C++와의 비교](#c와의-비교)
7. [실전 예제](#실전-예제)

---

## 핵심 개념 요약

### React의 핵심 데이터 관리 패턴

```
┌─────────────────────────────────────────┐
│  State (상태) = 데이터 저장소           │
│  - 부모 컴포넌트가 관리                 │
│  - setState 함수로만 변경 가능          │
│  - 변경 시 자동 리렌더링                │
└─────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│  Props (속성) = 데이터 전달 통로        │
│  - 부모 → 자식으로 전달                 │
│  - 읽기 전용 (수정 불가)                │
│  - 데이터 + 함수 모두 전달 가능         │
└─────────────────────────────────────────┘
```

**핵심 원칙:**
- **State로 데이터 관리 (부모)**
- **Props로 전달 (부모 → 자식)**
- **콜백 함수로 응답 (자식 → 부모)**

---

## State (상태) 이해하기

### State란?
컴포넌트가 관리하는 **동적 데이터**입니다. State가 변경되면 React가 자동으로 화면을 다시 그립니다.

### State 문법

```typescript
// 기본 구조
const [변수명, 변경함수] = useState<타입>(초기값);
      ↓        ↓                      ↓
    현재값   세터함수              초기값

// 실제 예시
const [step, setStep] = useState<Step>('hero');
```

### Mind Palette의 State 구조

```typescript
// App.tsx의 4가지 핵심 State
function App() {
  // 1. 현재 진행 단계
  const [step, setStep] = useState<Step>('hero');
  
  // 2. 자녀 정보
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
  
  // 3. 업로드된 파일
  const [file, setFile] = useState<File | null>(null);
  
  // 4. 분석 결과
  const [result, setResult] = useState<AnalysisResult | null>(null);
}
```

### State 변경 방법

```typescript
// ❌ 잘못된 방법 - 직접 변경 불가!
step = 'form';  // ERROR!

// ✅ 올바른 방법 - setter 함수 사용
setStep('form');  // OK!
```

### State 특징
- ✅ 값이 변경되면 **자동으로 화면이 업데이트**됨
- ✅ 컴포넌트의 **메모리 역할**
- ✅ 비동기적으로 업데이트됨
- ❌ 직접 수정 불가능 (setter 함수만 사용)

---

## Props (속성) 이해하기

### Props란?
**부모 컴포넌트**가 **자식 컴포넌트**에게 전달하는 데이터 또는 함수입니다.

### Props 문법

```typescript
// 1. Props 인터페이스 정의
interface HeroProps {
  onStart: () => void;  // 함수 타입
}

// 2. 컴포넌트에서 Props 받기
export const Hero: React.FC<HeroProps> = ({ onStart }) => {
  // onStart 사용 가능
  return <button onClick={onStart}>시작하기</button>;
};

// 3. 부모가 Props 전달
<Hero onStart={() => setStep('form')} />
```

### Props의 세 가지 전달 패턴

#### 1️⃣ **함수만 전달** (상태 변경 트리거)

```typescript
// 부모 (App.tsx)
<Hero onStart={() => setStep('form')} />

// 자식 (Hero.tsx)
const Hero = ({ onStart }) => {
  return <button onClick={onStart}>시작</button>;
};
```

#### 2️⃣ **함수 전달 + 데이터 수신** (자식 → 부모로 데이터 전송)

```typescript
// 부모 (App.tsx)
const handleInfoSubmit = (info: ChildInfo) => {
  setChildInfo(info);  // 자식이 보낸 데이터 저장
  setStep('guide');
};

<InfoForm onSubmit={handleInfoSubmit} />

// 자식 (InfoForm.tsx)
const InfoForm = ({ onSubmit }) => {
  const handleSubmit = () => {
    const info = { name: "철수", gender: "male", ... };
    onSubmit(info);  // 부모에게 데이터 전송
  };
};
```

#### 3️⃣ **데이터 전달** (부모 → 자식으로 데이터 전송)

```typescript
// 부모 (App.tsx)
<Result 
  childName={childInfo.name}
  result={result}
  onReset={handleReset}
/>

// 자식 (Result.tsx)
const Result = ({ childName, result, onReset }) => {
  return (
    <div>
      <h2>{childName}의 결과</h2>
      <p>점수: {result.score}</p>
    </div>
  );
};
```

### Props 특징
- ✅ **읽기 전용** (수정 불가)
- ✅ 데이터와 함수 모두 전달 가능
- ✅ 자식은 Props를 통해서만 부모와 소통
- ❌ Props를 직접 변경할 수 없음

---

## App.tsx 구조 분석

### 전체 구조

```typescript
import { useState, useEffect } from 'react';
import { Hero, InfoForm, Guide, Upload, Loading, Result } from './components';

type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result';

function App() {
  // ========== State 선언 ==========
  const [step, setStep] = useState<Step>('hero');
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
  const [file, setFile] = useState<File | null>(null);
  const [result, setResult] = useState<AnalysisResult | null>(null);

  // ========== 이벤트 핸들러 ==========
  const handleInfoSubmit = (info: ChildInfo) => {
    setChildInfo(info);
    setStep('guide');
  };

  const handleUpload = (uploadedFile: File) => {
    setFile(uploadedFile);
    setStep('loading');
    
    // Mock 분석 프로세스
    setTimeout(() => {
      setResult({ /* 분석 결과 */ });
      setStep('result');
    }, 3000);
  };

  const handleReset = () => {
    setStep('hero');
    setChildInfo(null);
    setFile(null);
    setResult(null);
  };

  // ========== 조건부 렌더링 ==========
  return (
    <div>
      {step === 'hero' && <Hero onStart={() => setStep('form')} />}
      {step === 'form' && <InfoForm onSubmit={handleInfoSubmit} />}
      {step === 'guide' && <Guide onNext={() => setStep('upload')} />}
      {step === 'upload' && <Upload onUpload={handleUpload} />}
      {step === 'loading' && <Loading />}
      {step === 'result' && childInfo && result && (
        <Result 
          childName={childInfo.name}
          childGender={childInfo.gender}
          childAge={childInfo.birthDate}
          imageFile={file}
          result={result}
          onReset={handleReset}
        />
      )}
    </div>
  );
}
```

### App.tsx의 역할

1. **중앙 상태 관리소** 🏢
   - 모든 중요한 데이터를 State로 관리
   - Single Source of Truth (단일 진실 공급원)

2. **교통 통제소** 🚦
   - step 변수로 현재 화면 제어
   - 조건부 렌더링으로 적절한 컴포넌트만 표시

3. **데이터 중계소** 📡
   - 자식 컴포넌트 간 데이터 전달 중개
   - Props를 통해 데이터와 함수 배포

---

## 컴포넌트 호출 관계 및 데이터 흐름

### 전체 흐름 다이어그램

```
┌─────────────────────────────────────────────────────────────────┐
│                          App.tsx (부모)                          │
│                                                                 │
│  State 관리:                                                     │
│  • step: 'hero' | 'form' | 'guide' | 'upload' | 'loading' | ... │
│  • childInfo: { name, gender, birthDate }                       │
│  • file: File                                                   │
│  • result: { score, percentile, interpretation, ... }           │
│                                                                 │
│  Handler 함수:                                                   │
│  • handleInfoSubmit(info) → childInfo 저장, step='guide'       │
│  • handleUpload(file) → file 저장, 분석 시작                    │
│  • handleReset() → 모든 상태 초기화                             │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
  조건부 렌더링         조건부 렌더링          조건부 렌더링
 (step 값에 따라)     (step 값에 따라)     (step 값에 따라)
```

### 단계별 데이터 흐름

#### **STEP 1: Hero (시작 화면)**

```
┌──────────────────────────┐
│   <Hero />               │
│   Props:                 │
│   • onStart: () => void  │
└──────────────────────────┘
            │
            │ 사용자 클릭: "무료로 분석 시작하기"
            ▼
      onStart() 실행
            │
            ▼
      setStep('form')
            │
            ▼
    다음 화면으로 이동
```

#### **STEP 2: InfoForm (정보 입력)**

```
┌──────────────────────────────────────┐
│   <InfoForm />                       │
│   Props:                             │
│   • onSubmit: (info: ChildInfo)      │
│              => void                 │
└──────────────────────────────────────┘
            │
            │ 사용자 입력: 이름, 성별, 생년월일
            ▼
      onSubmit(info) 실행
            │
            ▼
   handleInfoSubmit(info)
   {
     setChildInfo(info)  ← 📦 데이터 저장!
     setStep('guide')
   }
```

**데이터 흐름:**
```
InfoForm (자식)
  → info = { name: "철수", gender: "male", birthDate: "2018-05-20" }
  → onSubmit(info) 호출
  → App (부모)의 handleInfoSubmit(info) 실행
  → setChildInfo(info) - State에 저장됨! 📦
```

#### **STEP 3: Guide (가이드 화면)**

```
┌──────────────────────────┐
│   <Guide />              │
│   Props:                 │
│   • onNext: () => void   │
└──────────────────────────┘
            │
            │ 사용자 클릭: "준비되었어요"
            ▼
      onNext() 실행
            │
            ▼
      setStep('upload')
```

#### **STEP 4: Upload (파일 업로드)**

```
┌──────────────────────────────────┐
│   <Upload />                     │
│   Props:                         │
│   • onUpload: (file: File)       │
│              => void             │
└──────────────────────────────────┘
            │
            │ 사용자 액션: 파일 드래그 앤 드롭
            ▼
      onUpload(file) 실행
            │
            ▼
   handleUpload(uploadedFile)
   {
     setFile(uploadedFile)  ← 📦 파일 저장!
     setStep('loading')
     
     // 3초 후 분석 완료
     setTimeout(() => {
       setResult({...})     ← 📊 분석 결과 저장!
       setStep('result')
     }, 3000)
   }
```

#### **STEP 5: Loading (분석 중)**

```
┌──────────────────────────┐
│   <Loading />            │
│   Props: 없음            │
│   (애니메이션만 표시)    │
└──────────────────────────┘
            │
            │ 3초 대기 (AI 분석 시뮬레이션)
            ▼
    자동으로 setStep('result')
```

#### **STEP 6: Result (결과 화면)**

```
┌─────────────────────────────────────┐
│   <Result />                        │
│   Props:                            │
│   • childName: string ←─────────────┼─ childInfo.name
│   • childGender: string ←───────────┼─ childInfo.gender
│   • childAge: string ←──────────────┼─ childInfo.birthDate
│   • imageFile: File ←───────────────┼─ file
│   • result: AnalysisResult ←────────┼─ result
│   • onReset: () => void             │
└─────────────────────────────────────┘
            │
            │ 사용자 클릭: "다시하기"
            ▼
      onReset() 실행
            │
            ▼
   handleReset()
   {
     setStep('hero')        ← 🔄 처음으로
     setChildInfo(null)     ← 🧹 초기화
     setFile(null)          ← 🧹 초기화
     setResult(null)        ← 🧹 초기화
   }
```

### 데이터 생명주기

```
1. 사용자 입력 (InfoForm)
   ↓
2. App State에 저장 (childInfo)
   ↓
3. 파일 업로드 (Upload)
   ↓
4. App State에 저장 (file)
   ↓
5. 분석 실행 & 결과 생성
   ↓
6. App State에 저장 (result)
   ↓
7. Result 컴포넌트로 전달
   ↓
8. 화면에 표시
   ↓
9. Reset 버튼으로 모든 State 초기화
   ↓
10. 다시 시작
```

---

## C++와의 비교

### Props = `const 참조` + `함수 포인터`

#### C++ 버전

```cpp
// Props를 구조체로 표현
struct InfoFormProps {
    const ChildInfo& data;           // 읽기 전용 데이터
    void (*onSubmit)(ChildInfo);     // 함수 포인터
};

// 컴포넌트 = 함수
void InfoForm(const InfoFormProps& props) {
    // ✅ 읽기는 가능
    cout << props.data.name;
    
    // ❌ 변경은 불가능 (const!)
    // props.data.name = "변경";  // ERROR!
    
    // ✅ 함수 실행은 가능
    ChildInfo newData = {"철수", "male", "2018-05-20"};
    props.onSubmit(newData);  // OK!
}

// 부모가 호출
void App() {
    ChildInfo childInfo;
    
    auto handleSubmit = [&](ChildInfo info) {
        childInfo = info;  // State 업데이트
    };
    
    InfoFormProps props = {
        childInfo,
        handleSubmit
    };
    
    InfoForm(props);
}
```

#### React (TypeScript) 버전

```typescript
// Props 인터페이스 정의
interface InfoFormProps {
  data: ChildInfo;              // 읽기 전용 (자동으로 const)
  onSubmit: (info: ChildInfo) => void;  // 함수
}

// 컴포넌트
const InfoForm = ({ data, onSubmit }: InfoFormProps) => {
  // ✅ 읽기 가능
  console.log(data.name);
  
  // ❌ 변경 불가능
  // data.name = "변경";  // ERROR!
  
  // ✅ 함수 실행 가능
  const newData = { name: "철수", gender: "male", birthDate: "2018-05-20" };
  onSubmit(newData);  // OK!
};

// 부모
const App = () => {
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
  
  const handleSubmit = (info: ChildInfo) => {
    setChildInfo(info);  // State 업데이트
  };
  
  return <InfoForm data={childInfo} onSubmit={handleSubmit} />;
};
```

### 비교표

| 특징 | C++ 참조 변수 | C++ const 참조 | React Props |
|------|---------------|----------------|-------------|
| **데이터 전달** | ✅ | ✅ | ✅ |
| **함수 전달** | ✅ (함수 포인터) | ✅ (함수 포인터) | ✅ |
| **원본 수정** | ✅ 가능 | ❌ 불가능 | ❌ 불가능 |
| **일치도** | 부분 일치 | ✅ **정확히 일치** | ✅ |

### 핵심 정리

```
React Props = C++의 const 참조 + 함수 포인터
            = 읽기 전용 데이터 + 실행 가능한 함수
```

---

## 실전 예제

### 예제 1: 간단한 카운터 (State 기초)

```typescript
function Counter() {
  const [count, setCount] = useState(0);
  
  return (
    <div>
      <p>카운트: {count}</p>
      <button onClick={() => setCount(count + 1)}>증가</button>
    </div>
  );
}
```

### 예제 2: 부모-자식 통신 (Props 기초)

```typescript
// 자식 컴포넌트
interface ButtonProps {
  label: string;
  onClick: () => void;
}

const Button = ({ label, onClick }: ButtonProps) => {
  return <button onClick={onClick}>{label}</button>;
};

// 부모 컴포넌트
const App = () => {
  const [count, setCount] = useState(0);
  
  const increment = () => setCount(count + 1);
  
  return (
    <div>
      <p>카운트: {count}</p>
      <Button label="증가" onClick={increment} />
    </div>
  );
};
```

### 예제 3: 폼 데이터 수집 (자식 → 부모 데이터 전송)

```typescript
// 자식: 입력 폼
interface FormProps {
  onSubmit: (name: string) => void;
}

const NameForm = ({ onSubmit }: FormProps) => {
  const [name, setName] = useState('');
  
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit(name);  // 부모에게 데이터 전송
  };
  
  return (
    <form onSubmit={handleSubmit}>
      <input 
        value={name}
        onChange={(e) => setName(e.target.value)}
      />
      <button type="submit">제출</button>
    </form>
  );
};

// 부모: 데이터 수신 및 저장
const App = () => {
  const [userName, setUserName] = useState('');
  
  const handleNameSubmit = (name: string) => {
    setUserName(name);  // 자식이 보낸 데이터 저장
    console.log('받은 이름:', name);
  };
  
  return (
    <div>
      <NameForm onSubmit={handleNameSubmit} />
      {userName && <p>안녕하세요, {userName}님!</p>}
    </div>
  );
};
```

### 예제 4: 다중 State 관리 (Mind Palette 패턴)

```typescript
type Page = 'home' | 'form' | 'result';

interface UserData {
  name: string;
  age: number;
}

const App = () => {
  // 여러 State 관리
  const [page, setPage] = useState<Page>('home');
  const [userData, setUserData] = useState<UserData | null>(null);
  
  // 핸들러
  const handleFormSubmit = (data: UserData) => {
    setUserData(data);
    setPage('result');
  };
  
  const handleReset = () => {
    setPage('home');
    setUserData(null);
  };
  
  // 조건부 렌더링
  return (
    <div>
      {page === 'home' && (
        <HomePage onStart={() => setPage('form')} />
      )}
      
      {page === 'form' && (
        <FormPage onSubmit={handleFormSubmit} />
      )}
      
      {page === 'result' && userData && (
        <ResultPage 
          userName={userData.name}
          userAge={userData.age}
          onReset={handleReset}
        />
      )}
    </div>
  );
};
```

---

## 핵심 원칙 요약

### ✅ DO (해야 할 것)

1. **State는 부모가 관리**
   ```typescript
   const [data, setData] = useState(initialValue);
   ```

2. **Props는 읽기만**
   ```typescript
   const Child = ({ data }) => {
     console.log(data);  // ✅ 읽기
   };
   ```

3. **함수로 부모에게 알림**
   ```typescript
   const Child = ({ onSubmit }) => {
     onSubmit(newData);  // ✅ 부모에게 데이터 전송
   };
   ```

4. **단방향 데이터 흐름 유지**
   ```
   부모 State → Props → 자식 표시
   자식 이벤트 → 콜백 → 부모 State 업데이트
   ```

### ❌ DON'T (하지 말아야 할 것)

1. **Props 직접 변경 금지**
   ```typescript
   const Child = ({ data }) => {
     data.name = "변경";  // ❌ ERROR!
   };
   ```

2. **State 직접 변경 금지**
   ```typescript
   const [count, setCount] = useState(0);
   count = 1;  // ❌ ERROR!
   ```

3. **자식 컴포넌트 간 직접 통신 금지**
   ```
   Child1 ←❌→ Child2
   
   올바른 방법:
   Child1 → Parent → Child2
   ```

---

## 마무리

### 핵심 공식

```
React 데이터 흐름 = State (저장) + Props (전달) + 콜백 (응답)
```

### 기억해야 할 세 가지

1. **State**: 부모가 데이터를 관리하는 금고 🔐
2. **Props**: 부모가 자식에게 보내는 택배 📦
3. **콜백**: 자식이 부모에게 거는 전화 ☎️

### C++ 개발자를 위한 한 줄 요약

```cpp
Props = const& (데이터) + function pointer (콜백)
```

---

## 참고 자료

- **프로젝트**: Mind Palette - 아동 인물화 지능측정 AI 시스템
- **작성일**: 2025년 11월 27일
- **관련 파일**:
  - `frontend/src/App.tsx`
  - `frontend/src/components/InfoForm.tsx`
  - `frontend/src/components/Hero.tsx`
  - `frontend/src/components/Result.tsx`

---

**💡 Tip**: 이 문서를 읽고 실제 코드와 비교하면서 학습하면 더 효과적입니다!

