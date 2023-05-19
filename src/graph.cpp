#include "graph.hpp"
#include "imcpp20.hpp"
#include "math.hpp"
#include "util/util.hpp"
#include <glm/gtx/matrix_transform_2d.hpp>

using namespace ImGui;
using namespace ImScoped;

dmat3 Graph::get_transform () const
{ // Reverse order of operations
	const dmat3 scaled = glm::scale(dmat3(1), dvec2(1, -1)/(view_high - view_low));
	return glm::translate(scaled, dvec2(-view_low.x, -view_high.y));
}

void Graph::settings_widget ()
{
	constexpr float drag_speed = 0.03;

	const dvec2 center = (view_low + view_high) * 0.5;
	if (dvec2 delta = center; DragN("Центр", glm::value_ptr(delta), 2, drag_speed)) {
		delta -= center;
		view_low += delta;
		view_high += delta;
	}

	const dvec2 scale = (view_high - view_low);
	constexpr double min_scale = 0.5;
	constexpr double max_scale = 250;
	dvec2 rescale = scale;
	if (DragN("Масштаб", glm::value_ptr(rescale), 2, drag_speed, min_scale, max_scale)) {
		rescale /= scale;
		view_low = center + (view_low - center) * rescale;
		view_high = center + (view_high - center) * rescale;
	}
}

static mat3 get_view_to_screen (vec2 low, vec2 size)
{ // Reverse order of operations
	const mat3 translation = glm::translate(mat3(1), low);
	return glm::scale(translation, size);
}


Graph_draw_context::Graph_draw_context
(const Graph& graph_, ImDrawList& drawlist_, ImVec2 low_, ImVec2 size_)
	: graph{ graph_ }, drawlist{ drawlist_ },
	low{ low_ }, size{ size_ }, high{ low.x + size.x, low.y + size.y },
	world_screen_transform{
		get_view_to_screen(vec2(low.x, low.y), vec2(size.x, size.y)) *
		static_cast<mat3>(graph.get_transform())
	}
{}

Graph_draw_context::Graph_draw_context (const Graph& graph_)
	: Graph_draw_context(graph_, *GetBackgroundDrawList(),
			GetMainViewport()->WorkPos, GetMainViewport()->WorkSize) {}


constexpr static uint32_t color_thick_lines = 0xFF000000;
constexpr static uint32_t color_thin_lines = 0xFFAAAAAA;

// World->screen transform for single coordinate
double Graph_draw_context::world_to_screen (int coord, double x, bool translate) const
{
	vec3 v = { 0, 0, translate ? 1 : 0 };
	v[coord] = x;
	return (world_screen_transform * v)[coord];
}

constexpr static int ideal_num_gridlines = 20;
constexpr static double gridline_steps[] = {
	0.01, 0.05, 0.1, 0.25, 0.5, 1, 2, 5, 10, 25, 50, 100
};

ImVec2 Graph_draw_context::ortho_line (int coord, double x, uint32_t color, float thickness)
{
	ImVec2 begin = low;
	ImVec2 end = high;
	begin[coord] = end[coord] = world_to_screen(coord, x, true);
	drawlist.AddLine(begin, end, color, thickness);
	return begin;
}

void Graph_draw_context::line (dvec2 a, dvec2 b, uint32_t color, float thickness)
{
	const vec2 aa = (world_screen_transform * dvec3(a, 1));
	const vec2 bb = (world_screen_transform * dvec3(b, 1));
	drawlist.AddLine({ aa.x, aa.y }, { bb.x, bb.y }, color, thickness);
}

void Graph_draw_context::dot (dvec2 center, uint32_t color)
{
	const vec2 c = (world_screen_transform * dvec3(center, 1));
	drawlist.AddCircleFilled({ c.x, c.y }, 5.0, color);
}

void Graph_draw_context::background ()
{
	const vec3 screen_zero = world_screen_transform * vec3(0, 0, 1);
	const char zero_char[1] = { '0' };

	for (int coord: { 0, 1 }) {
		{ // Draw main axis
			ImVec2 pos = ortho_line(coord, 0, color_thick_lines, 3.0f);
			drawlist.AddText({ pos.x + 2, pos.y }, color_thick_lines, zero_char, zero_char+1);
		}

		const double vl = graph.view_low[coord];
		const double vh = graph.view_high[coord];

		// Choose the grid step
		double step = gridline_steps[std::size(gridline_steps)-1];
		for (const double min_step = (vh - vl) / ideal_num_gridlines;
				double candidate: gridline_steps) {
			if (min_step <= candidate) {
				step = candidate;
				break;
			}
		}

		{ // If step will be too small anyway, don't bother
			constexpr int min_step_pixels = 5;
			const double num_steps = std::abs(world_to_screen(coord, step, false));
			if (num_steps * min_step_pixels < (vh - vl))
				continue;
		}

		for (double x = step * std::ceil(vl / step); x < vh; x += step) {
			if (std::abs(x) < step * 0.5) continue; // Don't duplicate the main axes
			ImVec2 pos = ortho_line(coord, x, color_thin_lines, 1.0f);
			char buf[10];
			char* buf_end = fmt::format_to_n(buf, std::size(buf), "{:.3}", x).out;
			drawlist.AddText(pos, color_thin_lines, buf, buf_end);
		}
	}
}

void Graph_draw_context::function_plot (uint32_t color, double (*f) (double))
{
	constexpr int num_segments = 100;
	const dvec2& vl = graph.view_low;
	const dvec2& vh = graph.view_high;
	const double step = (vh.x - vl.x) / num_segments;

	vec2 prev = world_screen_transform * vec3(vl.x, f(vl.x), 1);

	for (int i = 1; i <= num_segments; i++) {
		const double x = vl.x + i * step;
		const double y = f(x);
		vec2 cur = world_screen_transform * vec3(x, y, 1);
		drawlist.AddLine({ prev.x, prev.y }, { cur.x, cur.y }, color, 3.0f);
		prev = cur;
	}
}