export interface ValidationResult {
  valid: boolean;
  message: string;
}

const ok = (): ValidationResult => ({ valid: true, message: '' });
const fail = (message: string): ValidationResult => ({ valid: false, message });

const NAME_REGEX = /^[가-힣a-zA-Z\s]+$/;
const NAME_MIN_LENGTH = 2;
const NAME_MAX_LENGTH = 20;
const MIN_AGE = 5;
const MAX_AGE = 13;
const MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB

export function validateName(name: string): ValidationResult {
  const trimmed = name.trim();

  if (trimmed.length === 0)                return fail('이름을 입력해주세요.');
  if (trimmed.length < NAME_MIN_LENGTH)    return fail(`이름은 ${NAME_MIN_LENGTH}자 이상 입력해주세요.`);
  if (trimmed.length > NAME_MAX_LENGTH)    return fail(`이름은 ${NAME_MAX_LENGTH}자 이하로 입력해주세요.`);
  if (!NAME_REGEX.test(trimmed)) return fail('이름은 한글 또는 영문만 입력 가능합니다.');

  return ok();
}

function calcAge(birthDate: Date, today: Date): number {
  let age = today.getFullYear() - birthDate.getFullYear();
  const monthDiff = today.getMonth() - birthDate.getMonth();
  if (monthDiff < 0 || (monthDiff === 0 && today.getDate() < birthDate.getDate())) {
    age--;
  }
  return age;
}

export function validateBirthDate(birthDate: string): ValidationResult {
  if (!birthDate) return fail('생년월일을 입력해주세요.');

  const birth = new Date(birthDate);
  if (isNaN(birth.getTime())) return fail('올바른 생년월일을 입력해주세요.');

  const today = new Date();
  today.setHours(0, 0, 0, 0);

  if (birth > today) return fail('생년월일이 오늘 날짜보다 클 수 없습니다.');

  const age = calcAge(birth, today);

  if (age < MIN_AGE) return fail(`이 서비스는 만 ${MIN_AGE}세 이상 아동을 대상으로 합니다.`);
  if (age > MAX_AGE) return fail(`이 서비스는 만 ${MAX_AGE}세 이하 아동을 대상으로 합니다.`);

  return ok();
}

export function validateImageFile(file: File): ValidationResult {
  if (file.size > MAX_FILE_SIZE) return fail('파일 크기는 10MB 이하여야 합니다.');
  return ok();
}

/** HTML date input의 min/max 속성에 쓸 "YYYY-MM-DD" 문자열 반환 */
export function getBirthDateRange(): { min: string; max: string } {
  const today = new Date();
  const pad = (n: number) => String(n).padStart(2, '0');
  const fmt = (d: Date) =>
    `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`;

  // 만 5세가 되는 생년월일 = 오늘로부터 정확히 5년 전
  const max = new Date(today.getFullYear() - 5, today.getMonth(), today.getDate());
  // 만 14세가 되기 하루 전 = 오늘로부터 14년 전 + 1일 (이 날 태어난 아이는 아직 13세)
  const min = new Date(today.getFullYear() - 14, today.getMonth(), today.getDate() + 1);

  return { min: fmt(min), max: fmt(max) };
}
