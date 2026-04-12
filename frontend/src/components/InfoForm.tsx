import React, { useState } from 'react';
import { motion } from 'framer-motion';
import { ChildInfo } from '../types';
import { validateName, validateBirthDate, getBirthDateRange } from '../utils/validation';

interface InfoFormProps {
  onSubmit: (info: ChildInfo) => void;
}

interface FormErrors {
  name?: string;
  birthDate?: string;
}

const { min: BIRTH_MIN, max: BIRTH_MAX } = getBirthDateRange();

export const InfoForm: React.FC<InfoFormProps> = ({ onSubmit }) => {
  const [info, setInfo] = useState<ChildInfo>({
    name: '',
    gender: 'male',
    birthDate: ''
  });
  const [errors, setErrors] = useState<FormErrors>({});

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();

    const nameResult = validateName(info.name);
    const birthResult = validateBirthDate(info.birthDate);

    if (!nameResult.valid || !birthResult.valid) {
      setErrors({
        name: nameResult.valid ? undefined : nameResult.message,
        birthDate: birthResult.valid ? undefined : birthResult.message,
      });
      return;
    }

    setErrors({});
    onSubmit(info);
  };

  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-white px-4 py-20">
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        whileInView={{ opacity: 1, y: 0 }}
        viewport={{ once: true }}
        className="w-full max-w-md"
      >
        <h2 className="text-3xl font-bold text-center text-slate-900 mb-8">
          아이 정보를 알려주세요
        </h2>

        <form onSubmit={handleSubmit} noValidate className="space-y-6 bg-white p-8 rounded-2xl shadow-xl border border-slate-100">
          <div>
            <label htmlFor="name" className="block text-sm font-medium text-slate-700 mb-2">이름 (또는 애칭)</label>
            <input
              id="name"
              type="text"
              value={info.name}
              onChange={(e) => {
                setInfo({ ...info, name: e.target.value });
                if (errors.name) setErrors((prev) => ({ ...prev, name: undefined }));
              }}
              className={`w-full px-4 py-3 rounded-lg border focus:ring-2 focus:ring-primary focus:border-transparent outline-none transition-all ${
                errors.name ? 'border-red-400' : 'border-slate-300'
              }`}
              placeholder="예: 김사랑"
            />
            {errors.name && (
              <p className="mt-1 text-sm text-red-500">{errors.name}</p>
            )}
          </div>

          <fieldset className="border-0 p-0 m-0">
            <legend className="block text-sm font-medium text-slate-700 mb-2">성별</legend>
            <div className="flex gap-4">
              <button
                type="button"
                onClick={() => setInfo({ ...info, gender: 'male' })}
                className={`flex-1 py-3 rounded-lg border transition-all ${info.gender === 'male'
                  ? 'bg-blue-50 border-blue-500 text-blue-700 font-semibold'
                  : 'border-slate-200 text-slate-500 hover:bg-slate-50'
                  }`}
              >
                남자
              </button>
              <button
                type="button"
                onClick={() => setInfo({ ...info, gender: 'female' })}
                className={`flex-1 py-3 rounded-lg border transition-all ${info.gender === 'female'
                  ? 'bg-pink-50 border-pink-500 text-pink-700 font-semibold'
                  : 'border-slate-200 text-slate-500 hover:bg-slate-50'
                  }`}
              >
                여자
              </button>
            </div>
          </fieldset>

          <div>
            <label htmlFor="birthDate" className="block text-sm font-medium text-slate-700 mb-2">
              생년월일 <span className="text-slate-400 font-normal">(만 5세 ~ 13세)</span>
            </label>
            <input
              id="birthDate"
              type="date"
              value={info.birthDate}
              min={BIRTH_MIN}
              max={BIRTH_MAX}
              onChange={(e) => {
                setInfo({ ...info, birthDate: e.target.value });
                if (errors.birthDate) setErrors((prev) => ({ ...prev, birthDate: undefined }));
              }}
              className={`w-full px-4 py-3 rounded-lg border focus:ring-2 focus:ring-primary focus:border-transparent outline-none transition-all ${
                errors.birthDate ? 'border-red-400' : 'border-slate-300'
              }`}
            />
            {errors.birthDate && (
              <p className="mt-1 text-sm text-red-500">{errors.birthDate}</p>
            )}
          </div>

          <button
            type="submit"
            className="w-full bg-slate-900 text-white py-4 rounded-lg font-bold text-lg hover:bg-slate-800 transition-all mt-8"
          >
            다음 단계로
          </button>
        </form>
      </motion.div>
    </section>
  );
};
