#include "integral/calc.hpp"

namespace math {

Integration_result integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset)
{
	Integration_result result;
	const double step = (high - low) / n;

	switch (offset) {
	case Rect_offset::left: break;
	case Rect_offset::middle: low += step * 0.5; break;
	case Rect_offset::right: low += step; break;
	}

	for (unsigned i = 0; i < n; i++)
		result.value += step * f(low + i * step);

	return result;
}

Integration_result integrate_trapezoids
(double (*f) (double), double low, double high, unsigned n)
{
	Integration_result result;
	const double step = (high - low) / n;

	dvec2 prev = { low, f(low) };

	for (unsigned i = 1; i <= n; i++) {
		dvec2 cur;
		cur.x = low + step * i;
		cur.y = f(cur.x);
		result.value += step * (cur.y + prev.y);
		prev = cur;
	}

	result.value *= 0.5;

	return result;
}

} // namespace math
