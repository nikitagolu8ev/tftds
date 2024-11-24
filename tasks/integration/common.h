#pragma once

#include <cstdint>

namespace defaults {

constexpr uint16_t BroadcastPort = 8080;
constexpr uint16_t ConnectionPort = 4242;

}

struct Task {
    uint32_t identifier;
    double left;
    double right;
};

struct TaskResponse {
    uint32_t identifier;
    double result;
};
