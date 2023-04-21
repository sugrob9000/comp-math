#pragma once

#include "task.hpp"
#include <glm/vec2.hpp>
#include <vector>

using Dvec2 = glm::vec<2, double, glm::packed>;
using Fvec2 = glm::vec<2, float, glm::packed>;

// Single non-linear equation solver
class Nonlinear: public Task {
	enum class Method { chords, newton, iteration, };
	Method active_method;
	unsigned active_function_id = 0;

	double precision = 1e-3;

	Dvec2 view_low { -1, -1 };
	Dvec2 view_high { 1, 1 };

	enum Coordinate: unsigned { X = 0, Y = 1 };
	double view_transform_x (double x) const;
	double view_transform_y (double y) const;
	double view_transform_i (Coordinate i, double coord) const;
	Dvec2 view_transform (Dvec2 v) const;

	double seek_low = -1;
	double seek_high = 1;

	struct Graph_display_cache {
		constexpr static size_t resolution = 200;
		float buffer[resolution];
		unsigned function_id = -1;
		Dvec2 view_low, view_high;
	};
	Graph_display_cache graph_cache;
	void update_graph_cache ();

	void settings_widget ();
	void graph_widget () const;

	friend struct Graph_draw_context;

public:
	void gui_frame () override;
	~Nonlinear () override = default;
};