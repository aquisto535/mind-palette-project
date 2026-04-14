export class AnalysisError extends Error {
  status: number;
  actionType: 'retry-upload' | 'retry-guide' | 'retry-later' | 'contact-support';

  constructor(
    status: number,
    message: string,
    actionType: 'retry-upload' | 'retry-guide' | 'retry-later' | 'contact-support'
  ) {
    super(message);
    this.name = 'AnalysisError';
    this.status = status;
    this.actionType = actionType;
  }
}
