#include "calc.hpp"
#include "util/util.hpp"
#include <cmath>
#include <ranges>
#include "gauss/solve.hpp"
#include <glm/matrix.hpp>

namespace math {
using std::views::transform;

double correlation (std::span<const dvec2> points)
{
	dvec2 avg(0, 0);
	for (const dvec2& p: points)
		avg += p;
	avg /= points.size();

	dvec2 variance(0, 0);
	for (const dvec2& p: points) {
		dvec2 diff = p - avg;
		variance += diff * diff;
	}

	double top = 0;
	for (const dvec2& p: points)
		top += (p.x - avg.x) * (p.y - avg.y);

	double bottom = sqrt(variance.x * variance.y);
	return top / bottom;
}

template <typename Range>
static std::array<double,2> approx_linear_range (Range&& points)
{
	double sx = 0, sxx = 0, sy = 0, sxy = 0;
	for (const dvec2& p: points) {
		sx += p.x;
		sy += p.y;
		sxx += p.x * p.x;
		sxy += p.x * p.y;
	}

	const double n = points.size();
	const double det = sxx * n - sx * sx;
	const double det1 = sxy * n - sx * sy;
	const double det2 = sxx * sy - sx * sxy;
	return { det1 / det, det2 / det };
}

std::array<double,2> approx_linear (std::span<const dvec2> points)
{
	return approx_linear_range(points);
}

std::array<double,2> approx_exponential (std::span<const dvec2> points)
{
	const auto ln_y = [] (const dvec2& v) { return dvec2(v.x, log(v.y)); };
	const auto [a, b] = approx_linear_range(points | transform(ln_y));
	return { exp(b), a };
}

std::array<double,2> approx_logarithmic (std::span<const dvec2> points)
{
	const auto ln_x = [] (const dvec2& v) { return dvec2(log(v.x), v.y); };
	return approx_linear_range(points | transform(ln_x));
}

std::array<double,2> approx_power (std::span<const dvec2> points)
{
	const auto ln_both = [] (const dvec2& v) { return dvec2(log(v.x), log(v.y)); };
	const auto [a, b] = approx_linear_range(points | transform(ln_both));
	return { exp(b), a };
}

std::array<double,3> approx_quadratic (std::span<const dvec2> points)
{
	double sx1 = 0, sx2 = 0, sx3 = 0, sx4 = 0;
	double sx0y = 0, sx1y = 0, sx2y = 0;
	for (const dvec2& p: points) {
		const double x1 = p.x;
		const double x2 = x1 * x1;
		const double x3 = x2 * x1;
		const double x4 = x3 * x1;
		const double y = p.y;
		sx1 += x1;
		sx2 += x2;
		sx3 += x3;
		sx4 += x4;
		sx0y += y;
		sx1y += x1 * y;
		sx2y += x2 * y;
	}
	const double sx0 = points.size();
	using glm::mat3, glm::determinant;
	const double det = determinant(mat3(
			sx0, sx1, sx2,
			sx1, sx2, sx3,
			sx2, sx3, sx4));
	const double a = determinant(mat3(
			sx0y, sx1, sx2,
			sx1y, sx2, sx3,
			sx2y, sx3, sx4)) / det;
	const double b = determinant(mat3(
			sx0, sx0y, sx2,
			sx1, sx1y, sx3,
			sx2, sx2y, sx4)) / det;
	const double c = determinant(mat3(
			sx0, sx1, sx0y,
			sx1, sx2, sx1y,
			sx2, sx3, sx2y)) / det;
	return { a, b, c };
}

std::array<double,4> approx_cubic (std::span<const dvec2> points)
{
	double sx1 = 0, sx2 = 0, sx3 = 0, sx4 = 0, sx5 = 0, sx6;
	double sx0y = 0, sx1y = 0, sx2y = 0, sx3y = 0;
	for (const dvec2& p: points) {
		const double x1 = p.x;
		const double x2 = x1 * x1;
		const double x3 = x2 * x1;
		const double x4 = x3 * x1;
		const double x5 = x4 * x1;
		const double x6 = x5 * x1;
		const double y = p.y;
		sx1 += x1;
		sx2 += x2;
		sx3 += x3;
		sx4 += x4;
		sx5 += x5;
		sx6 += x6;
		sx0y += y;
		sx1y += x1 * y;
		sx2y += x2 * y;
		sx3y += x3 * y;
	}
	const double sx0 = points.size();
	using glm::mat4, glm::determinant;
	const double det = determinant(mat4(
			sx0, sx1, sx2, sx3,
			sx1, sx2, sx3, sx4,
			sx2, sx3, sx4, sx5,
			sx3, sx4, sx5, sx6));
	const double a = determinant(mat4(
			sx0y, sx1, sx2, sx3,
			sx1y, sx2, sx3, sx4,
			sx2y, sx3, sx4, sx5,
			sx3y, sx4, sx5, sx6)) / det;
	const double b = determinant(mat4(
			sx0, sx0y, sx2, sx3,
			sx1, sx1y, sx3, sx4,
			sx2, sx2y, sx4, sx5,
			sx3, sx3y, sx5, sx6)) / det;
	const double c = determinant(mat4(
			sx0, sx1, sx0y, sx3,
			sx1, sx2, sx1y, sx4,
			sx2, sx3, sx2y, sx5,
			sx3, sx4, sx3y, sx6)) / det;
	const double d = determinant(mat4(
			sx0, sx1, sx2, sx0y,
			sx1, sx2, sx3, sx1y,
			sx2, sx3, sx4, sx2y,
			sx3, sx4, sx5, sx3y)) / det;
	return { a, b, c, d };
}

}