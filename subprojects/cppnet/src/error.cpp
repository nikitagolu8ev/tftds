#include "include/cppnet/error.h"

ErrorType GetLastError() {
    return std::make_error_code(std::errc(errno));
}

void ProcessError(ErrorType error, const std::string& prefix) {
    if (error) {
        if (!prefix.empty()) {
            throw std::runtime_error(prefix + ": " + error.message());
        }
        throw std::runtime_error(error.message());
    }
}
