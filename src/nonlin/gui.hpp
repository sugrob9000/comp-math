#pragma once

#include "math.hpp"
#include "nonlin/calc.hpp"
#include "task.hpp"
#include <variant>
#include <vector>

namespace math {
struct Chords_result { std::vector<math::Chord> chords; };
struct Newton_result {};
struct Iteration_result {};
using Calculation = std::variant<
	std::monostate,
	Chords_result,
	Newton_result,
	Iteration_result>;
};

// Single non-linear equation solver
class Nonlinear: public Task {
	math::Calculation calculation;
	void update_calculation ();
	template <typename T> void method_option_widget (const char* name);

	unsigned active_function_id = 0;

	math::Dvec2 view_low { -1.2, -1 };
	math::Dvec2 view_high { 1.2, 1 };

	enum Coordinate: unsigned { X = 0, Y = 1 };
	double view_transform_x (double x) const;
	double view_transform_y (double y) const;
	double view_transform_i (Coordinate i, double coord) const;
	math::Dvec2 view_transform (math::Dvec2 v) const;

	double seek_low = 0;
	double seek_high = 1;
	double precision = 1e-3;

	struct Graph_display_cache {
		constexpr static size_t resolution = 200;
		float buffer[resolution];
		unsigned function_id = -1;
		math::Dvec2 view_low, view_high;
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