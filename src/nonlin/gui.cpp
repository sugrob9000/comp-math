#include "imcpp20.hpp"
#include "nonlin/calc.hpp"
#include "nonlin/gui.hpp"
#include "util/util.hpp"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <utility>

using namespace ImGui;
using namespace ImScoped;

constexpr static ImGuiWindowFlags floating_window_flags
= ImGuiWindowFlags_AlwaysAutoResize;

constexpr static ImGuiWindowFlags fullscreen_window_flags
= ImGuiWindowFlags_NoCollapse
| ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoMove
| ImGuiWindowFlags_NoSavedSettings
| ImGuiWindowFlags_NoTitleBar
| ImGuiWindowFlags_NoBringToFrontOnFocus;

struct Function_spec {
	const char* name;
	double (*compute) (double);
	double (*compute_derivative) (double);
};
constexpr static Function_spec functions[] = {
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
	}
};


double Nonlinear::view_transform_x (double x) const
{
	return (x - view_low.x) / (view_high.x - view_low.x);
}

double Nonlinear::view_transform_y (double y) const
{
	return 1.0 - (y - view_low.y) / (view_high.y - view_low.y);
}

double Nonlinear::view_transform_i (Coordinate i, double coord) const
{
	return i == X ? view_transform_x(coord) : view_transform_y(coord);
}

math::Dvec2 Nonlinear::view_transform (math::Dvec2 v) const
{
	return { view_transform_x(v.x), view_transform_y(v.y) };
}


void Nonlinear::update_calculation ()
{
	auto f = functions[active_function_id].compute;
	auto dfdx = functions[active_function_id].compute_derivative;

	visit_calculation(
			[&] (math::Chords_result& cr) {
				cr = math::build_chords(f, seek_low, seek_high, precision);
			},
			[&] (math::Newton_result& nr) {
				nr = math::newtons_method(f, dfdx, seek_low, precision);
			},
			[&] (math::Iteration_result& ir) {
				ir = math::fixed_point_iteration(f, lambda, (seek_low + seek_high) * 0.5, precision);
			});
}


void Nonlinear::gui_frame ()
{
	if (auto w = Window("Параметры", nullptr, floating_window_flags))
		settings_widget();

	maybe_result_window();

	const auto& viewport = ImGui::GetMainViewport();
	SetNextWindowPos(viewport->WorkPos);
	SetNextWindowSize(viewport->WorkSize);
	if (auto w = Window("graph", nullptr, fullscreen_window_flags)) {
		update_graph_cache();
		graph_widget();
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

void Nonlinear::settings_widget ()
{
	constexpr float drag_speed = 0.03;

	if (auto t = ImScoped::TreeNode("Метод")) {
		method_option_widget<std::monostate>("(выкл.)");
		method_option_widget<math::Chords_result>("Хорд");
		method_option_widget<math::Newton_result>("Ньютона");
		method_option_widget<math::Iteration_result>("Простой итерации");

		bool changed = false;

		if (std::holds_alternative<math::Iteration_result>(calculation)) {
			constexpr double threshold = 0.001;
			changed |= Drag("λ", &lambda, drag_speed, threshold, 1/threshold);
			if (double one_over = 1.0/lambda; Drag("1/λ", &one_over, drag_speed, threshold)) {
				lambda = 1.0 / one_over;
				changed = true;
			}
		}

		constexpr auto min = std::numeric_limits<double>::lowest();
		constexpr auto max = std::numeric_limits<double>::max();

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
					changed |= Drag("Начальная оценка", &seek_low, drag_speed, min, max, fmt);
				});

		constexpr double min_precision = 1e-6;
		constexpr double max_precision = 1e-1;
		changed |= Drag("Погрешность", &precision, 1e-4,
				min_precision, max_precision, nullptr, ImGuiSliderFlags_Logarithmic);

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

	if (auto t = ImScoped::TreeNode("Вид")) {
		const math::Dvec2 center = (view_low + view_high) * 0.5;
		if (math::Dvec2 delta = center; DragN("Центр", glm::value_ptr(delta), 2, drag_speed)) {
			delta -= center;
			view_low += delta;
			view_high += delta;
		}

		const math::Dvec2 scale = (view_high - view_low);
		constexpr double min_scale = 0.5;
		constexpr double max_scale = 250;
		if (math::Dvec2 resc = scale;
				DragN("Масштаб", glm::value_ptr(resc), 2, drag_speed, min_scale, max_scale)) {
			resc /= scale;
			view_low = center + (view_low - center) * resc;
			view_high = center + (view_high - center) * resc;
		}
	}
}

void Nonlinear::maybe_result_window () const
{
	if (std::holds_alternative<std::monostate>(calculation)) return;
	if (auto w = Window("Результат", nullptr, floating_window_flags)) {
		PushTextWrapPos(300);
		visit_calculation(
				[&] (const math::Chords_result& r) {
					if (r.has_root) {
						TextFmt("{} хорд, чтобы достичь точности {}", r.lines.size(), precision);
						TextFmt("Оценка корня: {:.6}", r.root);
					} else {
						TextUnformatted(
								"Функция принимает значения одного знака на концах интервала:"
								"наличие корня не гарантировано.");
					}
				},
				[&] (const math::Newton_result& r) {
					if (r.has_root) {
						TextFmt("{} касательных, чтобы достичь точности {}",
								r.lines.size(), precision);
						TextFmt("Оценка корня: {:.6}", r.root);
					} else {
						TextFmt("Метод не сошёлся после {} итераций.", r.lines.size());
					}
				},
				[&] (const math::Iteration_result& r) {
					if (r.has_root) {
						TextFmt("{} шагов, чтобы достичь точности {}", r.steps.size(), precision);
						TextFmt("Оценка корня: {:.6}", r.root);
					} else {
						TextFmt("Метод не сошёлся после {} итераций.", r.steps.size());
					}
				});
		PopTextWrapPos();
	}
}

void Nonlinear::update_graph_cache ()
{
	if (graph_cache.function_id == active_function_id
	&& graph_cache.view_low == view_low && graph_cache.view_high == view_high)
		return;

	const double step = (view_high.x - view_low.x) / std::size(graph_cache.buffer);
	double x = view_low.x;
	const auto compute = functions[active_function_id].compute;
	for (auto& d: graph_cache.buffer) {
		d = view_transform_y(compute(x));
		x += step;
	}

	graph_cache.function_id = active_function_id;
	graph_cache.view_low = view_low;
	graph_cache.view_high = view_high;
}


struct Graph_draw_context {
	const Nonlinear& nl;
	ImDrawList& dl;
	ImVec2 pos, end, size;
	uint32_t color_thick_lines, color_thin_lines;
	constexpr static uint32_t color_interval = 0xFFFF0000;

	float screen_transform_x (double x) { x *= size.x; x += pos.x; return x; }
	float screen_transform_y (double y) { y *= size.y; y += pos.y; return y; }
	float screen_transform_i (Nonlinear::Coordinate i, double coord) {
		return i == Nonlinear::X ? screen_transform_x(coord) : screen_transform_y(coord);
	}
	math::Fvec2 screen_transform (math::Fvec2 v) {
		return { screen_transform_x(v.x), screen_transform_y(v.y) };
	}

	constexpr static int ideal_num_gridlines = 20;
	constexpr static double gridline_steps[] = {
		0.01, 0.05, 0.1, 0.25, 0.5, 1, 2, 5, 10, 25, 50, 100
	};

	Graph_draw_context (const Nonlinear& nl_)
		: nl{nl_}, dl{*GetWindowDrawList()},
		pos{dl.GetClipRectMin()}, end{dl.GetClipRectMax()},
		size{ end.x - pos.x, end.y - pos.y },
		color_thick_lines{ GetColorU32(ImGuiCol_Text) },
		color_thin_lines{ GetColorU32(ImGuiCol_TextDisabled) } {}

	void gridlines_for_coordinate (Nonlinear::Coordinate i) {
		const double vl = nl.view_low[i];
		const double vh = nl.view_high[i];

		// Choose the grid step
		double step = gridline_steps[0];
		for (double min_step = (vh - vl) / ideal_num_gridlines; auto cand: gridline_steps) {
			if (min_step <= cand) {
				step = cand;
				break;
			}
		}

		// Draw the lines and numbers
		for (double coord = step * std::ceil(vl / step); coord < vh; coord += step) {
			if (std::abs(coord) < step * 0.5) continue; // Don't duplicate the axes
			ImVec2 line_begin = pos, line_end = end;
			line_begin[i] = line_end[i] = screen_transform_i(i, nl.view_transform_i(i, coord));
			dl.AddLine(line_begin, line_end, color_thin_lines, 1.0);
			char buf[10];
			char* buf_end = fmt::format_to_n(buf, std::size(buf), "{:.3}", coord).out;
			dl.AddText(line_begin, color_thin_lines, buf, buf_end);
		}
	}

	void xy_axes () {
		math::Fvec2 zero = screen_transform(nl.view_transform({ 0, 0 }));
		dl.AddLine({ zero.x, pos.y }, { zero.x, end.y }, color_thick_lines, 3.0f);
		dl.AddLine({ pos.x, zero.y }, { end.x, zero.y }, color_thick_lines, 3.0f);
		const char zero_char[1] = { '0' };
		dl.AddText({ pos.x, zero.y }, color_thick_lines, zero_char, zero_char+1);
		dl.AddText({ zero.x + 2, pos.y }, color_thick_lines, zero_char, zero_char+1);
	}

	void vertical_line (double world_x, uint32_t color) {
		const float x = screen_transform_x(nl.view_transform_x(world_x));
		dl.AddLine({ x, pos.y }, { x, end.y }, color, 2.0);
	}

	void line (math::Dvec2 a, math::Dvec2 b, uint32_t color) {
		auto aa = screen_transform(nl.view_transform(a));
		auto bb = screen_transform(nl.view_transform(b));
		dl.AddLine({ aa.x, aa.y }, { bb.x, bb.y }, color, 1.5);
	}

	void function_graph (uint32_t color) {
		constexpr size_t steps = Nonlinear::Graph_display_cache::resolution;
		float x = pos.x;
		const float x_step = size.x / steps;
		for (size_t i = 1; i < steps; i++, x += x_step) {
			const float norm_y1 = nl.graph_cache.buffer[i-1];
			const float norm_y2 = nl.graph_cache.buffer[i];
			// Skip if both heights is offscreen
			if ((norm_y1 >= 0 && norm_y1 <= 1) || (norm_y2 >= 0 && norm_y1 <= 1)) {
				const float y1 = screen_transform_y(norm_y1);
				const float y2 = screen_transform_y(norm_y2);
				dl.AddLine({ x, y1 }, { x + x_step, y2 }, color, 2.0f);
			}
		}
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
					vertical_line(nl.seek_low, color_interval);
				});

		// Any result may have a root
		nl.visit_calculation(
				[&] (const math::Result& r) {
					if (r.has_root)
						vertical_line(r.root, color_root);
				});
	}
};

void Nonlinear::graph_widget () const
{
	Graph_draw_context g(*this);

	g.gridlines_for_coordinate(X);
	g.gridlines_for_coordinate(Y);
	g.xy_axes();

	g.function_graph(0xFF'AA00FF);

	g.calculation_result();
}