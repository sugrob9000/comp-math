#pragma once

#include "graph.hpp"
#include "integral/calc.hpp"
#include "task.hpp"

class Integration: public Task {
	unsigned active_function_id = 0;

	enum class Method { rect = 0, trapezoid = 1, simpson = 2, };
	Method active_method = Method::rect;
	math::Rect_offset rect_offset = math::Rect_offset::middle;

	double calculated_result;
	double exact_result;

	double low = -0.5, high = 1.3;

	constexpr static unsigned min_subdivisions = 1 << 2, max_subdivisions = 1 << 10;
	constexpr static double min_precision = 1e-6, max_precision = 1e-1;

	enum class Precision_spec { by_precision, by_num_subdivisions };
	Precision_spec precision_spec = Precision_spec::by_precision;
	unsigned subdivisions = min_subdivisions;
	double precision = 0.01;

	void update_calculation ();
	double integrate_once_with_current_settings () const;

	Graph graph;

	void settings_widget ();
	void result_window () const;
	void result_visualization () const;
public:
	Integration () { update_calculation(); }
	void gui_frame () override;
	~Integration () override = default;
};