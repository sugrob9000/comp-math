#pragma once

#include <functional>
#include <graph.hpp>
#include <interp/calc.hpp>
#include <math.hpp>
#include <points-input.hpp>
#include <task.hpp>
#include <vector>

class Interp: public Task {
	enum class Method { lagrange, newton };
	Method method = Method::newton;

	struct Input {
		Points_input points{"vectors"}; // only for Lagrange method
		struct Evenly_spaced {
			double low = 0, high = 4;

			enum class Input_method { values, sample_function, };
			Input_method input_method = Input_method::sample_function;
			unsigned sampled_function_id = 0; // only for `input_method == sample_function`

			std::vector<double> values = { 1, 1.67, 0.99, 0, 0 };

			double get_step () const { return (high - low) / (values.size() - 1); };
			bool widget ();
		} evenly_spaced; // only for Newton method
	};
	Input input;

	double approx_x = 1.0;

	struct Output {
		Method method;
		std::function<double(double)> function;
		double approx_x;
		double approx_value;
		math::Finite_differences diff; // only for Newton method

		void result_window () const;
	};
	Output output;

	Graph graph { { -5, -1 }, { 5, 5 } };

	void settings_widget ();
	void update_calculation ();

public:
	Interp () { update_calculation(); }

	void gui_frame () override;
	~Interp () override = default;
};