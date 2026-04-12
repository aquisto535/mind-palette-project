import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { InfoForm } from '../InfoForm';
import { ChildInfo } from '../../types';

// 오늘 기준 YYYY-MM-DD 반환 헬퍼
function dateOffset(years: number, days = 0): string {
  const d = new Date();
  d.setHours(0, 0, 0, 0);
  d.setFullYear(d.getFullYear() - years);
  d.setDate(d.getDate() - days);
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`;
}

describe('InfoForm Component', () => {
  // 1. 렌더링 테스트
  it('renders correctly', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    expect(screen.getByText('아이 정보를 알려주세요')).toBeInTheDocument();
    expect(screen.getByLabelText('이름 (또는 애칭)')).toBeInTheDocument();
    expect(screen.getByLabelText(/생년월일/)).toBeInTheDocument();
    expect(screen.getByText('남자')).toBeInTheDocument();
    expect(screen.getByText('여자')).toBeInTheDocument();
  });

  // 2. 정상 입력 및 제출 테스트
  it('calls onSubmit with correct data when form is submitted', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.change(screen.getByLabelText('이름 (또는 애칭)'), {
      target: { value: '김철수' },
    });
    fireEvent.click(screen.getByText('여자'));
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(7) }, // 만 7세 (정상 범위)
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).toHaveBeenCalledTimes(1);
    expect(mockSubmit).toHaveBeenCalledWith({
      name: '김철수',
      gender: 'female',
      birthDate: dateOffset(7),
    } as ChildInfo);
  });

  // 3. 필수 필드 없을 때
  it('does not call onSubmit if required fields are empty', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).not.toHaveBeenCalled();
  });

  // 4. 이름에 숫자/특수문자 입력 시 에러 메시지
  it('shows error message when name contains numbers or special characters', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.change(screen.getByLabelText('이름 (또는 애칭)'), {
      target: { value: '김1사랑' },
    });
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(7) },
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).not.toHaveBeenCalled();
    expect(screen.getByText(/한글 또는 영문/)).toBeInTheDocument();
  });

  // 5. 이름이 1자인 경우 에러 메시지
  it('shows error message when name is too short', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.change(screen.getByLabelText('이름 (또는 애칭)'), {
      target: { value: '김' },
    });
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(7) },
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).not.toHaveBeenCalled();
    expect(screen.getByText(/2자 이상/)).toBeInTheDocument();
  });

  // 6. 만 4세 (연령 미만) 입력 시 에러 메시지
  it('shows error message when child is younger than 5', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.change(screen.getByLabelText('이름 (또는 애칭)'), {
      target: { value: '김사랑' },
    });
    // 5년 전 날짜보다 하루 뒤 = 아직 만 5세 생일 안 지남 = 만 4세
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(5, -1) },
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).not.toHaveBeenCalled();
    expect(screen.getByText(/만 5세 이상/)).toBeInTheDocument();
  });

  // 7. 만 14세 (연령 초과) 입력 시 에러 메시지
  it('shows error message when child is older than 13', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    fireEvent.change(screen.getByLabelText('이름 (또는 애칭)'), {
      target: { value: '김사랑' },
    });
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(14) }, // 만 14세
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).not.toHaveBeenCalled();
    expect(screen.getByText(/만 13세 이하/)).toBeInTheDocument();
  });

  // 8. 에러 후 올바른 값으로 수정하면 제출 성공
  it('clears error and submits after correcting invalid input', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    const nameInput = screen.getByLabelText('이름 (또는 애칭)');

    // 잘못된 이름 입력 → 에러 발생
    fireEvent.change(nameInput, { target: { value: '김1' } });
    fireEvent.change(screen.getByLabelText(/생년월일/), {
      target: { value: dateOffset(7) },
    });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));
    expect(screen.getByText(/한글 또는 영문/)).toBeInTheDocument();

    // 올바른 이름으로 수정
    fireEvent.change(nameInput, { target: { value: '김사랑' } });
    fireEvent.click(screen.getByRole('button', { name: '다음 단계로' }));

    expect(mockSubmit).toHaveBeenCalledTimes(1);
  });
});
