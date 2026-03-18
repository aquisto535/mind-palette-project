# Mind Palette C++ 성능 최적화 규칙

## 핵심 원칙

### 1. 측정 먼저 (Measure First)
- 최적화 전 항상 프로파일링으로 병목 확인
- `/benchmark-reporter` 스킬로 결과 문서화
- "추측이 아닌 데이터로" 최적화 방향 결정

### 2. 이미지 처리 최적화 (OpenCV)

#### 메모리 효율
```cpp
// ❌ 매 프레임 새 Mat 생성
cv::Mat result = src.clone();

// ✅ 재사용 가능한 버퍼 유지
cv::Mat buffer;
src.copyTo(buffer);
```

#### 연산 최적화
```cpp
// ✅ 인플레이스 연산 우선 사용
cv::GaussianBlur(src, src, cv::Size(5, 5), 0);

// ✅ 연속 메모리 접근
if (src.isContinuous()) {
    // 배열 포인터 직접 접근
}
```

### 3. 멀티스레딩 (Thread Pool)

#### 현재 구조
- `preprocess-server/src/infra/thread_pool.cpp` — ThreadPool 구현
- 이미지 처리는 ThreadPool을 통해 비동기 처리

#### 데드락 방지
```cpp
// ✅ lock_guard 사용 (RAII)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // critical section
} // 자동 해제

// ❌ 수동 unlock — 예외 시 데드락 위험
mutex_.lock();
// ... 예외 발생 가능
mutex_.unlock();
```

### 4. Smart Pointer 규칙

```cpp
// ✅ 소유권 명확화
std::unique_ptr<Processor> processor = std::make_unique<Processor>();

// ✅ 공유 소유권
std::shared_ptr<Config> config = std::make_shared<Config>();

// ❌ raw new/delete 금지
Processor* p = new Processor();
delete p;
```

### 5. 성능 체크리스트 (코드 리뷰 시)

- [ ] 불필요한 복사 없는가? (`const &` 또는 `std::move` 사용)
- [ ] OpenCV Mat이 루프 안에서 생성되지 않는가?
- [ ] 뮤텍스 잠금 범위가 최소화되었는가?
- [ ] 스마트 포인터를 사용하는가?
- [ ] `reserve()`로 벡터 재할당이 방지되었는가?

### 6. 벤치마크 기준 (Preprocess Server)

| 작업 | 목표 응답시간 |
|------|-------------|
| Grayscale 변환 | < 10ms |
| Gaussian Blur | < 20ms |
| Canny Edge Detection | < 30ms |
| 전체 전처리 파이프라인 | < 100ms |

벤치마크 결과가 기준을 초과하면 `/benchmark-reporter` 스킬로 리포트 생성.
