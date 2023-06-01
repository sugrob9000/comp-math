#pragma once

#include "math.hpp"
#include <span>
#include <array>

namespace math {
double correlation (std::span<const dvec2>);
std::array<double,2> approx_linear (std::span<const dvec2>);
std::array<double,2> approx_exponential (std::span<const dvec2>);
std::array<double,2> approx_logarithmic (std::span<const dvec2>);
std::array<double,2> approx_power (std::span<const dvec2>);
std::array<double,3> approx_quadratic (std::span<const dvec2>);
std::array<double,4> approx_cubic (std::span<const dvec2>);
}