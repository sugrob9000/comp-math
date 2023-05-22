#include "gui.hpp"
#include "imcpp20.hpp"
#include "integral/calc.hpp"
#include "integral/int-gui.hpp"
#include "util/util.hpp"
#include <algorithm>
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
	const auto f = functions[active_function_id].compute;
	const double step = (high - low) / subdivisions;

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
		for (unsigned i = 0; i < subdivisions; i++) {
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
		// TODO: use the `prev` idiom
		draw.dot({ low, f(low) }, dot_color);
		dvec2 prev = { low, f(low) };

		for (unsigned i = 0; i < subdivisions; i++) {
			dvec2 cur;
			cur.x = low + step * i;

			const double x1 = low + step * i;
			const double x2 = low + step * (i+1);
			const double y1 = f(x1);
			const double y2 = f(x2);
			draw.trapezoid(x1, x2, 0, y1, y2, outline_color, fill_color);
			draw.dot({ x2, y2 }, dot_color);
		}
		break;
	}

	case Method::simpson: {
		unsigned n = subdivisions;
		if (n % 2 == 1) n++;
		const double step_corrected = (high - low) / n;
		for (unsigned i = 0; i <= n; i++) {
			const double x = low + step_corrected * i;
			draw.dot({ x, f(x) }, dot_color);
		}
		break;
	}
	}
}

void Integration::settings_widget ()
{
	constexpr float drag_speed = 0.03;
	constexpr float drag_speed_fast = 0.1;

	bool query_changed = false;

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
				query_changed = true;
			}
		}

		constexpr static std::pair<Method, const char*> other_methods[] = {
			{ Method::trapezoid, "Метод трапеций" },
			{ Method::simpson, "Метод Симпсона" },
		};
		for (auto [method, name]: other_methods) {
			if (RadioButton(name, active_method == method)) {
				active_method = method;
				query_changed = true;
			}
		}
	}

	if (auto node = ImScoped::TreeNode("Функция")) {
		for (unsigned id = 0; id < std::size(functions); id++) {
			if (RadioButton(functions[id].name, id == active_function_id)) {
				active_function_id = id;
				query_changed = true;
			}
		}
	}

	TextUnformatted("Область интегрирования");
	constexpr double min_width = 1e-2;
	query_changed |= DragMinMax("bounds", &low, &high, drag_speed, 1e-2);

	TextUnformatted("Точность");

	if (RadioButton("Погрешность", precision_spec == Precision_spec::by_precision)) {
		precision_spec = Precision_spec::by_precision;
		query_changed = true;
	}
	SameLine();

	const auto drag = [&] <ScalarType T>
		(const char* id, T* p, T min, T max, float speed, Precision_spec precision_spec_required) {
			BeginDisabled(precision_spec != precision_spec_required);
			query_changed |= Drag(id, p, speed, min, max, nullptr, ImGuiSliderFlags_AlwaysClamp);
			EndDisabled();
		};

	drag("##prec", &precision, min_precision, max_precision,
			1e-4, Precision_spec::by_precision);

	if (RadioButton("Подинтервалы", precision_spec == Precision_spec::by_num_subdivisions)) {
		precision_spec = Precision_spec::by_num_subdivisions;
		query_changed = true;
	}
	SameLine();
	drag("##subdiv", &subdivisions, min_subdivisions, max_subdivisions,
			drag_speed_fast, Precision_spec::by_num_subdivisions);

	TextUnformatted("Вид");
	graph.settings_widget();

	if (query_changed)
		update_calculation();
}

void Integration::result_window () const
{
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		TextFmt("Значение интеграла: {:.6}\n(Точное значение: {:.6})\n",
				calculated_result, exact_result);
		if (precision_spec == Precision_spec::by_precision)
			TextFmt("{} интервалов для погрешности {}", subdivisions, precision);
	}
}

double Integration::integrate_once_with_current_settings () const
{
	const Function_spec& f = functions[active_function_id];
	switch (active_method) {
	case Method::rect:
		return math::integrate_rect(f.compute, low, high, subdivisions, rect_offset);
	case Method::trapezoid:
		return math::integrate_trapezoids(f.compute, low, high, subdivisions);
	case Method::simpson:
		return math::integrate_simpson(f.compute, low, high, subdivisions);
	}
	unreachable();
}


void Integration::update_calculation ()
{
	const Function_spec& f = functions[active_function_id];
	exact_result = f.antiderivative(high) - f.antiderivative(low);

	precision = std::clamp(precision, min_precision, max_precision);
	subdivisions = std::clamp(subdivisions, min_subdivisions, max_subdivisions);

	switch (precision_spec) {
		using enum Precision_spec;
	case by_precision: {
		subdivisions = min_subdivisions;
		double last_result = integrate_once_with_current_settings();
		constexpr static int method_precision_orders[3] = { 3, 3, 15 };
		const double factor = 1.0 / method_precision_orders[static_cast<int>(active_method)];
		do {
			subdivisions *= 2;
			calculated_result = integrate_once_with_current_settings();
			const double diff = std::abs(calculated_result - last_result) * factor;
			last_result = calculated_result;
			if (diff < precision)
				break;
		} while (subdivisions < max_subdivisions);
		break;
	}
	case by_num_subdivisions:
		calculated_result = integrate_once_with_current_settings();
		break;
	}
}