#pragma once

#include "math.hpp"
#include <vector>

namespace math {
enum class Rect_offset { left, middle, right };
double integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset);

double integrate_trapezoids (double (*f) (double), double low, double high, unsigned n);
double integrate_simpson (double (*) (double), double low, double high, unsigned n);

struct Integrate_method {
	enum class Method: char { rectangle = 0, trapezoid = 1, simpson = 2 };
	enum class Rect_offset: char { left, middle, right };
};


struct Integrate_result {
	unsigned subdivisions;
	bool has_result;
	double result;
	double precision;
	struct Discontinuity {
		double x;
		bool different_sign;
	};
	std::vector<Discontinuity> discontinuities;
};
Integrate_result integrate_to_precision
(Integrate_method method, double (*f) (double),
 double low, double high, unsigned n, double precision);

} // namespace math