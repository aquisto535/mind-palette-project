# AI 아동 그림 인지 분석 웹사이트

## 프로젝트 실행 방법

1. 의존성 설치:
```bash
npm install
```

2. 개발 서버 실행:
```bash
npm run dev
```

## 주요 기능

- **단계별 진행**: 소개 -> 정보 입력 -> 가이드 -> 업로드 -> 분석 -> 결과
- **그림 업로드**: 드래그 앤 드롭 지원
- **결과 시각화**:
  - 나무 성장 애니메이션 (인지 발달 수준)
  - 정규분포 그래프 (또래 대비 위치)
- **결과 저장**: PDF 다운로드 기능 (html2canvas + jsPDF)

## 기술 스택

- React + TypeScript + Vite
- Tailwind CSS (스타일링)
- Framer Motion (애니메이션)
- Lucide React (아이콘)

