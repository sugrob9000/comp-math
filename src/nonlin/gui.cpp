#include "imcpp20.hpp"
#include "nonlin/gui.hpp"
#include "util/util.hpp"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <utility>

using namespace ImGui;
using namespace ImScoped;

constexpr static ImGuiWindowFlags fullscreen_window_flags
= ImGuiWindowFlags_NoCollapse
| ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoMove
| ImGuiWindowFlags_NoSavedSettings
| ImGuiWindowFlags_NoTitleBar
| ImGuiWindowFlags_NoBringToFrontOnFocus;

struct Function_spec {
	double (*compute) (double);
	const char* name;
};

constexpr static Function_spec functions[] = {
	{ [] (double x) { return x*x - 0.9; }, "x² - 0.9" },
	{ [] (double x) { return sin(x) * log(2*x + 2) - 0.5; }, "sin(x) ln(2x + 2) - 0.5" },
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

Dvec2 Nonlinear::view_transform (Dvec2 v) const
{
	return { view_transform_x(v.x), view_transform_y(v.y) };
}


void Nonlinear::gui_frame ()
{
	if (auto w = Window("Параметры", nullptr))
		settings_widget();

	const auto& viewport = ImGui::GetMainViewport();
	SetNextWindowPos(viewport->WorkPos);
	SetNextWindowSize(viewport->WorkSize);
	if (auto w = Window("graph", nullptr, fullscreen_window_flags)) {
		update_graph_cache();
		graph_widget();
	}
}

void Nonlinear::settings_widget ()
{
	constexpr float drag_speed = 0.03;

	if (auto t = ImScoped::TreeNode("Метод")) {
		constexpr static std::pair<Method, const char*> methods[] = {
			{ Method::chords, "Хорд" },
			{ Method::newton, "Ньютона" },
			{ Method::iteration, "Простой итерации" },
		};
		for (auto [method, name]: methods) {
			if (RadioButton(name, active_method == method))
				active_method = method;
		}

		constexpr auto min = std::numeric_limits<double>::lowest();
		constexpr auto max = std::numeric_limits<double>::max();
		constexpr auto small = std::numeric_limits<double>::min();  // >0

		TextUnformatted("Интервал изоляции корня");
		const double min_seek_width = 1e-2;
		const char* printf_fmt = "%.2f";
		PushItemWidth(CalcItemWidth() * 0.5);
		Drag("##il", &seek_low, drag_speed, min, seek_high - min_seek_width, printf_fmt);
		SameLine();
		Drag("##ih", &seek_high, drag_speed, seek_low + min_seek_width, max, printf_fmt);
		PopItemWidth();

		Drag("Точность", &precision, 1e-4, small, 0.1, nullptr, ImGuiSliderFlags_Logarithmic);
	}

	if (auto t = ImScoped::TreeNode("Функция")) {
		for (unsigned id = 0; id < std::size(functions); id++) {
			if (RadioButton(functions[id].name, active_function_id == id))
				active_function_id = id;
		}
	}

	if (auto t = ImScoped::TreeNode("Вид")) {
		const Dvec2 center = (view_low + view_high) * 0.5;
		if (Dvec2 delta = center; DragN("Центр", glm::value_ptr(delta), 2, drag_speed)) {
			delta -= center;
			view_low += delta;
			view_high += delta;
		}

		const Dvec2 scale = (view_high - view_low);
		constexpr double min_scale = 0.5;
		constexpr double max_scale = 250;
		if (Dvec2 resc = scale;
				DragN("Размер", glm::value_ptr(resc), 2, drag_speed, min_scale, max_scale)) {
			resc /= scale;
			view_low = center + (view_low - center) * resc;
			view_high = center + (view_high - center) * resc;
		}
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
	ImVec2 pos;
	ImVec2 end;
	ImVec2 size;

	uint32_t color_thick_lines;
	uint32_t color_thin_lines;
	constexpr static uint32_t color_interval = 0xFFFF0000;

	float screen_transform_x (double x) { x *= size.x; x += pos.x; return x; }
	float screen_transform_y (double y) { y *= size.y; y += pos.y; return y; }
	float screen_transform_i (Nonlinear::Coordinate i, double coord) {
		return i == Nonlinear::X ? screen_transform_x(coord) : screen_transform_y(coord);
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
			if (std::abs(coord) < step * 0.5) continue; // Don't duplicate the 0-lines
			ImVec2 line_begin = pos, line_end = end;
			line_begin[i] = line_end[i] = screen_transform_i(i, nl.view_transform_i(i, coord));
			dl.AddLine(line_begin, line_end, color_thin_lines, 1.0);
			char buf[10];
			char* buf_end = fmt::format_to_n(buf, std::size(buf), "{:.3}", coord).out;
			dl.AddText(line_begin, color_thin_lines, buf, buf_end);
		}
	}

	void xy_axes () {
		Fvec2 zero = {
			screen_transform_x(nl.view_transform_x(0)),
			screen_transform_y(nl.view_transform_y(0))
		};
		dl.AddLine({ zero.x, pos.y }, { zero.x, end.y }, color_thick_lines, 2.0f);
		dl.AddLine({ pos.x, zero.y }, { end.x, zero.y }, color_thick_lines, 2.0f);
		const char zero_char[1] = { '0' };
		dl.AddText({ pos.x, zero.y }, color_thick_lines, zero_char, zero_char+1);
		dl.AddText({ zero.x + 2, pos.y }, color_thick_lines, zero_char, zero_char+1);
	}

	void vertical_line (double norm_x) {
		const float x = screen_transform_x(norm_x);
		dl.AddLine({ x, pos.y }, { x, end.y }, color_interval, 2.0);
	}

	void function_graph (uint32_t color) {
		constexpr size_t steps = Nonlinear::Graph_display_cache::resolution;
		float x = pos.x;
		const float x_step = size.x / steps;
		for (size_t i = 1; i < steps; i++, x += x_step) {
			const float norm_y1 = nl.graph_cache.buffer[i-1];
			const float norm_y2 = nl.graph_cache.buffer[i];
			// Skip if both heights is offscreen
			if ((norm_y1 >= 0 && norm_y1 <= 1) || (norm_y2 >= 0 && norm_y1 <= 1))
				dl.AddLine({ x, size.y * norm_y1 }, { x + x_step, size.y * norm_y2 }, color, 2.0f);
		}
	}
};

void Nonlinear::graph_widget () const
{
	Graph_draw_context g(*this);
	g.gridlines_for_coordinate(X);
	g.gridlines_for_coordinate(Y);
	g.xy_axes();
	g.vertical_line(view_transform_x(seek_low));
	g.vertical_line(view_transform_x(seek_high));
	g.function_graph(0xFF'AA00FF);
}