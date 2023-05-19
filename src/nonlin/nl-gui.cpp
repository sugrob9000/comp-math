#include "gui.hpp"
#include "imcpp20.hpp"
#include "nonlin/calc.hpp"
#include "nonlin/nl-gui.hpp"
#include "util/util.hpp"
#include <cmath>
#include <utility>

using namespace ImGui;
using namespace ImScoped;

namespace {
struct Function_spec {
	const char* name;
	double (*compute) (double);
	double (*compute_derivative) (double);
};
constexpr Function_spec functions[] = {
	{
		"x² - 0.9",
		[] (double x) { return x*x-0.9; },
		[] (double x) { return 2*x; }
	},
	{
		"sin(x) ln(2x + 2) - 0.5",
		[] (double x) { return sin(x) * log(2*x + 2) - 0.5; },
		[] (double x) { return sin(x) / (x+1) + log(2*x + 2)*cos(x); }
	},
	{
		"exp(-x²) - 0.5",
		[] (double x) { return exp(-x*x) - 0.5; },
		[] (double x) { return -2 * exp(-x*x) * x; }
	},
	{
		"sqrt(x + 3) - 3.333",
		[] (double x) { return sqrt(x + 3) - 3.333; },
		[] (double x) { return 0.5/sqrt(x + 3); }
	}
};
} // anon namespace

void Nonlinear::update_calculation ()
{
	if (no_chosen_method()) return;

	auto f = functions[active_function_id].compute;
	auto dfdx = functions[active_function_id].compute_derivative;

	visit_calculation(
			[&] (math::Chords_result& cr) {
				cr = math::build_chords(f, seek_low, seek_high, precision);
			},
			[&] (math::Newton_result& nr) {
				nr = math::newtons_method(f, dfdx, initial_guess, precision);
			},
			[&] (math::Iteration_result& ir) {
				ir = math::fixed_point_iteration(f, lambda, initial_guess, precision);
			});
}


void Nonlinear::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();
	maybe_result_window();

	{ // Draw canvas
		Graph_draw_context context(graph);
		context.background();
		context.function_plot(0xFF'AA00FF, functions[active_function_id].compute);
		// TODO also visualize results
	}
}

template <typename T> void Nonlinear::method_option_widget (const char* name)
{
	if (RadioButton(name, std::holds_alternative<T>(calculation))) {
		calculation.emplace<T>();
		update_calculation();
	}
}

template <typename... Ops> void Nonlinear::visit_calculation (Ops&&... ops)
{
	std::visit(Overloaded{ [] (std::monostate) {}, std::forward<Ops>(ops)... }, calculation);
}

template <typename... Ops> void Nonlinear::visit_calculation (Ops&&... ops) const
{
	std::visit(Overloaded{ [] (std::monostate) {}, std::forward<Ops>(ops)... }, calculation);
}

template <typename T> bool Nonlinear::chosen_method_is () const
{
	return std::holds_alternative<T>(calculation);
}

bool Nonlinear::no_chosen_method () const
{
	return chosen_method_is<std::monostate>();
}


void Nonlinear::settings_widget ()
{
	constexpr float drag_speed = 0.03;

	if (auto t = ImScoped::TreeNode("Метод")) {
		method_option_widget<std::monostate>("(выкл.)");
		method_option_widget<math::Chords_result>("Хорд");
		method_option_widget<math::Newton_result>("Ньютона");
		method_option_widget<math::Iteration_result>("Простой итерации");

		constexpr auto min = std::numeric_limits<double>::lowest();
		constexpr auto max = std::numeric_limits<double>::max();
		bool changed = false;

		if (chosen_method_is<math::Iteration_result>()) {
			PushItemWidth(CalcItemWidth() * 0.5);
			changed |= Drag("λ", &lambda, drag_speed);
			SameLine();
			if (double one_over = 1.0/lambda; Drag("1/λ", &one_over, drag_speed)) {
				lambda = 1.0 / one_over;
				changed = true;
			}
			PopItemWidth();
		}

		// Input interval when using chords, just one initial guess otherwise
		const char* fmt = "%.2f";
		visit_calculation(
				[&] (math::Chords_result& r) {
					TextUnformatted("Интервал изоляции корня");
					const double min_seek_width = 1e-2;
					PushItemWidth(CalcItemWidth() * 0.5);
					changed |= Drag("##il", &seek_low, drag_speed, min, seek_high - min_seek_width, fmt);
					SameLine();
					changed |= Drag("##ih", &seek_high, drag_speed, seek_low + min_seek_width, max, fmt);
					PopItemWidth();
				},
				[&] (math::Result& r) {
					changed |= Drag("Начальная оценка", &initial_guess, drag_speed, min, max, fmt);
				});

		if (!no_chosen_method()) {
			constexpr double min_precision = 1e-6;
			constexpr double max_precision = 1e-1;
			changed |= Drag("Погрешность", &precision, 1e-4,
					min_precision, max_precision, nullptr, ImGuiSliderFlags_Logarithmic);
		}

		if (changed) update_calculation();
	}

	if (auto t = ImScoped::TreeNode("Функция")) {
		for (unsigned id = 0; id < std::size(functions); id++) {
			if (RadioButton(functions[id].name, active_function_id == id)) {
				active_function_id = id;
				update_calculation();
			}
		}
	}

	if (auto t = ImScoped::TreeNode("Вид"))
		graph.settings_widget();
}

void Nonlinear::maybe_result_window () const
{
	if (no_chosen_method()) return;

	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		PushTextWrapPos(300);
		visit_calculation(
				[&] (const math::Chords_result& r) {
					if (r.has_root) {
						TextFmt("{} хорд, чтобы достичь точности {}\n"
								"Оценка корня: {:.6}\n"
								"Значение функции: {:.6}",
								r.lines.size(), precision,
								r.root, r.value_at_root);
					} else {
						TextUnformatted(
								"Функция принимает значения одного знака на концах интервала: "
								"нельзя начать алгоритм хорд.");
					}
				},
				[&] (const math::Newton_result& r) {
					if (r.has_root) {
						TextFmt("{} касательных, чтобы достичь точности {}",
								r.lines.size(), precision);
						TextFmt("Оценка корня: {:.6}", r.root);
					} else {
						TextFmt("Алгоритм расходится после {} итераций", r.lines.size());
					}
				},
				[&] (const math::Iteration_result& r) {
					if (r.has_root) {
						TextFmt("{} шагов, чтобы достичь точности {}", r.steps.size(), precision);
						TextFmt("Оценка корня: {:.6}", r.root);
					} else {
						TextFmt("Алгоритм расходится после {} итераций", r.steps.size());
					}
				});
		PopTextWrapPos();
	}
}

/*
 * TODO: restore the visualization of results
 */

#if 0
struct Graph_draw_context2 {
	void vertical_line (double world_x, uint32_t color) {
		const float x = screen_transform_x(nl.view_transform_x(world_x));
		dl.AddLine({ x, pos.y }, { x, end.y }, color, 2.0);
	}

	void line (dvec2 a, dvec2 b, uint32_t color) {
		auto aa = screen_transform(nl.view_transform(a));
		auto bb = screen_transform(nl.view_transform(b));
		dl.AddLine({ aa.x, aa.y }, { bb.x, bb.y }, color, 1.5);
	}

	void calculation_result () {
		constexpr uint32_t color_line = 0xFFAAAA00;
		constexpr uint32_t color_root = 0xFF0000FF;

		// Show appropriate intermediate steps
		nl.visit_calculation(
				[&] (const math::Line_based_result& r) {
					for (const auto& [a, b]: r.lines)
						line(a, b, color_line);
				},
				[&] (const math::Iteration_result& r) {
					for (size_t i = 0; i < r.steps.size(); i++) {
						float factor = float(i) / (r.steps.size());
						vertical_line(r.steps[i], IM_COL32(200, 180, 55, 80 + 175 * factor));
					}
				});

		// Only draw the two lines on the chords method, otherwise only one is relevant
		nl.visit_calculation(
				[&] (const math::Chords_result&) {
					vertical_line(nl.seek_low, color_interval);
					vertical_line(nl.seek_high, color_interval);
				},
				[&] (const math::Result&) {
					vertical_line(nl.initial_guess, color_interval);
				});

		// Any result may have a root
		nl.visit_calculation(
				[&] (const math::Result& r) {
					if (r.has_root)
						vertical_line(r.root, color_root);
				});
	}
};
#endif