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
	assert(diff.diff.size() >= 2);
	const double step = (high - low) / (diff.diff.size()-1);

	return [low=low, high=high, diff=diff.diff, step=step] (double x) -> double {
		const double left = x-low;
		const double right = high-x;
		const size_t n = diff.size();
		if (left < 0 || right < 0) return 0.0;

		const size_t iif = floor(left / step);
		enum Direction { forward, backward };
		Direction direction = (iif > n/2 ? backward : forward);

		if (direction == forward) {
			const int ii = floor(left / step);
			const size_t i = ii;
			assert(ii >= 0 && i < n-1);

			const double x0 = low + step * i;
			const double t = (x - x0) / step;

			double result = 0;
			double t_acc = 1;
			unsigned long long factorial = 1;
			for (size_t j = 0; j < n && i < diff[j].size(); j++) {
				result += diff[j][i] * t_acc / factorial;
				t_acc *= t-j;
				factorial *= j+1;
			}
			return result;
		} else {
			const int ii = ceil(left / step);
			const size_t i = ii;
			assert(ii > 0 && i <= n-1);

			const double xn = low + step * i;
			const double t = (x - xn) / step;
			assert(xn >= x);

			double result = 0;
			double t_acc = 1;
			unsigned long long factorial = 1;
			for (size_t j = 0; j < n && (i-j) < diff[j].size(); j++) {
				result += diff[j][i-j] * t_acc / factorial;
				t_acc *= t+j;
				factorial *= j+1;
			}
			return result;
		}
	};
}

} // namespace math