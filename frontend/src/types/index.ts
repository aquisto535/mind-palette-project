export interface ChildInfo {
  name: string;
  gender: 'male' | 'female';
  birthDate: string;
}

export interface AnalysisResult {
  score: number;
  percentile: number;
  interpretation: string;
  date: string;
}

