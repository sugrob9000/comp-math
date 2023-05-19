#pragma once

#include "math.hpp"
#include <optional>
#include <vector>

namespace math {

struct Newton_system_result {
	std::vector<dvec2> guesses;
	std::optional<dvec2> root;
};

Newton_system_result newtons_method_system
(double (*f) (double), double (*dfdx) (double),
 double (*g) (double), double (*dgdx) (double),
 dvec2 initial_guess, double precision);
}