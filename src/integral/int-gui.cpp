#include "gui.hpp"
#include "imcpp20.hpp"
#include "integral/calc.hpp"
#include "integral/int-gui.hpp"

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
		[] (double x) { return 0.5 * sqrt(M_PI) * erf(x); }
	},
};
} // anon namespace

void Integration::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	result_window();

	Graph_draw_context draw(graph);
	draw.background();
	draw.function_plot(0xFF'55BB77, functions[active_function_id].compute);

	{
		constexpr uint32_t limit_color = 0xFF'BB5555;
		constexpr uint32_t outline_color = 0x88'CC0044;
		constexpr uint32_t fill_color = 0x22'C01054;
		constexpr uint32_t dot_color = fill_color | 0xFF'000000;

		constexpr float limit_thickness = 2.0;
		const double step = (high - low) / subdivisions;
		const auto f = functions[active_function_id].compute;

		draw.vert_line(low, limit_color, limit_thickness);
		draw.vert_line(high, limit_color, limit_thickness);

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
		case Method::trapezoid:
			// TODO: use the `prev` idiom
			draw.dot({ low, f(low) }, dot_color);
			for (unsigned i = 0; i < subdivisions; i++) {
				const double x1 = low + step * i;
				const double x2 = low + step * (i+1);
				const double y1 = f(x1);
				const double y2 = f(x2);
				draw.trapezoid(x1, x2, 0, y1, y2, outline_color, fill_color);
				draw.dot({ x2, y2 }, dot_color);
			}
			break;
		case Method::simpson:
			break;
		}
	}
}

void Integration::settings_widget ()
{
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
	constexpr float drag_speed = 0.03;
	constexpr double min_width = 1e-2;
	query_changed |= gui::drag_low_high("integral", low, high, 0.03, 1e-2, "%.2f");
	query_changed |= Drag("Подинтервалы", &subdivisions, 0.1, 3u, 1'000'000u);

	TextUnformatted("Вид");
	graph.settings_widget();

	if (query_changed)
		update_calculation();
}

void Integration::result_window () const
{
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		TextFmt("Значение интеграла: {:.6}", result.value);
		TextFmt("Точное значение: {:.6}", exact_result);
	}
}

void Integration::update_calculation ()
{
	const Function_spec& f = functions[active_function_id];

	exact_result = f.antiderivative(high) - f.antiderivative(low);

	switch (active_method) {
	case Method::rect:
		result = math::integrate_rect(f.compute, low, high, subdivisions, rect_offset);
		break;
	case Method::trapezoid:
		result = math::integrate_trapezoids(f.compute, low, high, subdivisions);
		break;
	case Method::simpson: break;
	}
}