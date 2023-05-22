#include "integral/calc.hpp"

namespace math {

double integrate_rect
(double (*f) (double), double low, double high, unsigned n, Rect_offset offset)
{
	double result = 0;
	const double step = (high - low) / n;

	switch (offset) {
	case Rect_offset::left: break;
	case Rect_offset::middle: low += step * 0.5; break;
	case Rect_offset::right: low += step; break;
	}

	for (unsigned i = 0; i < n; i++)
		result += step * f(low + i * step);

	return result;
}

double integrate_trapezoids (double (*f) (double), double low, double high, unsigned n)
{
	double result = 0;
	const double step = (high - low) / n;

	dvec2 prev = { low, f(low) };

	for (unsigned i = 1; i <= n; i++) {
		dvec2 cur;
		cur.x = low + step * i;
		cur.y = f(cur.x);
		result += step * (cur.y + prev.y);
		prev = cur;
	}

	result *= 0.5;

	return result;
}

double integrate_simpson (double (*f) (double), double low, double high, unsigned n)
{
	if (n % 2 != 0)
		n++;
	const double step = (high - low) / n;
	double acc = f(low) + f(high);
	for (unsigned i = 0; i < n/2; i++) {
		acc += 4 * f(low + step * (2 * i + 1));
		acc += 2 * f(low + step * (2 * i + 2));
	}
	return acc * step / 3;
}

} // namespace math