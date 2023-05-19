#pragma once

#include "graph.hpp"
#include "math.hpp"
#include "nonlin-system/calc.hpp"
#include "task.hpp"
#include <optional>

class Nonlinear_system: public Task {
	unsigned active_function_id[2] = { 0, 1 };
	void update_calculation ();

	dvec2 initial_guess = { 1, 2 };
	double precision = 0.1;
	math::Newton_system_result result;

	Graph graph { { -6, -4 }, { 6, 4 } };

	void settings_widget ();
	void result_window () const;

public:
	Nonlinear_system () { update_calculation(); }
	void gui_frame () override;
	~Nonlinear_system () override = default;
};