#include <algorithm>
#include <interp/calc.hpp>
#include <util/util.hpp>

namespace math {

std::function<double(double)> approx_lagrange (std::span<const dvec2> points)
{
	return [p = points] (double x) -> double {
		double result = 0;
		const size_t n = p.size();
		for (size_t i = 0; i < n; i++) {
			double term = p[i].y;
			for (size_t j = 0; j < n; j++) {
				if (i == j) continue;
				term *= (x - p[j].x) / (p[i].x - p[j].x);
			}
			result += term;
		}
		return result;
	};
}

Finite_differences newton_calc_finite_differences (std::span<const double> points)
{
	Finite_differences result;

	result.diff.resize(points.size());
	for (size_t n = points.size(), i = 0; n >= 1; n--, i++) result.diff[i].resize(n);

	std::copy_n(points.begin(), points.size(), result.diff[0].begin());

	for (size_t n = points.size()-1, i = 1; n >= 1; n--, i++) {
		auto& dest = result.diff[i];
		const auto& src = result.diff[i-1];
		for (size_t j = 0; j < n; j++)
			dest[j] = src[j+1]-src[j];
	}

	return result;
}

std::function<double(double)> approx_newton
(double low, double high, const Finite_differences& diff)
{
	return [low=low, high=high, diff=diff.diff] (double x) -> double {
		if (diff.size() < 2) return 0;
		const double step = (high - low) / (diff.size()-1);
		const double left = x-low;
		const double right = high-x;
		const size_t n = diff.size();
		if (left < 0 || right < 0) return 0.0;

		const size_t iif = floor(left / step);
		enum Direction { forward, backward };
		Direction direction = (iif > n/2 ? backward : forward);

		unsigned long long factorial = 1;
		const auto update_factorial = [&] (int i) {
			constexpr auto max = std::numeric_limits<unsigned long long>::max();
			if (factorial >= max / i)
				factorial = max;
			else
				factorial *= i;
		};

		double result = 0;
		double t_acc = 1;

		if (direction == forward) {
			const int ii = floor(left / step);
			const size_t i = ii;

			const double x0 = low + step * i;
			const double t = (x - x0) / step;

			for (size_t j = 0; j < n && i < diff[j].size(); j++) {
				const double add = diff[j][i] * t_acc / factorial;
				result += add;
				t_acc *= t-j;
				update_factorial(j+1);
			}
		} else {
			const int ii = ceil(left / step);
			const size_t i = ii;

			const double xn = low + step * i;
			const double t = (x - xn) / step;

			for (size_t j = 0; j < n && (i-j) < diff[j].size(); j++) {
				result += diff[j][i-j] * t_acc / factorial;
				t_acc *= t+j;
				update_factorial(j+1);
			}
		}
		return result;
	};
}

} // namespace math