#include <cassert>
#include <diffeq/calc.hpp>
#include <diffeq/de-gui.hpp>
#include <gui.hpp>
#include <imhelper.hpp>
#include <util/util.hpp>

namespace {
struct Equation_spec {
	const char* name;
	double (*compute_f) (dvec2);
	double (*compute_exact_solution) (dvec2 y0, double x);
};
constexpr Equation_spec equations[] = {
	{
		"y' = y + (1 + x) y²",
		[] (dvec2 v) -> double { return v.y + (1 + v.x)*v.y*v.y; },
		[] (dvec2 v0, double x) -> double {
			const double ex0 = std::exp(v0.x);
			const double C = -ex0 / v0.y - v0.x * ex0;
			const double ex = std::exp(x);
			return -ex / (C + ex * x);
		}
	},
	{
		"y' = xy",
		[] (dvec2 v) -> double { return v.x * v.y; },
		[] (dvec2 v0, double x) -> double {
			const double C = v0.y / std::exp(v0.x * v0.x * 0.5);
			return C * exp(x * x * 0.5);
		}
	},
	{
		"y' = x + y",
		[] (dvec2 v) -> double { return v.x + v.y; },
		[] (dvec2 v0, double x) -> double {
			const double C = (v0.y + v0.x + 1) / std::exp(v0.x);
			return C * std::exp(x) - x - 1;
		}
	}
};
} // anon namespace

const Diff_eq::Method_spec Diff_eq::methods[] = {
	{ "Эйлера (модифицированный)", 2 },
	{ "Рунге-Кутта (4 порядка)", 4 },
	{ "Милна", 0 },
};

using namespace ImGui;
using namespace ImScoped;

void Diff_eq::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		TextFmt("{} точек (шаг {:.4}).\n"
				"Точность {} {}.",
				output.points.size(), output.step,
				precision,
				(output.reached_precision ? "достигнута" : "не достигнута (алгоритм остановлен)"));
		TextFmt("Значение функции в конце интервала: {:.5}\n(точное: {:.5})",
				output.points.back().y,
				equations[active_equation_id].compute_exact_solution({ low, y_low }, high));
	}

	Graph_draw_context draw(graph);
	draw.background();
	for (double x: { low, high })
		draw.vert_line(x, 0xFF'6666FF, 2.0);

	const auto& pts = output.points;
	for (size_t i = 1; i < pts.size(); i++)
		draw.line(pts[i-1], pts[i], 0xFF'55CC55, 2.0);

	if (show_exact_solution) {
		const auto exact_fn = [this] (double x) -> double {
			return equations[active_equation_id].compute_exact_solution({ low, y_low }, x);
		};
		draw.function_plot(0x77'333333, exact_fn, low, high, 100, 1.0);
	}
}

void Diff_eq::settings_widget ()
{
	constexpr float drag_speed = 0.03f;
	bool dirty = false;

	if (auto node = ImScoped::TreeNode("Метод")) {
		for (unsigned i = 0; i < std::size(methods); i++) {
			const auto m = static_cast<Method>(i);
			if (RadioButton(methods[i].name, method == m)) {
				method = m;
				dirty = true;
			}
		}
	}

	if (auto node = ImScoped::TreeNode("Уравнение")) {
		for (unsigned id = 0; id < std::size(equations); id++) {
			if (RadioButton(equations[id].name, id == active_equation_id)) {
				active_equation_id = id;
				dirty = true;
			}
		}

		Checkbox("Показать точное решение", &show_exact_solution);

		TextUnformatted("Диапазон");
		dirty |= DragMinMax("range", &low, &high, drag_speed, 0.02);
		dirty |= Drag("Точность", &precision, drag_speed, 1e-4, 10.0);
		dirty |= Drag(fmt::format("y({})##y_low", low).c_str(), &y_low, drag_speed);
	}

	if (auto node = ImScoped::TreeNode("Вид")) graph.settings_widget();

	if (dirty) update_calculation();
}

void Diff_eq::update_calculation ()
{
	const auto& eq = equations[active_equation_id];
	const auto& f = eq.compute_f;
	const unsigned precision_order = methods[static_cast<int>(method)].precision_order;

	const auto compute = [&, this] (std::vector<dvec2>& target, size_t new_size) {
		assert(new_size > 1);
		target.resize(new_size);
		const size_t intervals = target.size()-1;
		const double step = (high - low) / intervals;
		switch (method) {
		case Method::euler_modified:
			math::diffeq_euler_modified(f, low, y_low, step, target);
			break;
		case Method::runge_kutta_4:
			math::diffeq_runge_kutta4(f, low, y_low, step, target);
			break;
		case Method::milne: unreachable();
		}
	};

	output.reached_precision = false;

	if (method == Method::milne) {
		std::vector<dvec2> cur;
		cur.resize(3);

		for (int i = 0; i < 14; i++) {
			cur.resize((cur.size()-1) * 2 + 1);
			const double step = (high - low) / (cur.size()-1);

			math::diffeq_milne(f, low, y_low, step, precision, cur);

			const double xn = low + step * (cur.size()-1);
			const double yn = eq.compute_exact_solution({ low, y_low }, xn);

			if (std::abs(cur.back().y - yn) < precision) {
				output.reached_precision = true;
				break;
			}
		}
		output.points = std::move(cur);
	} else {
		std::vector<dvec2> prev, cur;

		constexpr int begin_order = 1;
		compute(prev, (1 << begin_order) + 1);

		const double diff_factor = ((1 << precision_order) - 1);
		const double want_diff = precision * diff_factor;

		for (int i = 0; i < 14; i++) {
			compute(cur, (prev.size()-1) * 2 + 1);
			const double diff = std::abs(prev.back().y - cur.back().y);
			if (diff < want_diff) {
				output.reached_precision = true;
				break;
			}
			prev = std::move(cur);
		}
		output.points = std::move(prev);
	}
	output.step = (high - low) / (output.points.size()-1);
}