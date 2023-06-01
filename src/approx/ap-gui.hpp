#pragma once

#include "approx/calc.hpp"
#include "graph.hpp"
#include "imhelper.hpp"
#include "task.hpp"
#include <vector>

class Approx: public Task {
	enum class Method { linear, polynomial_2, polynomial_3, exponential, logarithmic, power };
	Method method;

	struct Input {
		std::vector<dvec2> points;
		dvec2 new_point_input = {};

		enum class File_load_status { ok, unreadable, bad_data };
		void try_load_file ();
		File_load_status last_file_load_status = File_load_status::ok;
		constexpr static size_t path_buf_size = 256;
		char path_buf[path_buf_size] = "vectors";

		bool widget ();
	};
	Input input;

	struct Output {
		Method method;
		double a, b, c, d;
		template <size_t N> void assign_coefs (std::array<double, N>) requires(N<=4);
		std::function<double(double)> get_function () const;
		void result_window () const;
		double deviation;
		double correlation;
	};
	Output output;

	Graph graph { { -0.5, -0.5 }, { 3.5, 3.5 } };

	void settings_widget ();
	void update_calculation ();

public:
	Approx ();
	void gui_frame () override;
	~Approx () override = default;
};