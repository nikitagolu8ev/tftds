#include "integral.h"
#include <cmath>

double FuncToIntegrate(double x) {
    return std::sin(x) * x;
}

double NumericIntegration(double left, double right, uint32_t points) {
    double delta = (right - left) / (points - 1);
    double sum = 0;
    double left_function_value = FuncToIntegrate(left);
    for (uint32_t i = 0; i + 1 < points; ++i) {
        double right_function_value = FuncToIntegrate(left + delta);
        sum += (left_function_value + right_function_value) * delta;

        left_function_value = right_function_value;
        left += delta;
    }
    return sum / 2;
}
