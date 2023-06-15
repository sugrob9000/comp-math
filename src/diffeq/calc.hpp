#pragma once

#include <math.hpp>
#include <span>

namespace math {

void diffeq_euler_modified
(double (*f) (dvec2), double low, double y0, double step, std::span<dvec2> result);

void diffeq_runge_kutta4
(double (*f) (dvec2), double low, double y0, double step, std::span<dvec2> result);

void diffeq_milne
(double (*f) (dvec2), double low, double y0, double step, double eps, std::span<dvec2> result);

}