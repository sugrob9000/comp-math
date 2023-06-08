#pragma once

#include <math.hpp>
#include <optional>
#include <vector>

namespace math {
struct Newton_system_result {
	std::vector<dvec2> guesses;
	std::optional<dvec2> root;
};

Newton_system_result newtons_method_system
(double (*f) (dvec2), double (*dfdx) (dvec2), double (*dfdy) (dvec2),
 double (*g) (dvec2), double (*dgdx) (dvec2), double (*dgdy) (dvec2),
 dvec2 initial_guess, double precision);
} // namespace math