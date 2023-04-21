#include "nonlin/calc.hpp"
#include "util/util.hpp"
#include <cassert>

namespace math {

Chords_result build_chords (double (*f) (double), double low, double high, double precision)
{
	assert(high > low);
	assert(precision > 0);

	double f_low = f(low);
	double f_high = f(high);

	if ((f_low >= 0) == (f_high >= 0)) return {};
	Chords_result result;

	const auto push_chord = [&] { result.lines.push_back({{ low, f_low }, { high, f_high }}); };

	push_chord();
	while (true) {
		const double mid = low + f_low * (high - low) / (f_low - f_high);
		const double f_mid = f(mid);
		if (std::abs(f_mid) < precision) {
			result.success(mid);
			return result;
		} else if ((f_mid > 0) == (f_high > 0)) {
			high = mid;
			f_high = f_mid;
		} else {
			low = mid;
			f_low = f_mid;
		}
		push_chord();
	}
}

Newton_result newtons_method
(double (*f) (double), double (*dfdx) (double), double initial_guess, double precision)
{
	Newton_result result;
	constexpr size_t max_iterations = 100;

	double x = initial_guess;
	double fx = f(x);

	for (size_t i = 0; i < max_iterations; i++) {
		if (std::abs(fx) < precision) {
			result.success(x);
			return result;
		}

		double slope = dfdx(x);
		if (slope == 0)
			break;
		double nx = x - fx / slope;
		result.lines.push_back({ { x, fx }, { nx, 0 } });
		x = nx;
		fx = f(x);
	}

	return result;
}

Iteration_result fixed_point_iteration
	(double (*f) (double), double lambda, double initial_guess, double precision)
{
	Iteration_result result;
	constexpr size_t max_iterations = 200;
	double x = initial_guess;
	double last_x = initial_guess + 2 * precision;
	for (size_t i = 0; i < max_iterations; i++) {
		if (std::abs(last_x - x) < precision || std::abs(f(x)) < precision) {
			result.success(x);
			return result;
		}
		result.steps.push_back(x);
		last_x = x;
		x += lambda * f(x);
	}
	return result;
}

} // namespace math