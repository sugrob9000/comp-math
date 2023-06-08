#pragma once

#include <math.hpp>
#include <vector>

namespace math {
enum class Rect_offset { left, middle, right };
double integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset);
double integrate_trapezoids (double (*f) (double), double low, double high, unsigned n);
double integrate_simpson (double (*) (double), double low, double high, unsigned n);
} // namespace math