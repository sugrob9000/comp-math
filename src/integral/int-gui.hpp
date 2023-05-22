#pragma once

#include "graph.hpp"
#include "integral/calc.hpp"
#include "task.hpp"

class Integration: public Task {
	unsigned active_function_id = 0;

	enum class Method { rect = 0, trapezoid = 1, simpson = 2, };
	Method active_method = Method::rect;
	math::Rect_offset rect_offset = math::Rect_offset::middle;

	constexpr static double min_precision = 1e-6, max_precision = 1e-1;
	double precision = 0.01;
	double low = -0.5, high = 1.3;

	constexpr static unsigned min_subdivisions = 2, max_subdivisions = 1024;
	struct Result {
		double calculated = 0.0;
		double exact = 0.0;
		bool diverges = false;
		unsigned subdivisions = min_subdivisions;
	};
	Result result;

	void update_calculation ();
	double integrate_once (unsigned subdivisions) const;

	Graph graph;

	void settings_widget ();
	void result_window () const;
	void result_visualization () const;

public:
	Integration () { update_calculation(); }
	void gui_frame () override;
	~Integration () override = default;
};