#pragma once

#include <graph.hpp>
#include <math.hpp>
#include <nonlin/calc.hpp>
#include <task.hpp>
#include <variant>

// Single non-linear equation solver
// TODO: the variant-based method choice is really unpleasant, should do something else
class Nonlinear: public Task {
	using Calculation = std::variant<
		std::monostate,
		math::Chords_result,
		math::Newton_result,
		math::Iteration_result>;
	Calculation calculation;
	void update_calculation ();
	template <typename T> void method_option_widget (const char* name);

	template <typename... Ops> void visit_calculation (Ops&&...);
	template <typename... Ops> void visit_calculation (Ops&&...) const;

	template <typename T> bool chosen_method_is () const;
	bool no_chosen_method () const;

	unsigned active_function_id = 0;

	double seek_low = 0.1;
	double seek_high = 1;
	double initial_guess = 0.5;
	double precision = 1e-3;
	double lambda = 1;  // for use with fixed-point iteration

	Graph graph;

	void settings_widget ();
	void maybe_result_window () const;

public:
	void gui_frame () override;
	~Nonlinear () override = default;
};