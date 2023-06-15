#include <diffeq/calc.hpp>
#include <vector>
#include <util/util.hpp>

namespace math {

void diffeq_euler_modified
(double (*f) (dvec2), double low, double y0, double step, std::span<dvec2> result)
{
	auto prev = result.begin();
	auto cur = prev;
	assert(cur != result.end());
	*prev = { low, y0 };
	++cur;

	for (int i = 1; cur != result.end(); ++cur, i++) {
		const auto& p = *prev;
		const double fp = f(p);
		const double x = low + i * step;
		const double y = p.y + 0.5 * step * (fp + f({ x, p.y + step * fp }));
		*cur = { x, y };
		prev = cur;
	}
}

void diffeq_runge_kutta4
(double (*f) (dvec2), double low, double y0, double step, std::span<dvec2> result)
{
	auto prev = result.begin();
	auto cur = prev;
	assert(cur != result.end());
	*prev = { low, y0 };
	++cur;

	for (int i = 1; cur != result.end(); ++cur, i++) {
		const auto& p = *prev;
		const double x = low + i * step;
		const double px = p.x;
		const double py = p.y;

		const double k1 = step * f(p);
		const double k2 = step * f({ px + 0.5*step, py + 0.5*k1 });
		const double k3 = step * f({ px + 0.5*step, py + 0.5*k2 });
		const double k4 = step * f({ px + step, py + k3 });

		const double y = py + (k1 + 2 * (k2 + k3) + k4) / 6.0;

		*cur = { x, y };
		prev = cur;
	}
}

void diffeq_milne
(double (*f) (dvec2), double low, double y0, double step, double eps, std::span<dvec2> result)
{
	if (result.size() <= 5)
		return diffeq_runge_kutta4(f, low, y0, step, result);

	diffeq_runge_kutta4(f, low, y0, step, result.subspan(0, 4));

	std::vector<double> fs(result.size());
	for (int i = 0; i < 4; i++)
		fs[i] = f(result[i]);

	for (size_t i = 4; i < result.size(); i++) {
		const double x = low + i * step;
		double y_pred = result[i-4].y + 4 * step * (2*fs[i-3] - fs[i-2] + 2*fs[i-1]) / 3;
		for (int c = 0; c < 10; c++) {
			const double f_pred = f({ x, y_pred });
			const double y_corr = result[i-2].y + step * (fs[i-2] + 4*fs[i-1] + f_pred) / 3;
			if (std::abs(y_pred - y_corr) < eps)
				break;
			y_pred = y_corr;
		}
		result[i] = { x, y_pred };
		fs[i] = f(result[i]);
	}
}

}