#include "integral/calc.hpp"
#include "util/util.hpp"

namespace math {

double integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset)
{
	const double step = (high - low) / n;
	switch (offset) {
	case Rect_offset::left: break;
	case Rect_offset::middle: low += step * 0.5; break;
	case Rect_offset::right: low += step; break;
	}
	double acc = 0;
	for (unsigned i = 0; i < n; i++)
		acc += f(low + i * step);
	return acc * step;
}

double integrate_trapezoids (double (*f) (double), double low, double high, unsigned n)
{
	const double step = (high - low) / n;

	dvec2 prev = { low, f(low) };
	double acc = 0;
	for (unsigned i = 1; i <= n; i++) {
		const double x = low + step * i;
		const dvec2 cur = { x, f(x) };
		acc += cur.y + prev.y;
		prev = cur;
	}
	return acc * 0.5 * step;
}

double integrate_simpson (double (*f) (double), double low, double high, unsigned n)
{
	const double step = (high - low) / n;
	const double edges = f(low) + f(high);
	double odd = 0, even = 0;
	for (unsigned i = 0; 2*i+1 < n; i++)
		odd += f(low + step * (2 * i + 1));
	for (unsigned i = 1; 2*i < n; i++)
		even += f(low + step * (2 * i));
	return (edges + 4*odd + 2*even) * step / 3;
}


} // namespace math