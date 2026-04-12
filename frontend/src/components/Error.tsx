import React from 'react';
import { motion } from 'framer-motion';
import { AlertCircle, RefreshCw, BookOpen } from 'lucide-react';
import { AnalysisError } from '../types/errors';

interface ErrorProps {
  error: AnalysisError;
  onRetry: () => void;
  onGuide: () => void;
}

export const ErrorScreen: React.FC<ErrorProps> = ({ error, onRetry, onGuide }) => {
  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-slate-50 px-4 py-20">
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        className="max-w-md w-full bg-white rounded-2xl shadow-sm border border-slate-100 p-10 text-center"
      >
        <div className="flex justify-center mb-6">
          <div className="p-4 bg-red-50 rounded-full">
            <AlertCircle className="w-10 h-10 text-red-400" />
          </div>
        </div>

        <h2 className="text-2xl font-bold text-slate-900 mb-3">업로드 오류</h2>
        <p className="text-slate-600 mb-8 leading-relaxed word-keep">{error.message}</p>

        <div className="flex flex-col gap-3">
          {error.actionType === 'retry-guide' && (
            <button
              onClick={onGuide}
              className="flex items-center justify-center gap-2 bg-primary hover:bg-indigo-700 text-white font-semibold py-3 px-8 rounded-full shadow transition-all"
            >
              <BookOpen className="w-4 h-4" />
              올바른 그림 안내 보기
            </button>
          )}
          <button
            onClick={onRetry}
            className="flex items-center justify-center gap-2 bg-white hover:bg-slate-50 text-slate-700 font-semibold py-3 px-8 rounded-full border border-slate-200 transition-all"
          >
            <RefreshCw className="w-4 h-4" />
            다시 업로드
          </button>
        </div>
      </motion.div>
    </section>
  );
};
