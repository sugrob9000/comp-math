#pragma once

#include <functional>
#include <math.hpp>
#include <span>
#include <vector>

namespace math {
std::function<double(double)> approx_lagrange (std::span<const dvec2>);

struct Finite_differences {
	std::vector<std::vector<double>> diff;
};

Finite_differences newton_calc_finite_differences (std::span<const double>);
std::function<double(double)> approx_newton (double low, double high, const Finite_differences&);

}