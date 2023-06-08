#include <approx/calc.hpp>
#include <cmath>
#include <glm/matrix.hpp>
#include <ranges>
#include <util/util.hpp>

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

template <size_t N, typename Range>
static std::array<double,N+1> approx_polynomial (Range&& points)
{
	constexpr size_t M = 2*N+1;
	double sxi[M] = {};
	sxi[0] = std::max(points.size(), N+1);
	glm::vec<N+1,double> sxiy = {};

	for (const dvec2& p: points) {
		double xi[M-1];
		xi[0] = p.x;
		for (size_t i = 1; i < std::size(xi); i++) xi[i] = p.x * xi[i-1];
		for (size_t i = 0; i < std::size(xi); i++) sxi[i+1] += xi[i];
		sxiy[0] += p.y;
		for (size_t i = 1; i < N+1; i++) sxiy[i] += p.y * xi[i-1];
	}

	using Mat = glm::mat<N+1,N+1,double>;
	using glm::determinant;
	Mat main;
	for (size_t i = 0; i < N+1; i++) {
		for (size_t j = 0; j < N+1; j++)
			main[i][j] = sxi[i+j];
	}

	double main_det = determinant(main);
	if (main_det == 0)
		return {};

	std::array<double,N+1> result;
	for (size_t i = 0; i < N+1; i++) {
		Mat mat = main;
		mat[i] = sxiy;
		result[i] = determinant(mat) / main_det;
	}
	return result;
}

std::array<double,2> approx_linear (std::span<const dvec2> points)
{
	auto [a, b] = approx_polynomial<1>(points);
	return { b, a };
}

std::array<double,2> approx_exponential (std::span<const dvec2> points)
{
	const auto ln_y = [] (const dvec2& v) { return dvec2(v.x, log(v.y)); };
	const auto [a, b] = approx_polynomial<1>(points | transform(ln_y));
	return { exp(a), b };
}

std::array<double,2> approx_logarithmic (std::span<const dvec2> points)
{
	const auto ln_x = [] (const dvec2& v) { return dvec2(log(v.x), v.y); };
	const auto [a, b] = approx_polynomial<1>(points | transform(ln_x));
	return { b, a };
}

std::array<double,2> approx_power (std::span<const dvec2> points)
{
	const auto ln_both = [] (const dvec2& v) { return dvec2(log(v.x), log(v.y)); };
	const auto [a, b] = approx_polynomial<1>(points | transform(ln_both));
	return { exp(a), b };
}

std::array<double,3> approx_quadratic (std::span<const dvec2> points)
{
	return approx_polynomial<2>(points);
}

std::array<double,4> approx_cubic (std::span<const dvec2> points)
{
	return approx_polynomial<3>(points);
}

}