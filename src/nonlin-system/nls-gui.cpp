#include "gui.hpp"
#include "imcpp20.hpp"
#include "nonlin-system/nls-gui.hpp"
#include "util/util.hpp"

using namespace ImGui;
using namespace ImScoped;

namespace {
struct Function_spec {
	const char* name;
	double (*f) (dvec2);
	dvec2 (*parametric) (double);
	double parametric_low, parametric_high;
	double (*dfdx) (dvec2);
	double (*dfdy) (dvec2);
};
constexpr Function_spec functions[] = {
	{
		"x² + y² = 4",
		[] (dvec2 v) { return v.x*v.x + v.y*v.y - 4; },
		[] (double t) { return 2.0*dvec2(cos(t), sin(t)); },
		0, 7,
		[] (dvec2 v) { return 2 * v.x; },
		[] (dvec2 v) { return 2 * v.y; }
	},
	{
		"y = 3x²",
		[] (dvec2 v) { return -3 * v.x*v.x + v.y; },
		[] (double t) { return dvec2(t, 3*t*t); },
		-3, 3,
		[] (dvec2 v) { return -6 * v.x; },
		[] (dvec2 v [[maybe_unused]]) { return 1.0; }
	},
	{
		"xy = 1",
		[] (dvec2 v) { return v.x * v.y - 1; },
		[] (double t) { return dvec2(t, 1.0/t); },
		-3, 3,
		[] (dvec2 v [[maybe_unused]]) { return v.y; },
		[] (dvec2 v [[maybe_unused]]) { return v.x; }
	}
};
} // anon namespace

void Nonlinear_system::result_window () const
{
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		size_t guesses = result.guesses.size();
		TextFmt("Было сделано {} итераций.", guesses);
		if (result.root) {
			const dvec2 r = *result.root;
			TextFmt("Корень: {:.6}", r);
			if (guesses >= 1) {
				dvec2 fault = r - result.guesses[guesses-1];
				TextFmt("Вектор погрешностей: |{:.6}| = {:.6}", fault, glm::length(fault));
			}
			{
				const auto& f = functions[active_function_id[0]].f;
				const auto& g = functions[active_function_id[1]].f;
				TextFmt("Значения: {:.6}, {:.6}", f(r), g(r));
			}
		} else {
			TextUnformatted("Алгоритм разошёлся.");
		}
	}
}

constexpr static uint32_t function_colors[2] = { 0xFF'AA00FF, 0xFF'FF00AA, };

void Nonlinear_system::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	Graph_draw_context draw(graph);
	draw.background();
	for (int i: { 0, 1 }) {
		const Function_spec& f = functions[active_function_id[i]];
		draw.parametric_plot(function_colors[i], f.parametric, f.parametric_low, f.parametric_high);
	}

	draw.dot(initial_guess, 0xFF'22AA22);

	{ // Draw guess lines
		constexpr uint32_t guess_color = 0xFF'AA2222;
		constexpr float guess_thickness = 1.5f;
		for (unsigned i = 1; i < result.guesses.size(); i++)
			draw.line(result.guesses[i-1], result.guesses[i], guess_color, guess_thickness);
		if (result.root && !result.guesses.empty())
			draw.line(result.guesses.back(), *result.root, guess_color, guess_thickness);
	}

	if (result.root)
		draw.dot(*result.root, 0xFF'FF0000);
	result_window();
}

void Nonlinear_system::settings_widget ()
{
	bool query_changed = false;

	TextUnformatted("Функции");
	if (auto table = ImScoped::Table("func", 3)) {
		for (unsigned func_id = 0; func_id < std::size(functions); func_id++) {
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

	constexpr float drag_speed = 0.03;
	TextUnformatted("Начальное приближение");
	query_changed |= DragN("##guess", glm::value_ptr(initial_guess), 2, drag_speed);
	constexpr double min_precision = 1e-6;
	constexpr double max_precision = 1e-1;
	query_changed |= Drag("Погрешность", &precision, 1e-4,
			min_precision, max_precision, nullptr, ImGuiSliderFlags_Logarithmic);

	TextUnformatted("Вид");
	graph.settings_widget();

	if (query_changed)
		update_calculation();
}

void Nonlinear_system::update_calculation ()
{
	const Function_spec& f = functions[active_function_id[0]];
	const Function_spec& g = functions[active_function_id[1]];
	result = math::newtons_method_system(
			f.f, f.dfdx, f.dfdy,
			g.f, g.dfdx, g.dfdy,
			initial_guess, precision);
}