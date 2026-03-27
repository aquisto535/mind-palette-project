"""TDD: HybridCombiner 모듈 테스트."""

import pytest

from src.core.hybrid_combiner import HybridCombiner


class TestHybridCombiner:
    """HybridCombiner.combine() 테스트."""

    def test_result_contains_all_keys(self):
        """결과에 ai_analysis, geometric_analysis, combined_confidence 키가 포함된다."""
        combiner = HybridCombiner()
        ai_result = {"iq": 100, "percentile": 50}
        geometric = {"tremor_score": 0.1, "pressure_score": 0.6}
        result = combiner.combine(ai_result, geometric)
        assert "ai_analysis" in result
        assert "geometric_analysis" in result
        assert "combined_confidence" in result

    def test_ai_analysis_matches_input(self):
        """ai_analysis 필드는 입력 ai_result와 동일하다."""
        combiner = HybridCombiner()
        ai_result = {"iq": 90, "percentile": 30}
        result = combiner.combine(ai_result, {"tremor_score": 0.0})
        assert result["ai_analysis"] == ai_result

    def test_geometric_analysis_matches_input(self):
        """geometric_analysis 필드는 입력 geometric과 동일하다."""
        combiner = HybridCombiner()
        geometric = {"tremor_score": 0.2, "pressure_score": 0.5}
        result = combiner.combine({"iq": 100}, geometric)
        assert result["geometric_analysis"] == geometric

    def test_empty_geometric_returns_full_confidence(self):
        """geometric이 빈 dict이면 combined_confidence는 1.0."""
        combiner = HybridCombiner()
        result = combiner.combine({"iq": 100}, {})
        assert result["combined_confidence"] == pytest.approx(1.0)

    def test_high_tremor_reduces_confidence(self):
        """tremor_score가 높으면 combined_confidence가 낮아진다."""
        combiner = HybridCombiner()
        low_tremor = combiner.combine({"iq": 100}, {"tremor_score": 0.0})
        high_tremor = combiner.combine({"iq": 100}, {"tremor_score": 1.0})
        assert high_tremor["combined_confidence"] < low_tremor["combined_confidence"]

    def test_confidence_clamped_between_0_and_1(self):
        """combined_confidence는 항상 [0, 1] 범위 내에 있다."""
        combiner = HybridCombiner()
        result = combiner.combine({"iq": 100}, {"tremor_score": 999.0})
        assert 0.0 <= result["combined_confidence"] <= 1.0

    def test_tremor_score_reduces_by_formula(self):
        """combined_confidence = 1.0 - (tremor_score * 0.3), clamp [0, 1]."""
        combiner = HybridCombiner()
        result = combiner.combine({"iq": 100}, {"tremor_score": 0.5})
        assert result["combined_confidence"] == pytest.approx(1.0 - 0.5 * 0.3)

    def test_no_tremor_key_returns_full_confidence(self):
        """geometric에 tremor_score 키가 없으면 confidence는 1.0."""
        combiner = HybridCombiner()
        result = combiner.combine({"iq": 100}, {"pressure_score": 0.8})
        assert result["combined_confidence"] == pytest.approx(1.0)
