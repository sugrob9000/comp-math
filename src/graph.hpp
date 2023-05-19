#pragma once

#include "imcpp20.hpp"
#include "math.hpp"

class Graph {
	dvec2 view_low { -2, -2 };
	dvec2 view_high { 2, 2 };

	dmat3 get_transform () const;

	friend class Graph_draw_context;
public:
	Graph () = default;
	Graph (dvec2 low, dvec2 high): view_low{ low }, view_high{ high } {}

	void settings_widget ();
};


class Graph_draw_context {
	const Graph& graph;
	ImDrawList& drawlist;
	ImVec2 low, size, high;
	mat3 world_screen_transform;

	double world_to_screen (int coord, double x, bool translate) const;

public:
	// All inputs are in world space
	Graph_draw_context (const Graph&); // Uses background drawlist and main viewport
	Graph_draw_context (const Graph&, ImDrawList&, ImVec2 low, ImVec2 size);

	void background ();
	void function_plot (uint32_t color, double (*) (double));
	void parametric_plot (uint32_t color, dvec2 (*) (double t), double t_low, double t_high);

	// Returns the top or left point of the line, in pixel coordinates
	ImVec2 ortho_line (int coord, double x, uint32_t color, float thick);
	ImVec2 vert_line (double x, uint32_t color, float thick) { return ortho_line(0, x, color, thick); }
	ImVec2 horz_line (double y, uint32_t color, float thick) { return ortho_line(1, y, color, thick); }

	void line (dvec2 a, dvec2 b, uint32_t color, float thickness);
	void dot (dvec2, uint32_t color);

	void rect (dvec2 a, dvec2 b, uint32_t color_border, uint32_t color_fill);
	void trapezoid (double x1, double x2, double y0, double y1, double y2,
			uint32_t color_border, uint32_t color_fill);
};