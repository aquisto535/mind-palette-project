"""TDD RED: item_mapping 모듈 테스트.

60문항 ↔ 4개 Head 매핑 검증:
- Head A (머리/얼굴): 19개
- Head B (몸통/비례): 14개
- Head C (사지/말단): 16개
- Head D (의복/질적): 11개
- 남녀 척도 차이 처리
"""

import pytest
from src.core.item_mapping import (
    HEAD_A_ITEMS,
    HEAD_B_ITEMS,
    HEAD_C_ITEMS,
    HEAD_D_ITEMS,
    ALL_ITEMS,
    FEMALE_EXCEPTION_ITEMS,
    items_to_head_labels,
)


class TestHeadMappingStructure:
    """Head 매핑 구조 검증."""

    def test_head_a_size(self):
        assert len(HEAD_A_ITEMS) == 19

    def test_head_b_size(self):
        assert len(HEAD_B_ITEMS) == 14

    def test_head_c_size(self):
        assert len(HEAD_C_ITEMS) == 16

    def test_head_d_size(self):
        assert len(HEAD_D_ITEMS) == 11

    def test_total_items_count(self):
        """전체 문항 수는 60개."""
        assert len(ALL_ITEMS) == 60

    def test_no_duplicate_items(self):
        """각 문항은 정확히 하나의 Head에만 속한다."""
        all_items = HEAD_A_ITEMS + HEAD_B_ITEMS + HEAD_C_ITEMS + HEAD_D_ITEMS
        assert len(all_items) == len(set(all_items)), "중복 문항 존재"

    def test_all_items_range(self):
        """모든 문항 번호는 1~60 범위."""
        for item in ALL_ITEMS:
            assert 1 <= item <= 60, f"범위 밖 문항: {item}"

    def test_all_60_items_covered(self):
        """1번~60번 문항이 빠짐없이 모두 포함."""
        assert set(ALL_ITEMS) == set(range(1, 61))


class TestItemsToHeadLabels:
    """items_to_head_labels 변환 함수 검증."""

    def test_male_scale_all_zero(self):
        """남자 척도, 모든 문항 0 → 모든 head 레이블이 0."""
        items = {str(i): 0 for i in range(1, 61)}
        result = items_to_head_labels(items, scale="male")
        assert result["head_a"] == [0] * 19
        assert result["head_b"] == [0] * 14
        assert result["head_c"] == [0] * 16
        assert result["head_d"] == [0] * 11

    def test_male_scale_all_one(self):
        """남자 척도, 모든 문항 1 → 모든 head 레이블이 1."""
        items = {str(i): 1 for i in range(1, 61)}
        result = items_to_head_labels(items, scale="male")
        assert result["head_a"] == [1] * 19
        assert result["head_b"] == [1] * 14
        assert result["head_c"] == [1] * 16
        assert result["head_d"] == [1] * 11

    def test_head_keys_present(self):
        """반환값에 4개 head 키가 모두 있어야 한다."""
        items = {str(i): 0 for i in range(1, 61)}
        result = items_to_head_labels(items, scale="male")
        assert set(result.keys()) == {"head_a", "head_b", "head_c", "head_d"}

    def test_head_label_lengths(self):
        """각 head 레이블 길이가 정의된 크기와 일치."""
        items = {str(i): 0 for i in range(1, 61)}
        result = items_to_head_labels(items, scale="male")
        assert len(result["head_a"]) == 19
        assert len(result["head_b"]) == 14
        assert len(result["head_c"]) == 16
        assert len(result["head_d"]) == 11

    def test_label_values_are_binary(self):
        """레이블 값은 0 또는 1만 허용."""
        items = {str(i): (i % 2) for i in range(1, 61)}
        result = items_to_head_labels(items, scale="male")
        for head, labels in result.items():
            for v in labels:
                assert v in (0, 1), f"{head}에 이진값 아닌 값: {v}"

    def test_specific_item_mapping(self):
        """문항 1번은 Head A의 첫 번째 레이블에 매핑."""
        items = {str(i): 0 for i in range(1, 61)}
        items["1"] = 1
        result = items_to_head_labels(items, scale="male")
        # 문항 1은 Head A 소속
        assert result["head_a"][HEAD_A_ITEMS.index(1)] == 1

    def test_female_scale_exception_items_exist(self):
        """여자 척도 예외 문항이 정의되어 있다."""
        assert len(FEMALE_EXCEPTION_ITEMS) == 6

    def test_female_scale_accepted(self):
        """scale='female' 호출 시 오류 없이 동작."""
        items = {str(i): 0 for i in range(1, 61)}
        result = items_to_head_labels(items, scale="female")
        assert len(result["head_a"]) == 19
