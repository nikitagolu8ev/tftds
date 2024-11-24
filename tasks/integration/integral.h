#pragma once

#include <cstdint>

double FuncToIntegrate(double x);
double NumericIntegration(double left, double right, uint32_t points = 100);
