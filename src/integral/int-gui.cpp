#include "gui.hpp"
#include "imhelper.hpp"
#include "integral/calc.hpp"
#include "integral/int-gui.hpp"
#include "util/util.hpp"
#include <numbers>

using namespace ImGui;
using namespace ImScoped;

namespace {
struct Function_spec {
	const char* name;
	double (*compute) (double);
	double (*antiderivative) (double);
};
constexpr Function_spec functions[] = {
	{
		"x² - 0.9",
		[] (double x) { return x*x-0.9; },
		[] (double x) { return x*x*x / 3.0 - 0.9 * x; },
	},
	{
		"sin(x) exp(x)",
		[] (double x) { return sin(x) * exp(x); },
		[] (double x) { return 0.5 * exp(x) * (sin(x) - cos(x)); }
	},
	{
		"exp(-x²) - 0.5",
		[] (double x) { return exp(-x*x) - 0.5; },
		[] (double x) { return 0.5 * sqrt(std::numbers::pi) * erf(x); }
	},
	{
		"1/x",
		[] (double x) { return 1.0 / x; },
		[] (double x) { return log(x); }
	}
};

} // anon namespace

void Integration::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	result_window();
	result_visualization();
}

void Integration::result_visualization () const
{
	Graph_draw_context draw(graph);
	draw.background();
	draw.function_plot(0xFF'55BB77, functions[active_function_id].compute);

	constexpr uint32_t limit_color = 0xFF'BB5555;
	constexpr uint32_t outline_color = 0x88'CC0044;
	constexpr uint32_t fill_color = 0x22'C01054;
	constexpr uint32_t dot_color = fill_color | 0xFF'000000;

	constexpr float limit_thickness = 2.0;
	const auto& f = functions[active_function_id].compute;
	const double step = (high - low) / result.subdivisions;

	draw.vert_line(low, limit_color, limit_thickness);
	draw.vert_line(high, limit_color, limit_thickness);

	// TODO: output the required information from special `math::` functions
	// instead of replicating most of the computation here
	switch (active_method) {
	case Method::rect: {
		double low_sample = low;
		switch (rect_offset) {
		case math::Rect_offset::left: break;
		case math::Rect_offset::middle: low_sample += step * 0.5; break;
		case math::Rect_offset::right: low_sample += step; break;
		}
		for (unsigned i = 0; i < result.subdivisions; i++) {
			const double x_low = low + step * i;
			const double x_high = low + step * (i+1);
			const double x_sample = low_sample + step * i;
			const double y = f(x_sample);
			draw.rect({ x_low, 0 }, { x_high, y }, outline_color, fill_color);
			draw.dot({ x_sample, y }, dot_color);
		}
		break;
	}

	case Method::trapezoid: {
		dvec2 prev = { low, f(low) };
		draw.dot(prev, dot_color);
		for (unsigned i = 1; i <= result.subdivisions; i++) {
			const double x = low + step * i;
			const dvec2 cur = { x, f(x) };
			draw.trapezoid(cur.x, prev.x, 0, cur.y, prev.y, outline_color, fill_color);
			draw.dot(cur, dot_color);
			prev = cur;
		}
		break;
	}

	case Method::simpson: {
		for (unsigned i = 0; i <= result.subdivisions; i++) {
			const double x = low + step * i;
			draw.dot({ x, f(x) }, dot_color);
		}
		break;
	}
	}
}

void Integration::settings_widget ()
{
	constexpr float drag_speed = 0.03;
	bool dirty = false;

	if (auto node = ImScoped::TreeNode("Метод")) {
		constexpr static const char* rect_method_names[3] = {
			"Метод левых прямоугольников",
			"Метод средних прямоугольников",
			"Метод правых прямоугольников"
		};
		for (int i = 0; i < 3; i++) {
			const auto offs = static_cast<math::Rect_offset>(i);
			if (RadioButton(rect_method_names[i],
					active_method == Method::rect && rect_offset == offs)) {
				active_method = Method::rect;
				rect_offset = offs;
				dirty = true;
			}
		}

		constexpr static std::pair<Method, const char*> other_methods[] = {
			{ Method::trapezoid, "Метод трапеций" },
			{ Method::simpson, "Метод Симпсона" },
		};
		for (auto [method, name]: other_methods) {
			if (RadioButton(name, active_method == method)) {
				active_method = method;
				dirty = true;
			}
		}
	}

	if (auto node = ImScoped::TreeNode("Функция")) {
		for (unsigned id = 0; id < std::size(functions); id++) {
			if (RadioButton(functions[id].name, id == active_function_id)) {
				active_function_id = id;
				dirty = true;
			}
		}
	}

	TextUnformatted("Область интегрирования");
	dirty |= DragMinMax("bounds", &low, &high, drag_speed, 1e-2);

	dirty |= Drag("Погрешность", &precision, 1e-4,
			min_precision, max_precision, nullptr, ImGuiSliderFlags_AlwaysClamp);

	TextUnformatted("Вид");
	graph.settings_widget();

	if (dirty)
		update_calculation();
}

void Integration::result_window () const
{
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		if (result.diverges) {
			TextFmtColored(gui::error_text_color,
					"Похоже, интеграл расходится на интервале ({}, {})\n", low, high);
		}
		TextFmt("Вычисленное значение интеграла: {:.6}\n", result.calculated);
		if (!std::isnan(result.exact)) TextFmt("(Точное значение: {:.6})\n", result.exact);
		TextFmt("{} интервалов для погрешности {}", result.subdivisions, precision);
	}
}

double Integration::integrate_once (unsigned subdivisions) const
{
	const auto& f = functions[active_function_id].compute;
	switch (active_method) {
	case Method::rect: return math::integrate_rect(f, low, high, subdivisions, rect_offset);
	case Method::trapezoid: return math::integrate_trapezoids(f, low, high, subdivisions);
	case Method::simpson: return math::integrate_simpson(f, low, high, subdivisions);
	}
	unreachable();
}


void Integration::update_calculation ()
{
	const Function_spec& f = functions[active_function_id];
	precision = std::clamp(precision, min_precision, max_precision);

	const double factor = 1.0 / [&] {
		switch (active_method) {
		case Method::rect: return rect_offset == math::Rect_offset::middle ? 3 : 1;
		case Method::trapezoid: return 3;
		case Method::simpson: return 15;
		}
		unreachable();
	} ();

	unsigned subdivisions = min_subdivisions;
	double last_result = integrate_once(subdivisions);
	double last_diff = std::numeric_limits<double>::max();

	constexpr int diverge_strike_threshold = 2;
	int diverge_strikes = 0;

	do {
		subdivisions *= 2;
		const double cur_result = integrate_once(subdivisions);
		const double diff = std::abs(cur_result - last_result) * factor;
		last_result = cur_result;

		if (std::exchange(last_diff, diff) < diff
		&& ++diverge_strikes == diverge_strike_threshold)
			break;
		if (diff < precision)
			break;
	} while (subdivisions < max_subdivisions);

	result = {
		.calculated = last_result,
		.exact = f.antiderivative(high) - f.antiderivative(low),
		.diverges = diverge_strikes >= diverge_strike_threshold,
		.subdivisions = subdivisions
	};
}