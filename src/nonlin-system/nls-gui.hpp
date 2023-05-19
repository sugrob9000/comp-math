#pragma once

#include "graph.hpp"
#include "math.hpp"
#include "nonlin-system/calc.hpp"
#include "task.hpp"
#include <optional>

class Nonlinear_system: public Task {
	int active_function_id[2] = { 0, 1 };
	void update_calculation ();
	bool calculation_enabled () const;

	dvec2 initial_guess = { 1, 1 };
	double precision = 0.1;
	std::optional<math::Newton_system_result> result;

	Graph graph;

	void settings_widget ();

public:
	void gui_frame () override;
	~Nonlinear_system () override = default;
};