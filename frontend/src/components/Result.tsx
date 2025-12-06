import React, { useRef } from 'react';
import { motion } from 'framer-motion';
import { Download, Share2, RefreshCw } from 'lucide-react';
import html2canvas from 'html2canvas';
import jsPDF from 'jspdf';
import { AnalysisResult } from '../types';

interface ResultProps {
  childName: string;
  childGender: string;
  childAge: string;
  imageFile: File | null;
  result: AnalysisResult;
  onReset: () => void;
}

export const Result: React.FC<ResultProps> = ({
  childName,
  childGender,
  childAge,
  imageFile,
  result,
  onReset,
}) => {
  const reportRef = useRef<HTMLDivElement>(null);

  const handleDownloadPDF = async () => {
    if (!reportRef.current) return;
    
    const canvas = await html2canvas(reportRef.current, {
      scale: 2,
      logging: false,
      useCORS: true
    });
    
    const imgData = canvas.toDataURL('image/png');
    const pdf = new jsPDF({
      orientation: 'portrait',
      unit: 'mm',
      format: 'a4'
    });
    
    const imgWidth = 210;
    const imgHeight = (canvas.height * imgWidth) / canvas.width;
    
    pdf.addImage(imgData, 'PNG', 0, 0, imgWidth, imgHeight);
    pdf.save(`${childName}_그림분석결과.pdf`);
  };

  // Tree Visualization Component
  const TreeVisual = ({ level }: { level: number }) => (
    <div className="relative w-full aspect-[3/4] flex items-end justify-center">
      {/* Background Tree Outline */}
      <svg viewBox="0 0 200 300" className="w-full h-full absolute opacity-20">
        <path
          d="M100,280 L100,250 C100,250 120,230 120,200 C120,170 100,150 100,150 C100,150 140,130 140,90 C140,50 100,20 100,20 C100,20 60,50 60,90 C60,130 100,150 100,150 C100,150 80,170 80,200 C80,230 100,250 100,250 L100,280 Z"
          fill="currentColor"
          className="text-green-800"
        />
      </svg>
      
      {/* Filled Tree (Animated) */}
      <div className="absolute inset-0 overflow-hidden flex items-end justify-center">
        <motion.div 
          initial={{ height: "0%" }}
          animate={{ height: `${level}%` }}
          transition={{ duration: 1.5, ease: "easeOut" }}
          className="w-full flex justify-center relative"
        >
           <svg viewBox="0 0 200 300" className="w-full h-full absolute bottom-0" preserveAspectRatio="xMidYBottom slice">
            <path
              d="M100,280 L100,250 C100,250 120,230 120,200 C120,170 100,150 100,150 C100,150 140,130 140,90 C140,50 100,20 100,20 C100,20 60,50 60,90 C60,130 100,150 100,150 C100,150 80,170 80,200 C80,230 100,250 100,250 L100,280 Z"
              fill="url(#treeGradient)"
            />
            <defs>
              <linearGradient id="treeGradient" x1="0" x2="0" y1="1" y2="0">
                <stop offset="0%" stopColor="#15803d" />
                <stop offset="100%" stopColor="#4ade80" />
              </linearGradient>
            </defs>
          </svg>
        </motion.div>
      </div>
      
      {/* Percentage Label */}
      <div className="absolute bottom-4 right-4 bg-white/90 px-3 py-1 rounded-full text-sm font-bold shadow-sm border">
        발달지수 {level}점
      </div>
    </div>
  );

  // Normal Distribution Component
  const DistributionGraph = ({ percentile }: { percentile: number }) => {
    // Simple bell curve path approximation
    const pathD = "M10,140 C50,140 80,10 150,10 C220,10 250,140 290,140";
    // Calculate approximate position based on percentile (simplified mapping)
    const xPos = 30 + (percentile / 100) * 240;
    const yPos = 140 - (Math.exp(-Math.pow((percentile - 50) / 20, 2)) * 120);

    return (
      <div className="w-full aspect-[2/1] relative bg-slate-50 rounded-xl p-4 border border-slate-100">
        <svg viewBox="0 0 300 150" className="w-full h-full">
          {/* Grid lines */}
          <line x1="150" y1="10" x2="150" y2="140" stroke="#e2e8f0" strokeDasharray="4 4" />
          
          {/* Curve */}
          <path d={pathD} fill="none" stroke="#94a3b8" strokeWidth="3" />
          
          {/* Area under curve to the left of percentile could be filled here if calculated */}
          
          {/* Indicator Point */}
          <motion.circle 
            initial={{ cx: 10, cy: 140, opacity: 0 }}
            animate={{ cx: xPos, cy: yPos, opacity: 1 }}
            transition={{ duration: 1.5, delay: 0.5 }}
            r="6" 
            fill="#4F46E5" 
            stroke="white" 
            strokeWidth="3"
          />
          
          {/* Text Label */}
          <motion.g
             initial={{ opacity: 0, y: 10 }}
             animate={{ opacity: 1, y: 0 }}
             transition={{ delay: 2 }}
          >
             <text x={xPos} y={yPos - 15} textAnchor="middle" fontSize="12" fill="#1e293b" fontWeight="bold">
               상위 {100 - percentile}%
             </text>
          </motion.g>
          
          {/* X Axis Labels */}
          <text x="30" y="148" fontSize="10" fill="#94a3b8">낮음</text>
          <text x="150" y="148" textAnchor="middle" fontSize="10" fill="#94a3b8">평균</text>
          <text x="270" y="148" textAnchor="end" fontSize="10" fill="#94a3b8">높음</text>
        </svg>
      </div>
    );
  };

  return (
    <section className="min-h-screen py-20 bg-slate-50 px-4">
      <div className="max-w-4xl mx-auto space-y-8">
        
        {/* Report Container (Target for PDF) */}
        <div ref={reportRef} className="bg-white p-8 md:p-12 rounded-3xl shadow-xl space-y-8">
          {/* Header */}
          <div className="text-center border-b border-slate-100 pb-8">
            <div className="inline-block px-4 py-1 bg-primary/10 text-primary rounded-full text-sm font-semibold mb-4">
              AI 그림 심리 분석 리포트
            </div>
            <h2 className="text-3xl font-bold text-slate-900">
              {childName} 어린이의 분석 결과
            </h2>
            <p className="text-slate-500 mt-2">
              분석일: {result.date} | {childGender === 'male' ? '남아' : '여아'} | {childAge}
            </p>
          </div>

          <div className="grid md:grid-cols-2 gap-12">
            {/* Left Column: Visuals */}
            <div className="space-y-8">
              {/* Original Image */}
              <div className="bg-slate-50 p-4 rounded-2xl border border-slate-100">
                <h3 className="font-bold text-slate-700 mb-3">분석한 그림</h3>
                <div className="aspect-square rounded-xl overflow-hidden bg-white shadow-inner flex items-center justify-center">
                  {imageFile && (
                    <img 
                      src={URL.createObjectURL(imageFile)} 
                      alt="Original" 
                      className="max-w-full max-h-full object-contain" 
                    />
                  )}
                </div>
              </div>

              {/* Tree Visualization */}
              <div className="bg-slate-50 p-4 rounded-2xl border border-slate-100">
                <h3 className="font-bold text-slate-700 mb-3">인지 발달 나무</h3>
                <TreeVisual level={result.score} />
                <p className="text-xs text-slate-400 mt-2 text-center">나무가 풍성할수록 인지 표현력이 높습니다</p>
              </div>
            </div>

            {/* Right Column: Data & Text */}
            <div className="space-y-8">
              {/* Distribution */}
              <div>
                <h3 className="font-bold text-slate-700 mb-3">또래 대비 발달 수준</h3>
                <DistributionGraph percentile={result.percentile} />
              </div>

              {/* Text Interpretation */}
              <div className="prose prose-slate">
                <h3 className="font-bold text-slate-800 text-xl mb-4">종합 해석</h3>
                <p className="text-slate-600 leading-relaxed whitespace-pre-line">
                  {result.interpretation}
                </p>
                
                <div className="bg-blue-50 p-6 rounded-xl mt-6">
                  <h4 className="font-bold text-blue-900 mb-2">부모님을 위한 팁</h4>
                  <ul className="list-disc list-inside text-blue-800 space-y-2 text-sm">
                    <li>아이의 그림에 대해 구체적으로 칭찬해주세요.</li>
                    <li>"이 사람은 무엇을 하고 있니?" 라고 물어봐주세요.</li>
                    <li>결과보다는 그리는 과정을 즐기도록 격려해주세요.</li>
                  </ul>
                </div>
              </div>
            </div>
          </div>
          
          <div className="text-center pt-8 border-t border-slate-100 text-slate-400 text-sm">
            * 본 결과는 AI 분석에 기반한 추정치이며 전문적인 의학적 진단을 대체할 수 없습니다.
          </div>
        </div>

        {/* Action Buttons */}
        <div className="flex gap-4 justify-center">
          <button
            onClick={handleDownloadPDF}
            className="flex items-center gap-2 bg-slate-900 text-white px-8 py-4 rounded-full font-bold hover:bg-slate-800 transition-all shadow-lg"
          >
            <Download size={20} />
            PDF 저장하기
          </button>
          <button
             onClick={() => navigator.clipboard.writeText(window.location.href)}
             className="flex items-center gap-2 bg-white text-slate-700 border border-slate-200 px-8 py-4 rounded-full font-bold hover:bg-slate-50 transition-all shadow-sm"
          >
            <Share2 size={20} />
            공유하기
          </button>
           <button
            onClick={onReset}
            className="flex items-center gap-2 bg-white text-slate-700 border border-slate-200 px-8 py-4 rounded-full font-bold hover:bg-slate-50 transition-all shadow-sm"
          >
            <RefreshCw size={20} />
            다시하기
          </button>
        </div>

      </div>
    </section>
  );
};

