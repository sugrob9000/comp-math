#include "gui.hpp"
#include "util/util.hpp"
#include <SDL2/SDL.h>
#include <fstream>
#include <glm/glm.hpp>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_sdlrenderer.h>
#include <memory>
#include <optional>

using Resolution = glm::vec<2, int>;

namespace gui {
namespace {
struct Context {
	Resolution resolution;
	SDL_Window* window;
	SDL_Renderer* renderer;
	ImGuiIO* im_io;

	constexpr static const char window_title[] = "Вариант 31";

	explicit Context (Resolution res): resolution{res} {
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			FATAL("Failed to initialize SDL: {}", SDL_GetError());

		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		window = SDL_CreateWindow(window_title,
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				resolution.x, resolution.y,
				SDL_WINDOW_RESIZABLE);
		if (!window)
			FATAL("Failed to create SDL window: {}", SDL_GetError());

		renderer = SDL_CreateRenderer(window, -1, 0);
		if (!renderer)
			FATAL("Failed to create SDL renderer: {}", SDL_GetError());

		set_icon("icon");

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		im_io = &ImGui::GetIO();

		{
			constexpr float font_size = 20;
			auto fonts = im_io->Fonts;

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(fonts->GetGlyphRangesCyrillic());
			builder.AddRanges(fonts->GetGlyphRangesGreek());
			ImVector<ImWchar> ranges;
			builder.BuildRanges(&ranges);

			fonts->AddFontFromFileTTF("DejaVuSansMono.ttf", font_size, nullptr,
					ranges.begin());
			fonts->Build();
		}

		// we don't use a steady framerate anyway
		im_io->ConfigInputTextCursorBlink = false;

		ImGui::StyleColorsLight();
		ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
		ImGui_ImplSDLRenderer_Init(renderer);
	}

	~Context () {
		ImGui_ImplSDLRenderer_Shutdown();
		ImGui_ImplSDL2_Shutdown();

		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void resize_view (Resolution new_res) { this->resolution = new_res; }

	// Returns: whether the event was consumed (meaning no further processing should happen)
	bool process_event (const SDL_Event& event) {
		ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type) {
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			return im_io->WantCaptureKeyboard;

		case SDL_MOUSEWHEEL:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEMOTION:
			return im_io->WantCaptureMouse;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				resize_view({ event.window.data1, event.window.data2 });
			break;
		}

		return false;
	}

	void set_icon (const char* filename) {
		std::ifstream f(filename, std::ios::binary);
		if (!f) {
			WARNING("Cannot open icon '{}'", filename);
			return;
		}

		constexpr int bytes_per_pixel = 3;
		constexpr int side = 128;
		constexpr int bytes = side * side * bytes_per_pixel;
		auto pixels = std::make_unique<char[]>(bytes);
		if (!f.read(pixels.get(), bytes)) {
			WARNING("Cannot read {} bytes from '{}'", bytes, filename);
			return;
		}
		f.close();

		auto surf = SDL_CreateRGBSurfaceFrom(pixels.get(),
				side, side, 8*bytes_per_pixel, side*bytes_per_pixel,
				0x00FF'0000, 0x0000'FF00, 0x0000'00FF, 0);
		SDL_SetColorKey(surf, SDL_TRUE, 0x00FF'00FF);
		SDL_SetWindowIcon(window, surf);
		SDL_FreeSurface(surf);
	}
};
} // anon namespace

static std::optional<Context> global_context;

// ================================== Public interface ==================================

void init (int res_x, int res_y)
{
	assert(!global_context.has_value());
	global_context.emplace(Resolution{ res_x, res_y });
}

void deinit ()
{
	assert(global_context.has_value());
	global_context.reset();
}

Event_process_result process_event (const SDL_Event& event)
{
	return global_context->process_event(event)
		? Event_process_result::consumed
		: Event_process_result::passthrough;
}

void begin_frame ()
{
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	SDL_Renderer* renderer = global_context->renderer;
	SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
	SDL_RenderClear(renderer);
}

void end_frame ()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
	SDL_RenderPresent(global_context->renderer);
}
} // namespace gui