#pragma once

#include <stdexcept>
#include <string>

/**
 * @brief ValidationException - 이미지 유효성 검사 실패 예외
 *
 * FilterPipeline에서 발생시키고, Crow 서버가 캐치하여 HTTP 422를 반환한다.
 * ADR-033: 비연필(컬러) 이미지 조기 필터링 전략
 */
class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& message)
        : std::runtime_error(message) {}
};
