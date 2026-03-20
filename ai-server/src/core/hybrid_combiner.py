"""C++ 기하 분석 + AI 추론 결과 결합 모듈."""

from __future__ import annotations


class HybridCombiner:
    """C++ preprocess-server의 기하 분석 결과와 AI 추론 결과를 결합한다.

    combined_confidence 계산:
        confidence = 1.0 - (tremor_score * 0.3), clamp [0, 1]
    tremor_score가 높을수록(선 떨림이 심할수록) 신뢰도 감소.
    """

    _TREMOR_WEIGHT = 0.3

    def combine(self, ai_result: dict, geometric: dict) -> dict:
        """AI 분석 결과와 기하 분석 결과를 결합한다.

        Args:
            ai_result: AI 추론 결과 (items, iq, percentile 등)
            geometric: C++ 기하 분석 결과 (tremor_score, pressure_score 등)

        Returns:
            {
                "ai_analysis": ai_result,
                "geometric_analysis": geometric,
                "combined_confidence": float [0, 1]
            }
        """
        tremor_score = geometric.get("tremor_score", 0.0)
        confidence = 1.0 - (tremor_score * self._TREMOR_WEIGHT)
        confidence = max(0.0, min(1.0, confidence))

        return {
            "ai_analysis": ai_result,
            "geometric_analysis": geometric,
            "combined_confidence": confidence,
        }
