import React from 'react';
import { motion } from 'framer-motion';
import { ChevronDown } from 'lucide-react';

interface HeroProps {
  onStart: () => void;
}

export const Hero: React.FC<HeroProps> = ({ onStart }) => {
  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-gradient-to-b from-blue-50 to-white text-center px-4">
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.8 }}
        className="max-w-2xl"
      >
        <h1 className="text-4xl md:text-6xl font-bold text-slate-900 mb-6 leading-tight">
          우리 아이의 마음,<br />
          <span className="text-primary">그림</span>으로 이해해보세요
        </h1>
        <p className="text-lg md:text-xl text-slate-600 mb-10 leading-relaxed">
          아이가 그린 사람 그림 한 장으로<br />
          인지 발달 수준과 심리적 특성을 분석해드립니다.
        </p>
        
        <button
          onClick={onStart}
          className="bg-primary hover:bg-indigo-700 text-white text-lg font-semibold py-4 px-10 rounded-full shadow-lg transition-all transform hover:scale-105"
        >
          무료로 분석 시작하기
        </button>
      </motion.div>

      <motion.div
        animate={{ y: [0, 10, 0] }}
        transition={{ repeat: Infinity, duration: 2 }}
        className="absolute bottom-10 text-slate-400"
      >
        <ChevronDown size={32} />
      </motion.div>
    </section>
  );
};

