import React from 'react';
import { motion } from 'framer-motion';
import { Pencil, Sun, Smile } from 'lucide-react';

interface GuideProps {
  onNext: () => void;
}

export const Guide: React.FC<GuideProps> = ({ onNext }) => {
  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-slate-50 px-4 py-20">
      <motion.div
        initial={{ opacity: 0 }}
        whileInView={{ opacity: 1 }}
        viewport={{ once: true }}
        className="max-w-3xl w-full"
      >
        <div className="text-center mb-12">
          <h2 className="text-3xl font-bold text-slate-900 mb-4">그림 그리기 가이드</h2>
          <p className="text-slate-600">정확한 분석을 위해 아래 내용을 꼭 읽어주세요.</p>
        </div>

        <div className="grid md:grid-cols-3 gap-6 mb-12">
          {[
            { icon: <Pencil className="w-8 h-8 text-blue-500" />, title: "전신 그리기", desc: "머리부터 발끝까지 사람의 전체 모습을 그려주세요." },
            { icon: <Sun className="w-8 h-8 text-orange-500" />, title: "자유로운 표현", desc: "지우개를 사용해도 괜찮아요. 아이가 편안하게 그리도록 해주세요." },
            { icon: <Smile className="w-8 h-8 text-green-500" />, title: "간섭 금지", desc: "부모님의 조언 없이 아이 스스로 그리게 해주세요." },
          ].map((item, idx) => (
            <motion.div
              key={idx}
              initial={{ opacity: 0, y: 20 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ delay: idx * 0.1 }}
              className="bg-white p-6 rounded-2xl shadow-sm border border-slate-100 flex flex-col items-center text-center"
            >
              <div className="mb-4 p-3 bg-slate-50 rounded-full">{item.icon}</div>
              <h3 className="font-bold text-lg mb-2">{item.title}</h3>
              <p className="text-slate-500 text-sm word-keep">{item.desc}</p>
            </motion.div>
          ))}
        </div>

        <div className="text-center">
          <button
            onClick={onNext}
            className="bg-primary hover:bg-indigo-700 text-white text-lg font-semibold py-4 px-12 rounded-full shadow-lg transition-all"
          >
            준비되었어요
          </button>
        </div>
      </motion.div>
    </section>
  );
};

