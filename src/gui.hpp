#pragma once

#include <SDL2/SDL.h>
#include <imgui/imgui.h>
#include <string_view>
#include <cstdint>

namespace gui {
// =================================== GUI lifecycle ===================================

void init (int res_x, int res_y);
void deinit ();

enum class Event_process_result: bool { consumed, passthrough };
Event_process_result process_event (const SDL_Event& event);

void begin_frame ();
void end_frame ();

// =============================== Convenience constants ===============================
constexpr inline ImGuiWindowFlags floating_window_flags
= ImGuiWindowFlags_AlwaysAutoResize;

constexpr inline ImGuiWindowFlags fullscreen_window_flags
= ImGuiWindowFlags_NoCollapse
| ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoMove
| ImGuiWindowFlags_NoSavedSettings
| ImGuiWindowFlags_NoTitleBar
| ImGuiWindowFlags_NoBringToFrontOnFocus;

} // namespace gui