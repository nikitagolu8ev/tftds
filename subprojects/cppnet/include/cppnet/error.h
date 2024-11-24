#pragma once

#include <cerrno>
#include <cstring>
#include <system_error>
#include <expected>

using ErrorType = std::error_code;

template <typename T>
using ResultType = std::expected<T, ErrorType>;

ErrorType GetLastError();
void ProcessError(ErrorType error, const std::string& prefix = "");

template <typename T>
void ProcessResultError(const ResultType<T>& result, const std::string& prefix = "") {
    if (!result) {
        if (!prefix.empty()) {
            throw std::runtime_error(prefix + ": " + result.error().message());
        }
        throw std::runtime_error(result.error().message());
    }
}
