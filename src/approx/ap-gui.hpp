#pragma once

#include <approx/calc.hpp>
#include <graph.hpp>
#include <imhelper.hpp>
#include <points-input.hpp>
#include <task.hpp>
#include <vector>

class Approx: public Task {
	enum class Method {
		find_best, linear, polynomial_2, polynomial_3, exponential, logarithmic, power,
	};
	Method method = Method::find_best;

	Points_input input{"vector.hpp"};

	struct Output {
		Method method; // cannot be find_best
		double a, b, c, d;
		double deviation;
		double correlation; // only valid when method is linear

		template <size_t N> void assign_coefs (std::array<double, N>) requires(N<=4);
		std::function<double(double)> get_function () const;
		void result_window () const;
	};
	Output last_output;
	static Output calculate (Method, std::span<const dvec2>);

	Graph graph { { -0.5, -0.5 }, { 3.5, 3.5 } };

	void settings_widget ();
	void update_calculation ();

public:
	Approx () { update_calculation(); }
	void gui_frame () override;
	~Approx () override = default;
};