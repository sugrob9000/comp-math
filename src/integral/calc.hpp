#pragma once

#include "math.hpp"

namespace math {
struct Integration_result {
	double value = 0.0;
};

enum class Rect_offset { left, middle, right };

Integration_result integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset);

Integration_result integrate_trapezoids
(double (*f) (double), double low, double high, unsigned n);

} // namespace math