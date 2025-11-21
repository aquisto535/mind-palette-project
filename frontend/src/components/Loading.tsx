import React from 'react';
import { motion } from 'framer-motion';

export const Loading: React.FC = () => {
  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-white z-50 fixed inset-0">
      <div className="relative">
        <motion.div
          animate={{
            scale: [1, 1.2, 1],
            rotate: [0, 180, 360],
          }}
          transition={{
            duration: 2,
            repeat: Infinity,
            ease: "easeInOut"
          }}
          className="w-24 h-24 border-4 border-primary rounded-2xl"
        />
        <motion.div
          animate={{
            scale: [1.2, 1, 1.2],
            rotate: [0, -180, -360],
          }}
          transition={{
            duration: 2,
            repeat: Infinity,
            ease: "easeInOut"
          }}
          className="absolute inset-0 border-4 border-secondary rounded-full opacity-50"
        />
      </div>
      
      <h2 className="mt-8 text-2xl font-bold text-slate-800">
        AI가 그림을 분석하고 있습니다
      </h2>
      <p className="mt-2 text-slate-500">
        잠시만 기다려주세요...
      </p>
    </section>
  );
};

