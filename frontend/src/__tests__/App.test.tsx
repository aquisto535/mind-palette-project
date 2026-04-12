import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import { vi, describe, it, expect, beforeEach } from 'vitest';
import App from '../App';
import * as uploadApi from '../api/uploadApi';

// Mocking the uploadApi
vi.mock('../api/uploadApi', () => ({
  uploadImage: vi.fn(),
}));

// Mocking globalThis.scrollTo
globalThis.scrollTo = vi.fn() as typeof globalThis.scrollTo;

describe('App Integration Test', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  const navigateToUpload = () => {
    render(<App />);

    // 1. Hero -> Form
    fireEvent.click(screen.getByText(/분석 시작하기/i));

    // 2. Form -> Guide
    // InfoForm placeholders and labels
    fireEvent.change(screen.getByPlaceholderText(/예: 김사랑/i), {
      target: { value: '홍길동' },
    });
    fireEvent.click(screen.getByText(/남자/i));
    fireEvent.change(screen.getByLabelText(/생년월일/i), {
      target: { value: '2020-01-01' },
    });

    fireEvent.click(screen.getByText(/다음 단계로/i));

    // 3. Guide -> Upload
    // Guide button text
    fireEvent.click(screen.getByText(/준비되었어요/i));
  };

  it('should call uploadImage API with correct data when file is uploaded', async () => {
    navigateToUpload();

    // Verify Upload screen
    expect(screen.getByText(/그림을 올려주세요/i)).toBeInTheDocument();

    const file = new File(['dummy content'], 'test.png', { type: 'image/png' });
    const fileInput = document.getElementById('file-input') as HTMLInputElement;

    // Mock successful response
    const mockResult = {
      score: 85,
      percentile: 90,
      date: '2026-01-11',
      interpretation: '테스트 해석 결과입니다.',
    };
    (uploadApi.uploadImage as any).mockResolvedValue(mockResult);

    // Trigger upload
    fireEvent.change(fileInput, { target: { files: [file] } });

    // Wait for Result screen directly
    // The Upload component has a 1000ms delay, and then App processes the API call.
    await waitFor(() => {
      // Check for a specific text in the Result component
      expect(screen.getByText(/홍길동 어린이의 분석 결과/i)).toBeInTheDocument();
    }, { timeout: 4000 });

    // Verify API call
    expect(uploadApi.uploadImage).toHaveBeenCalledTimes(1);
    expect(uploadApi.uploadImage).toHaveBeenCalledWith(
      expect.any(File),
      expect.objectContaining({
        name: '홍길동',
        gender: 'male',
        birthDate: '2020-01-01',
      })
    );
  });

  it('should handle API errors correctly', async () => {
    navigateToUpload();

    const file = new File(['dummy content'], 'test.png', { type: 'image/png' });
    const fileInput = document.getElementById('file-input') as HTMLInputElement;

    // Mock error response
    const mockError = new Error('Upload failed');
    (uploadApi.uploadImage as any).mockRejectedValue(mockError);

    // Trigger upload
    fireEvent.change(fileInput, { target: { files: [file] } });

    // Wait for error message
    await waitFor(() => {
      expect(screen.getByText(/예기치 못한 오류가 발생했습니다/i)).toBeInTheDocument();
    }, { timeout: 4000 });

    // Click retry button
    fireEvent.click(screen.getByText(/다시 업로드/i));

    // Should be back to upload screen
    await waitFor(() => {
      expect(screen.getByText(/그림을 올려주세요/i)).toBeInTheDocument();
    });
  });
});
