import { useState, useEffect } from 'react';
import { Hero } from './components/Hero';
import { InfoForm } from './components/InfoForm';
import { Guide } from './components/Guide';
import { Upload } from './components/Upload';
import { Loading } from './components/Loading';
import { Result } from './components/Result';

type Step = 'hero' | 'form' | 'guide' | 'upload' | 'loading' | 'result';

interface ChildInfo {
  name: string;
  gender: 'male' | 'female';
  birthDate: string;
}

interface AnalysisResult {
  score: number;
  percentile: number;
  interpretation: string;
  date: string;
}

function App() {
  const [step, setStep] = useState<Step>('hero');
  const [childInfo, setChildInfo] = useState<ChildInfo | null>(null);
  const [file, setFile] = useState<File | null>(null);
  const [result, setResult] = useState<AnalysisResult | null>(null);

  // Smooth scroll to top when step changes
  useEffect(() => {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }, [step]);

  const handleInfoSubmit = (info: ChildInfo) => {
    setChildInfo(info);
    setStep('guide');
  };

  const handleUpload = (uploadedFile: File) => {
    setFile(uploadedFile);
    setStep('loading');
    
    // Mock Analysis Process
    setTimeout(() => {
      const mockScore = Math.floor(Math.random() * (95 - 70) + 70); // Random 70-95
      const mockPercentile = Math.floor(Math.random() * (99 - 60) + 60); // Random 60-99
      
      setResult({
        score: mockScore,
        percentile: mockPercentile,
        date: new Date().toLocaleDateString(),
        interpretation: `${childInfo?.name} 어린이는 그림을 통해 풍부한 상상력을 표현하고 있습니다.\n\n선이 굵고 힘이 있는 것으로 보아 자신감이 넘치고 에너지가 많은 성향으로 보입니다. 세부적인 묘사가 ${mockScore > 80 ? '매우 뛰어나며' : '잘 나타나 있으며'}, 이는 관찰력이 우수함을 의미합니다.\n\n전체적인 그림의 크기가 종이에 꽉 차는 것은 외향적이고 적극적인 성격을 나타낼 수 있습니다. 또래 친구들에 비해 인지적 표현력이 상위권에 속하는 것으로 분석됩니다.`
      });
      
      setStep('result');
    }, 3000);
  };

  const handleReset = () => {
    setStep('hero');
    setChildInfo(null);
    setFile(null);
    setResult(null);
  };

  return (
    <div className="min-h-screen bg-white text-slate-900">
      {step === 'hero' && <Hero onStart={() => setStep('form')} />}
      
      {step === 'form' && <InfoForm onSubmit={handleInfoSubmit} />}
      
      {step === 'guide' && <Guide onNext={() => setStep('upload')} />}
      
      {step === 'upload' && <Upload onUpload={handleUpload} />}
      
      {step === 'loading' && <Loading />}
      
      {step === 'result' && childInfo && result && (
        <Result 
          childName={childInfo.name}
          childGender={childInfo.gender}
          childAge={childInfo.birthDate} // Simply passing DOB string for now
          imageFile={file}
          result={result}
          onReset={handleReset}
        />
      )}
    </div>
  );
}

export default App;

