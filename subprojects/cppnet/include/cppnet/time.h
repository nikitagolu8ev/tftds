#pragma once

#include <chrono>

#include <sys/time.h>

namespace cppnet {

struct TimeValue {
public:
    template <typename P, typename R>
    TimeValue(std::chrono::duration<P, R> duration) {
        time_value_.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        time_value_.tv_usec =
            std::chrono::duration_cast<std::chrono::microseconds>(duration).count() %
            MicrosecondsInSecond;
    }

private:
    static constexpr uint32_t MicrosecondsInSecond = 1'000'000;

    timeval time_value_;
};

}  // namespace cppnet
