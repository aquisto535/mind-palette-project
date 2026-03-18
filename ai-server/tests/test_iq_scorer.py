"""TDD RED: iq_scorer 모듈 테스트.

국가 규준 데이터를 이용한 IQ/백분위 계산 검증:
- IQ = 100 + 15 * ((raw_score - M) / SD)
- IQ → 백분위 조회
- 범위 클램핑 (67~133)
"""

import pytest
from src.core.iq_scorer import calculate_iq, get_percentile, score_to_result


class TestCalculateIQ:
    """IQ 산출 공식 검증."""

    def test_document_example(self):
        """문서 예시: 만10세 남아, 남자상, 원점수 40 → IQ 122."""
        iq = calculate_iq(raw_score=40, age=10, child_gender="male", figure_gender="male")
        assert iq == 122

    def test_average_score_gives_iq_100(self):
        """평균 점수(M) 입력 시 IQ = 100."""
        # 남아, 만 10세, 남자상: M=28.8, SD=7.6
        # round(100 + 15 * ((28.8 - 28.8) / 7.6)) = 100
        iq = calculate_iq(raw_score=28.8, age=10, child_gender="male", figure_gender="male")
        assert iq == 100

    def test_female_child_male_figure(self):
        """여아-남자상 조합 테스트."""
        # 여아, 만 10세, 남자상: M=29.8, SD=8.2
        iq = calculate_iq(raw_score=29.8, age=10, child_gender="female", figure_gender="male")
        assert iq == 100

    def test_female_child_female_figure(self):
        """여아-여자상 조합 테스트."""
        # 여아, 만 10세, 여자상: M=33.2, SD=9.6
        iq = calculate_iq(raw_score=33.2, age=10, child_gender="female", figure_gender="female")
        assert iq == 100

    def test_upper_clamp(self):
        """IQ 상한 133 클램핑."""
        iq = calculate_iq(raw_score=99, age=10, child_gender="male", figure_gender="male")
        assert iq == 133

    def test_lower_clamp(self):
        """IQ 하한 67 클램핑."""
        iq = calculate_iq(raw_score=0, age=10, child_gender="male", figure_gender="male")
        assert iq == 67

    def test_age_3_supported(self):
        """최소 지원 연령 만 3세."""
        iq = calculate_iq(raw_score=8.7, age=3, child_gender="male", figure_gender="male")
        assert iq == 100

    def test_age_13_supported(self):
        """최대 지원 연령 만 13세."""
        iq = calculate_iq(raw_score=31.8, age=13, child_gender="male", figure_gender="male")
        assert iq == 100

    def test_invalid_age_raises(self):
        """지원 범위 밖 연령은 ValueError."""
        with pytest.raises(ValueError):
            calculate_iq(raw_score=20, age=14, child_gender="male", figure_gender="male")

    def test_invalid_gender_raises(self):
        """잘못된 성별 값은 ValueError."""
        with pytest.raises(ValueError):
            calculate_iq(raw_score=20, age=10, child_gender="unknown", figure_gender="male")


class TestGetPercentile:
    """IQ → 백분위 변환 검증."""

    def test_iq_100_is_50th_percentile(self):
        assert get_percentile(100) == 50

    def test_document_example_iq_122(self):
        """IQ 122 → 93 백분위."""
        assert get_percentile(122) == 93

    def test_iq_133_is_99th(self):
        assert get_percentile(133) == 99

    def test_iq_67_is_1st(self):
        assert get_percentile(67) == 1

    def test_iq_above_133_clamps(self):
        """IQ > 133은 99 백분위."""
        assert get_percentile(140) == 99

    def test_iq_below_67_clamps(self):
        """IQ < 67은 1 백분위."""
        assert get_percentile(50) == 1


class TestScoreToResult:
    """end-to-end score_to_result 통합 함수 검증."""

    def test_full_result_structure(self):
        """반환값에 필요한 키가 모두 있다."""
        result = score_to_result(
            raw_score=40, age=10, child_gender="male", figure_gender="male"
        )
        assert "iq" in result
        assert "percentile" in result
        assert "raw_score" in result

    def test_document_example_full(self):
        """만10세 남아, 남자상, 원점수 40 → IQ 122, 백분위 93."""
        result = score_to_result(
            raw_score=40, age=10, child_gender="male", figure_gender="male"
        )
        assert result["iq"] == 122
        assert result["percentile"] == 93
        assert result["raw_score"] == 40
