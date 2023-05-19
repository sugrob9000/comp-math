#pragma once

#include "math.hpp"
#include <vector>
#include <optional>

namespace math {

struct Result {
	double root = 0;
	double value_at_root = 0;
	bool has_root = false;
	void success (double r, double v) {
		root = r;
		value_at_root = v;
		has_root = true;
	}
};

struct Line_based_result: Result {
	struct Line { dvec2 a, b; };
	std::vector<Line> lines;
};

struct Chords_result: Line_based_result {};
Chords_result build_chords (double (*f) (double), double low, double high, double precision);


struct Newton_result: Line_based_result {};
Newton_result newtons_method
(double (*f) (double), double (*dfdx) (double), double initial_guess, double precision);


struct Iteration_result: Result {
	std::vector<double> steps;
};
Iteration_result fixed_point_iteration
	(double (*f) (double), double lambda, double initial_guess, double precision);

} // namespace math