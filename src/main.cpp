#include "gauss_gui.hpp"
#include "gui.hpp"
#include "imcpp20.hpp"
#include <chrono>
#include <thread>

int main ()
{
	gui::init(1280, 760);

	gauss::Input input;
	std::optional<gauss::Output> output;

	using clock = std::chrono::steady_clock;

	// Draw at least this many frames at a steady rate after an event
	constexpr int max_frames_since_event = 2;
	int frames_since_event = 0;

	constexpr auto target_frame_time = std::chrono::microseconds{ 1000'000 / 60 };
	clock::time_point next_frame_time;

	bool show_demo = false;

	bool should_quit = false;
	SDL_Event event;
	do {
		const bool got_events = frames_since_event > max_frames_since_event
			? SDL_WaitEvent(&event)
			: SDL_PollEvent(&event);

		if (got_events) {
			{ // Mark frame as eventful
				frames_since_event = 0;
				next_frame_time = clock::now();
			}
			do {
				// Process some events no matter what...
				if (event.type == SDL_QUIT
				|| (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE))
					should_quit = true;

				// ...others unless GUI consumes them
				if (gui::process_event(event) == gui::Event_process_result::passthrough
				&& event.type == SDL_KEYDOWN) {
					const auto& sym = event.key.keysym;

					if (sym.scancode == SDL_SCANCODE_D && (sym.mod & KMOD_SHIFT))
						asm("int3":::);

					if (sym.scancode == SDL_SCANCODE_SLASH && (sym.mod & KMOD_SHIFT))
						show_demo = true;

					if (sym.scancode == SDL_SCANCODE_Q && (sym.mod & KMOD_SHIFT))
						should_quit = true;
				}
			} while (SDL_PollEvent(&event));
		}

		{ // Draw GUI frame
			gui::begin_frame();

			if (show_demo)
				ImGui::ShowDemoWindow(&show_demo);

			constexpr auto window_flags
				= ImGuiWindowFlags_NoCollapse
				| ImGuiWindowFlags_NoResize
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoBringToFrontOnFocus;

			const auto& viewport = ImGui::GetMainViewport();
			const float x = viewport->WorkPos.x;
			const float y = viewport->WorkPos.y;
			const float width = viewport->WorkSize.x;
			const float height = viewport->WorkSize.y;

			ImGui::SetNextWindowPos({ x, y });
			ImGui::SetNextWindowSize({ width / 2, height });
			if (auto w = ImScoped::Window("Ввод", nullptr, window_flags))
				input.widget();

			ImGui::SetNextWindowPos({ x + width / 2, y });
			ImGui::SetNextWindowSize({ width / 2, height });
			if (auto w = ImScoped::Window("Вывод", nullptr, window_flags)) {
				if (ImGui::Button("Вычислить"))
					output.emplace(input);
				if (output) {
					ImGui::SameLine();
					if (ImGui::Button("Сбросить"))
						output.reset();
					else
						output->widget();
				}
			}

			gui::end_frame();
		}

		{ // Advance frame bookkeeping
			frames_since_event++;
			next_frame_time += target_frame_time;
			std::this_thread::sleep_until(next_frame_time);
		}
	} while (!should_quit);

	gui::deinit();
}
