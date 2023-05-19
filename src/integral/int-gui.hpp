#pragma once

#include "graph.hpp"
#include "integral/calc.hpp"
#include "task.hpp"

class Integration: public Task {
	unsigned active_function_id = 0;

	enum class Method { rect, trapezoid, simpson, };
	Method active_method = Method::rect;
	math::Rect_offset rect_offset = math::Rect_offset::left;

	math::Integration_result result;
	double exact_result;

	double low = -1, high = 1;
	unsigned subdivisions = 4;
	void update_calculation ();

	Graph graph;

	void settings_widget ();
	void result_window () const;
public:
	Integration () { update_calculation(); }
	void gui_frame () override;
	~Integration () override = default;
};