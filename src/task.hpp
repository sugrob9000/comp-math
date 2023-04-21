#pragma once

struct Task {
	// Called in a "global" context, i.e. outside any ImGui window
	virtual void gui_frame () = 0;

	virtual ~Task () = default;
};