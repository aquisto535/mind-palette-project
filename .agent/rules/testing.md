# Mind Palette TDD 테스트 규칙

## TDD 사이클 강제 규칙

### 절대 규칙
- **구현 코드 작성 전 반드시 실패 테스트를 먼저 작성한다**
- 테스트 없이 기능 구현을 시작하지 않는다
- Green 단계에서는 테스트를 통과시키는 최소한의 코드만 작성한다

### TDD 깊이 분류 (L1 → L2 → L3 순서로 작성)

| 레벨 | 대상 | 설명 |
|------|------|------|
| L1 | 데이터 구조·타입 | 올바른 타입, 초기화, null 안전성 |
| L2 | 변환 로직 | 입력 → 출력 변환, 알고리즘 |
| L3 | 경계·제약 조건 | 에러 핸들링, 엣지 케이스 |

### 테스트 네이밍 규칙
```
should_<기대결과>_when_<조건>
예: should_return_grayscale_when_color_image_provided
```

## C++ 테스트 (Google Test)

### 파일 위치
```
preprocess-server/
├── src/         ← 구현 코드
└── tests/       ← 테스트 코드 (src와 동일 구조)
```

### 빌드 및 실행
```bash
cmake --build build/
cd build && ctest --output-on-failure
```

### 필수 패턴
```cpp
// Arrange - Act - Assert 패턴 준수
TEST(ClassName, should_expected_when_condition) {
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(expected, actual);
}
```

## Python 테스트 (pytest)

### 실행
```bash
pytest -v --tb=short
pytest --cov=app tests/  # 커버리지 포함
```

### 필수 패턴
```python
def test_should_expected_when_condition():
    # Arrange
    # Act
    # Assert
    assert actual == expected
```

## TypeScript 테스트 (Jest)

### 실행
```bash
npm test -- --watchAll=false
npm test -- --coverage
```

## 테스트 금지 사항
- ❌ 프로덕션 DB/API에 직접 연결하는 테스트
- ❌ 외부 네트워크 의존 테스트 (mock 필수)
- ❌ 한 테스트에 여러 행동 검증 (단일 책임)
- ❌ 테스트 간 상태 공유
