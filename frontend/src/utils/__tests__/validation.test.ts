import { describe, it, expect } from 'vitest';
import { validateName, validateBirthDate, validateImageFile } from '../validation';

// 오늘 기준 YYYY-MM-DD 반환 헬퍼
function dateOffset(years: number, days = 0): string {
  const d = new Date();
  d.setHours(0, 0, 0, 0);
  d.setFullYear(d.getFullYear() - years);
  d.setDate(d.getDate() - days);
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`;
}

// 오늘 기준 미래 날짜 반환 헬퍼
function futureDate(days = 1): string {
  const d = new Date();
  d.setHours(0, 0, 0, 0);
  d.setDate(d.getDate() + days);
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`;
}

// ──────────────────────────────────────────
// validateName
// ──────────────────────────────────────────
describe('validateName', () => {
  it('정상: 한글 이름', () => {
    expect(validateName('김사랑').valid).toBe(true);
  });

  it('정상: 영문 이름', () => {
    expect(validateName('Sarang').valid).toBe(true);
  });

  it('정상: 한글+공백 (이름 사이 공백 허용)', () => {
    expect(validateName('김 사랑').valid).toBe(true);
  });

  it('정상: 최소 2자', () => {
    expect(validateName('김사').valid).toBe(true);
  });

  it('정상: 최대 20자', () => {
    expect(validateName('가나다라마바사아자차카타파하가나다라마바').valid).toBe(true); // 20자
  });

  it('실패: 빈 문자열', () => {
    const result = validateName('');
    expect(result.valid).toBe(false);
    expect(result.message).toBeTruthy();
  });

  it('실패: 공백만 입력', () => {
    const result = validateName('   ');
    expect(result.valid).toBe(false);
  });

  it('실패: 1자 (너무 짧음)', () => {
    const result = validateName('김');
    expect(result.valid).toBe(false);
    expect(result.message).toContain('2자 이상');
  });

  it('실패: 21자 (너무 긺)', () => {
    const result = validateName('가나다라마바사아자차카타파하가나다라마바사'); // 21자
    expect(result.valid).toBe(false);
    expect(result.message).toContain('20자 이하');
  });

  it('실패: 숫자 포함', () => {
    const result = validateName('김1사랑');
    expect(result.valid).toBe(false);
    expect(result.message).toContain('한글 또는 영문');
  });

  it('실패: 특수문자 포함', () => {
    const result = validateName('김사랑!');
    expect(result.valid).toBe(false);
    expect(result.message).toContain('한글 또는 영문');
  });
});

// ──────────────────────────────────────────
// validateBirthDate
// ──────────────────────────────────────────
describe('validateBirthDate', () => {
  it('정상: 만 8세 (범위 내)', () => {
    expect(validateBirthDate(dateOffset(8)).valid).toBe(true);
  });

  it('정상: 만 5세 정확히 (최소 나이)', () => {
    expect(validateBirthDate(dateOffset(5)).valid).toBe(true);
  });

  it('정상: 만 13세 정확히 (최대 나이)', () => {
    expect(validateBirthDate(dateOffset(13)).valid).toBe(true);
  });

  it('정상: 만 13세 생일 하루 전 (아직 13세)', () => {
    // 생일이 내일인 아이 → 아직 만 13세에서 1일 부족 → 만 12세
    // 생일이 364일 후인 아이 → 만 12세 → valid
    expect(validateBirthDate(dateOffset(13, -1)).valid).toBe(true); // 12세 364일 → 12세
  });

  it('실패: 빈 문자열', () => {
    const result = validateBirthDate('');
    expect(result.valid).toBe(false);
    expect(result.message).toBeTruthy();
  });

  it('실패: 미래 날짜', () => {
    const result = validateBirthDate(futureDate(1));
    expect(result.valid).toBe(false);
    expect(result.message).toContain('오늘 날짜');
  });

  it('실패: 만 4세 (5세 미만)', () => {
    // 5년 전 날짜보다 하루 뒤 = 아직 생일 안 지남 → 만 4세
    const result = validateBirthDate(dateOffset(5, -1));
    expect(result.valid).toBe(false);
    expect(result.message).toContain('만 5세 이상');
  });

  it('실패: 만 14세 (13세 초과)', () => {
    const result = validateBirthDate(dateOffset(14));
    expect(result.valid).toBe(false);
    expect(result.message).toContain('만 13세 이하');
  });
});

// ──────────────────────────────────────────
// validateImageFile
// ──────────────────────────────────────────
describe('validateImageFile', () => {
  function makeFile(sizeBytes: number, name = 'test.jpg', type = 'image/jpeg'): File {
    const buffer = new ArrayBuffer(sizeBytes);
    return new File([buffer], name, { type });
  }

  it('정상: 5MB JPEG', () => {
    const file = makeFile(5 * 1024 * 1024);
    expect(validateImageFile(file).valid).toBe(true);
  });

  it('정상: 정확히 10MB', () => {
    const file = makeFile(10 * 1024 * 1024);
    expect(validateImageFile(file).valid).toBe(true);
  });

  it('실패: 10MB 초과 (15MB)', () => {
    const file = makeFile(15 * 1024 * 1024);
    const result = validateImageFile(file);
    expect(result.valid).toBe(false);
    expect(result.message).toContain('10MB');
  });
});
