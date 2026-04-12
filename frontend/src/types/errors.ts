export class AnalysisError extends Error {
  status: number;
  actionType: 'retry-upload' | 'retry-guide' | 'retry-later';

  constructor(
    status: number,
    message: string,
    actionType: 'retry-upload' | 'retry-guide' | 'retry-later'
  ) {
    super(message);
    this.name = 'AnalysisError';
    this.status = status;
    this.actionType = actionType;
  }
}
