import { useState, useEffect } from 'react';
import { Hero } from './components/Hero';
import { InfoForm } from './components/InfoForm';
import { Guide } from './components/Guide';
import { Upload } from './components/Upload';
import { Loading } from './components/Loading';
import { Result } from './components/Result';
import { ErrorScreen } from './components/Error';
import { ChildInfo, AnalysisResult } from './types';
import { AnalysisError } from './types/errors';
import { uploadImage } from './api/uploadApi';

type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result' | 'error';

function App() {
  const [step, setStep] = useState<Step>('hero');
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
  const [file, setFile] = useState<File | null>(null);
  const [result, setResult] = useState<AnalysisResult | null>(null);
  const [analysisError, setAnalysisError] = useState<AnalysisError | null>(null);

  // Smooth scroll to top when step changes
  useEffect(() => {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }, [step]); // 단계가 변경될 때마다 페이지 맨 위로 스크롤

  const handleInfoSubmit = (info: ChildInfo) => {
    setChildInfo(info); // 자녀 정보 저장
    setStep('guide'); // 다음 단계로 이동
  };

  const handleUpload = async (uploadedFile: File) => {
    setFile(uploadedFile); // 업로드된 파일 저장
    setStep('loading'); // 다음 단계로 이동

    try {
      // API 호출 (Mock 또는 실제)
      const analysisResult = await uploadImage(uploadedFile, childInfo);

      setResult(analysisResult);
      setStep('result');
    } catch (error) {
      if (error instanceof AnalysisError) {
        setAnalysisError(error);
      } else {
        setAnalysisError(new AnalysisError(0, '예기치 못한 오류가 발생했습니다. 잠시 후 다시 시도해주세요.', 'retry-later'));
      }
      setStep('error');
      setFile(null);
    }
  };

  const handleReset = () => {
    setStep('hero');
    setChildInfo(null);
    setFile(null);
    setResult(null);
    setAnalysisError(null);
  };

  return (
    <div className="min-h-screen bg-white text-slate-900">
      {step === 'hero' ? <Hero onStart={() => setStep('form')} /> : null}

      {step === 'form' ? <InfoForm onSubmit={handleInfoSubmit} /> : null}

      {step === 'guide' ? <Guide onNext={() => setStep('upload')} /> : null}

      {step === 'upload' ? <Upload onUpload={handleUpload} /> : null}

      {step === 'loading' ? <Loading /> : null}

      {step === 'error' && analysisError ? (
        <ErrorScreen
          error={analysisError}
          onRetry={() => { setAnalysisError(null); setStep('upload'); }}
          onGuide={() => { setAnalysisError(null); setStep('guide'); }}
        />
      ) : null}

      {step === 'result' && childInfo && result ? (
        <Result
          childName={childInfo.name}
          childGender={childInfo.gender}
          childAge={childInfo.birthDate}
          imageFile={file}
          result={result}
          onReset={handleReset}
        />
      ) : null}
    </div>
  );
}

export default App;

