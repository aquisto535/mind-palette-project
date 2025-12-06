import { useState, useEffect } from 'react';
import { Hero } from './components/Hero';
import { InfoForm } from './components/InfoForm';
import { Guide } from './components/Guide';
import { Upload } from './components/Upload';
import { Loading } from './components/Loading';
import { Result } from './components/Result';
import { ChildInfo, AnalysisResult } from './types';
import { uploadImage } from './api/uploadApi';

type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result';

function App() 
{
  const [step, setStep] = useState<Step>('hero'); // 현재 진행 중인 단계
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null); // 자녀 정보
  const [file, setFile] = useState<File | null>(null); // 업로드된 파일
  const [result, setResult] = useState<AnalysisResult | null>(null); // 분석 결과

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
      console.error('Analysis failed:', error);
      alert('분석 중 오류가 발생했습니다. 잠시 후 다시 시도해주세요.');
      setStep('upload');
      setFile(null);
    }
  };

  const handleReset = () => {
    setStep('hero'); // 첫 단계로 이동
    setChildInfo(null); // 자녀 정보 초기화
    setFile(null); // 업로드된 파일 초기화
    setResult(null); // 분석 결과 초기화
  };

  return (
    <div className="min-h-screen bg-white text-slate-900">
      {step === 'hero' && <Hero onStart={() => setStep('form')} />} // 히어로 섹션
      
      {step === 'form' && <InfoForm onSubmit={handleInfoSubmit} />} // 정보 입력 섹션
      
      {step === 'guide' && <Guide onNext={() => setStep('upload')} />} // 가이드 섹션
      
      {step === 'upload' && <Upload onUpload={handleUpload} />} // 업로드 섹션
      
      {step === 'loading' && <Loading />} // 로딩 섹션
      
      {step === 'result' && childInfo && result && (
        <Result 
          childName={childInfo.name} // 자녀 이름
          childGender={childInfo.gender} // 자녀 성별
          childAge={childInfo.birthDate} // Simply passing DOB string for now
          imageFile={file} // 업로드된 파일
          result={result}
          onReset={handleReset} // 초기화 핸들러
        />
      )}
    </div>
  );
}

export default App;

