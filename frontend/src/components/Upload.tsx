import React, { useCallback, useState } from 'react';
import { motion } from 'framer-motion';
import { Upload as UploadIcon, X } from 'lucide-react';
import { validateImageFile } from '../utils/validation';

interface UploadProps {
  onUpload: (file: File) => void;
}

export const Upload: React.FC<UploadProps> = ({ onUpload }) => {
  const [preview, setPreview] = useState<string | null>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [fileError, setFileError] = useState<string | null>(null);

  const handleFile = useCallback((file: File) => {
    if (!file.type.startsWith('image/')) {
      setFileError('이미지 파일만 업로드할 수 있습니다.');
      return;
    }

    const validation = validateImageFile(file);
    if (!validation.valid) {
      setFileError(validation.message);
      return;
    }

    setFileError(null);

    const reader = new FileReader();
    reader.onload = (e) => {
      setPreview(e.target?.result as string);
    };
    reader.readAsDataURL(file);

    // Delay the actual upload callback slightly for UX
    setTimeout(() => onUpload(file), 1000);
  }, [onUpload]);

  const onDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(false);
    if (e.dataTransfer.files?.[0]) {
      handleFile(e.dataTransfer.files[0]);
    }
  }, [handleFile]);

  const onChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files?.[0]) {
      handleFile(e.target.files[0]);
    }
  }, [handleFile]);

  return (
    <section className="min-h-screen flex flex-col items-center justify-center bg-white px-4 py-20">
      <motion.div
        initial={{ opacity: 0, scale: 0.95 }}
        whileInView={{ opacity: 1, scale: 1 }}
        className="w-full max-w-xl"
      >
        <h2 className="text-3xl font-bold text-center text-slate-900 mb-8">
          그림을 올려주세요
        </h2>

        <label
          htmlFor={preview ? undefined : 'file-input'}
          className={`relative aspect-square rounded-3xl border-4 border-dashed flex flex-col items-center justify-center transition-all cursor-pointer overflow-hidden
            ${isDragging ? 'border-primary bg-blue-50' : 'border-slate-200 bg-slate-50 hover:border-primary/50 hover:bg-slate-100'}
          `}
          onDragOver={(e) => { e.preventDefault(); setIsDragging(true); }}
          onDragLeave={() => setIsDragging(false)}
          onDrop={onDrop}
        >
          {preview ? (
            <div className="relative w-full h-full group">
              <img src={preview} alt="Upload preview" className="w-full h-full object-contain p-4" />
              <div className="absolute inset-0 bg-black/50 opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center">
                <button
                  onClick={(e) => {
                    e.stopPropagation();
                    setPreview(null);
                    setFileError(null);
                  }}
                  className="p-3 bg-white rounded-full text-red-500 hover:bg-red-50"
                >
                  <X size={24} />
                </button>
              </div>
            </div>
          ) : (
            <>
              <div className="p-6 bg-white rounded-full shadow-sm mb-4">
                <UploadIcon size={48} className="text-primary" />
              </div>
              <p className="text-xl font-medium text-slate-700 mb-2">여기를 클릭하거나 그림을 끌어오세요</p>
              <p className="text-slate-400">JPG, PNG 등 이미지 파일 · 최대 10MB</p>
            </>
          )}

          <input
            id="file-input"
            type="file"
            className="hidden"
            accept="image/*"
            onChange={onChange}
          />
        </label>

        {fileError && (
          <p className="mt-3 text-sm text-red-500 text-center">{fileError}</p>
        )}
      </motion.div>
    </section>
  );
};
