import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { InfoForm } from '../InfoForm';
import { ChildInfo } from '../../types';

describe('InfoForm Component', () => {
  // 1. 렌더링 테스트
  it('renders correctly', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    expect(screen.getByText('아이 정보를 알려주세요')).toBeInTheDocument();
    expect(screen.getByLabelText('이름 (또는 애칭)')).toBeInTheDocument();
    expect(screen.getByLabelText('생년월일')).toBeInTheDocument();
    expect(screen.getByText('남자')).toBeInTheDocument();
    expect(screen.getByText('여자')).toBeInTheDocument();
  });

  // 2. 입력 및 제출 테스트
  it('calls onSubmit with correct data when form is submitted', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    // 이름 입력
    const nameInput = screen.getByLabelText('이름 (또는 애칭)');
    fireEvent.change(nameInput, { target: { value: '김철수' } });

    // 성별 선택 (기본값이 남자이므로 여자로 변경 테스트)
    const femaleButton = screen.getByText('여자');
    fireEvent.click(femaleButton);

    // 생년월일 입력
    const dateInput = screen.getByLabelText('생년월일');
    fireEvent.change(dateInput, { target: { value: '2018-05-05' } });

    // 제출 버튼 클릭
    const submitButton = screen.getByRole('button', { name: '다음 단계로' });
    fireEvent.click(submitButton);

    // 검증: onSubmit이 1번 호출되었는지, 그리고 전달된 데이터가 맞는지
    expect(mockSubmit).toHaveBeenCalledTimes(1);
    expect(mockSubmit).toHaveBeenCalledWith({
      name: '김철수',
      gender: 'female',
      birthDate: '2018-05-05',
    } as ChildInfo);
  });

  // 3. 유효성 검사 테스트 (선택 사항)
  it('does not call onSubmit if required fields are empty', () => {
    const mockSubmit = vi.fn();
    render(<InfoForm onSubmit={mockSubmit} />);

    const submitButton = screen.getByRole('button', { name: '다음 단계로' });
    fireEvent.click(submitButton);

    // 필수 입력값이 없으므로 호출되지 않아야 함
    // (HTML5 required 속성으로 인해 브라우저 레벨에서 막히지만, 
    //  jsdom 환경에서는 form submit 이벤트가 발생하지 않거나 
    //  핸들러 내부의 if 문에 의해 막히는 것을 확인)
    expect(mockSubmit).not.toHaveBeenCalled();
  });
});

