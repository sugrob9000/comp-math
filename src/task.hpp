#pragma once

#include "imcpp20.hpp"

struct Task {
	constexpr static auto static_window_flags
		= ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoBringToFrontOnFocus;

	// Called in a "global" context, i.e. outside any ImGui window
	virtual void gui_frame () = 0;

	virtual ~Task () = default;
};