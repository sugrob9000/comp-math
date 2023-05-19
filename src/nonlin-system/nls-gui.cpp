#include "gui.hpp"
#include "imcpp20.hpp"
#include "nonlin-system/nls-gui.hpp"

using namespace ImGui;
using namespace ImScoped;

namespace {
struct Function_spec {
	const char* name;
	double (*compute) (double);
	double (*compute_dfdx) (double);
	// dfdy is implicitly -1
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
		"sqrt(x + 3)",
		[] (double x) { return sqrt(x + 3); },
		[] (double x) { return 0.5/sqrt(x + 3); }
	}
};

void result_window (const math::Newton_system_result r)
{
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		size_t guesses = r.guesses.size();

		TextFmt("Было сделано {} итераций.", guesses);
		if (r.root) {
			TextFmt("Корень: {:.6}", *r.root);
			if (guesses >= 2)
				TextFmt("Вектор погрешностей: {:.6}", r.guesses[guesses-1] - r.guesses[guesses-2]);
		} else {
			TextUnformatted("Алгоритм разошёлся.");
		}
	}
}
} // anon namespace

constexpr static uint32_t function_colors[2] = { 0xFF'AA00FF, 0xFF'FF00AA, };

bool Nonlinear_system::calculation_enabled () const
{
	return active_function_id[0] != -1 && active_function_id[1] != -1;
}

void Nonlinear_system::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	Graph_draw_context draw(graph);
	draw.background();
	for (int i: { 0, 1 }) {
		if (int id = active_function_id[i]; id != -1)
			draw.function_plot(function_colors[i], functions[id].compute);
	}

	for (int i: { 0, 1 })
		draw.ortho_line(i, initial_guess[i], 0xFF'22AA22, 3.0f);

	if (result) {
		const auto& r = *result;

		for (unsigned i = 1; i < r.guesses.size(); i++)
			draw.line(r.guesses[i-1], r.guesses[i], 0xFF'AA2222, 1.5f);
		if (r.root)
			draw.dot(*r.root, 0xFF'FF0000);

		result_window(r);
	}
}

void Nonlinear_system::settings_widget ()
{
	bool query_changed = false;

	TextUnformatted("Функции");
	if (auto table = ImScoped::Table("func", 3)) {
		TableNextRow();
		TableNextColumn();
		TextUnformatted("(выкл.)");
		for (int i: { 0, 1 }) {
			TableNextColumn();
			if (RadioButton(GenerateId(1000, i), active_function_id[i] == -1))
				active_function_id[i] = -1;
		}
		for (int func_id = 0; unsigned(func_id) < std::size(functions); func_id++) {
			TableNextRow();
			TableNextColumn();
			TextUnformatted(functions[func_id].name);
			for (int i: { 0, 1 }) {
				TableNextColumn();
				TableSetBgColor(ImGuiTableBgTarget_CellBg, function_colors[i] & 0x88'FFFFFF);
				if (RadioButton(GenerateId(func_id, i), active_function_id[i] == func_id)) {
					if (active_function_id[i^1] == func_id)
						std::swap(active_function_id[0], active_function_id[1]);
					else
						active_function_id[i] = func_id;
					query_changed = true;
				}
			}
		}
	}

	if (calculation_enabled()) {
		constexpr float drag_speed = 0.03;
		TextUnformatted("Начальное приближение");
		query_changed |= DragN("##guess", glm::value_ptr(initial_guess), 2, drag_speed);
		constexpr double min_precision = 1e-6;
		constexpr double max_precision = 1e-1;
		query_changed |= Drag("Погрешность", &precision, 1e-4,
				min_precision, max_precision, nullptr, ImGuiSliderFlags_Logarithmic);
	}

	TextUnformatted("Вид");
	graph.settings_widget();

	if (query_changed)
		update_calculation();
}

void Nonlinear_system::update_calculation ()
{
	if (calculation_enabled()) {
		const Function_spec& f1 = functions[active_function_id[0]];
		const Function_spec& f2 = functions[active_function_id[1]];
		result = math::newtons_method_system(
				f1.compute, f1.compute_dfdx,
				f2.compute, f2.compute_dfdx,
				initial_guess, precision);
	} else {
		result.reset();
	}
}