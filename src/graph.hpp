#pragma once

#include "imcpp20.hpp"
#include "math.hpp"

class Graph {
	dvec2 view_low { -2, -2 };
	dvec2 view_high { 2, 2 };

	dmat3 get_transform () const;

	friend class Graph_draw_context;
public:
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

	// Returns the top or left point of the line, in pixel coordinates
	ImVec2 ortho_line (int coord, double x, uint32_t color, float thickness);

	void line (dvec2 a, dvec2 b, uint32_t color, float thickness);
	void dot (dvec2, uint32_t color);
};