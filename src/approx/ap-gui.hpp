#pragma once

#include "graph.hpp"
#include "imcpp20.hpp"
#include "task.hpp"

class Approx: public Task {
	Graph graph;

	void result_window () const;
	void settings_widget ();
public:
	void gui_frame () override;
	~Approx () override = default;
};
