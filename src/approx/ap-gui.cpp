#include <approx/ap-gui.hpp>
#include <approx/calc.hpp>
#include <gui.hpp>
#include <util/util.hpp>

using namespace ImGui;
using namespace ImScoped;

void Approx::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	{ // Render graph
		Graph_draw_context draw(graph);
		draw.background();
		const auto f = last_output.get_function();

		{ // Some functions only accept a positive argument
			using enum Method;
			double min = -std::numeric_limits<double>::infinity();
			if (last_output.method == power || last_output.method == logarithmic)
				min = 0;
			draw.function_plot(0xFF'996633, f, min);
		}

		for (const auto& p: input.view()) {
			draw.dot(p, 0xFF'0000FF);
			draw.line(p, dvec2(p.x, f(p.x)), 0xFF'1111EE, 2.0);
		}
	}
	last_output.result_window();
}

std::function<double(double)> Approx::Output::get_function () const
{
	switch (method) {
		using enum Method;
	case linear: return [a=a, b=b] (double x) { return a*x + b; };
	case exponential: return [a=a, b=b] (double x) { return a * exp(b * x); };
	case logarithmic: return [a=a, b=b] (double x) { return a * log(x) + b; };
	case power: return [a=a, b=b] (double x) { return a * pow(x, b); };
	case polynomial_2: return [a=a, b=b, c=c] (double x) { return a + b*x + c*x*x; };
	case polynomial_3: return [a=a, b=b, c=c, d=d] (double x) { return a + b*x + c*x*x + d*x*x*x; };
	case find_best: break;
	}
	unreachable();
}

void Approx::settings_widget ()
{
	bool dirty = false;

	if (auto node = ImScoped::TreeNode("Метод")) {
		using enum Method;
		struct Method_spec {
			const char* name;
			Method method;
		};
		constexpr static Method_spec methods[] = {
			{ "С наименьшим отклонением", find_best },
			{ "Линейная функция", linear },
			{ "Полином степени 2", polynomial_2 },
			{ "Полином степени 3", polynomial_3 },
			{ "Экспонента", exponential },
			{ "Логарифм", logarithmic },
			{ "Степенная функция", power },
		};
		for (auto [name, m]: methods) {
			if (RadioButton(name, method == m)) {
				method = m;
				dirty = true;
			}
		}
	}

	if (auto node = ImScoped::TreeNode("Данные")) dirty |= input.widget();
	if (auto node = ImScoped::TreeNode("Вид")) graph.settings_widget();

	if (dirty) update_calculation();
}

void Approx::Output::result_window () const
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		switch (method) {
		case Method::linear:
			TextFmt("{:.3}x {:+.3}\nКоэффициент корреляции: {:.3}", a, b, correlation);
			break;
		case Method::exponential: TextFmt("{:.3} exp({:.3} x)", a,  b); break;
		case Method::logarithmic: TextFmt("{:.3} ln(x) {:+.3}", a, b); break;
		case Method::power: TextFmt("{:.3} x^{:.3}", a, b); break;
		case Method::polynomial_2: TextFmt("{:.3} {:+.3}x {:+.3}x²", a, b, c); break;
		case Method::polynomial_3: TextFmt("{:.3} {:+.3}x {:+.3}x² {:+.3}x³", a, b, c, d); break;
		case Method::find_best: unreachable();
		}
		TextFmt("Среднеквадратичное отклонение: {:.3}", deviation);
	}
}

template <size_t N>
void Approx::Output::assign_coefs (std::array<double, N> coefs) requires(N<=4)
{
	constexpr double Output::* off[4] = { &Output::a, &Output::b, &Output::c, &Output::d };
	for (size_t i = 0; i < N; i++)
		this->*off[i] = coefs[i];
}

auto Approx::calculate (Method method, std::span<const dvec2> points) -> Output
{
	Output out;
	out.method = method;

	switch (method) {
		using enum Method;
		using namespace math;
	case linear:
		out.assign_coefs(approx_linear(points));
		out.correlation = math::correlation(points);
		break;
	case polynomial_2: out.assign_coefs(approx_quadratic(points)); break;
	case polynomial_3: out.assign_coefs(approx_cubic(points)); break;
	case exponential: out.assign_coefs(approx_exponential(points)); break;
	case logarithmic: out.assign_coefs(approx_logarithmic(points)); break;
	case power: out.assign_coefs(approx_power(points)); break;
	case find_best: FATAL("find_best is not valid for Output");
	}

	const auto f = out.get_function();
	double deviation = 0;
	for (const dvec2& p: points) {
		const double diff = f(p.x) - p.y;
		deviation += diff * diff;
	}
	deviation /= points.size();
	deviation = sqrt(deviation);
	out.deviation = deviation;

	return out;
}

void Approx::update_calculation ()
{
	using enum Method;
	const std::span<const dvec2> points = input.view();
	if (method == Method::find_best) {
		constexpr static Method other_methods[] = {
			linear, polynomial_2, polynomial_3, exponential, logarithmic, power,
		};
		last_output = calculate(other_methods[0], points);
		for (size_t i = 1; i < std::size(other_methods); i++) {
			Output out = calculate(other_methods[i], points);
			if (out.deviation < last_output.deviation)
				last_output = out;
		}
	} else {
		last_output = calculate(method, points);
	}
}