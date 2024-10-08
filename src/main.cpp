#include <approx/ap-gui.hpp>
#include <chrono>
#include <gauss/g-gui.hpp>
#include <diffeq/de-gui.hpp>
#include <gui.hpp>
#include <imhelper.hpp>
#include <integral/int-gui.hpp>
#include <interp/it-gui.hpp>
#include <nonlin-system/nls-gui.hpp>
#include <nonlin/nl-gui.hpp>
#include <task.hpp>
#include <thread>

int main ()
{
	gui::init(1280, 760);
	std::unique_ptr<Task> task;

	// Draw at least this many frames at a steady rate after an event
	constexpr int max_frames_since_event = 2;
	int frames_since_event = 0;

	using clock = std::chrono::steady_clock;
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

			if (task) {
				// Show current task
				bool to_main_menu = false;
				if (auto bar = ImScoped::MainMenuBar()) {
					to_main_menu |= ImGui::SmallButton("Главное меню");
					should_quit |= ImGui::SmallButton("Выйти");
				}
				task->gui_frame();
				if (to_main_menu)
					task.reset();
			} else {
				// Show main menu with task selection
				if (auto window = ImScoped::Window("вариант 31", nullptr,
						ImGuiWindowFlags_AlwaysAutoResize)) {
					if (ImGui::Button("Метод Гаусса")) task.reset(new Gauss);
					if (ImGui::Button("Решение нелинейного уравнения")) task.reset(new Nonlinear);
					if (ImGui::Button("Метод Ньютона для нелинейных систем"))
						task.reset(new Nonlinear_system);
					if (ImGui::Button("Численное интегрирование")) task.reset(new Integration);
					if (ImGui::Button("Аппроксимация")) task.reset(new Approx);
					if (ImGui::Button("Интерполяция")) task.reset(new Interp);
					if (ImGui::Button("Дифференциальные уравнения")) task.reset(new Diff_eq);
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