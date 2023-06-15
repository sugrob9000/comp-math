#pragma once

#include <graph.hpp>
#include <math.hpp>
#include <task.hpp>
#include <vector>

class Diff_eq: public Task {
	enum class Method: int { euler_modified = 0, runge_kutta_4 = 1, milne = 2 };
	struct Method_spec {
		const char* name;
		unsigned precision_order;
	};
	static const Method_spec methods[];
	Method method = Method::milne;

	unsigned active_equation_id = 0;
	double low = 0;
	double high = 1;
	double precision = 0.1;
	double y_low = 0.3;

	struct Output {
		std::vector<dvec2> points;
		double step;
		bool reached_precision;
	};
	Output output;

	Graph graph { { -0.5, -0.1 }, { 1.5, 5 } };
	bool show_exact_solution = false;

	void update_calculation ();
	void settings_widget ();

public:
	Diff_eq () { update_calculation(); }
	void gui_frame () override;
	~Diff_eq () = default;
};