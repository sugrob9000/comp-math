#include "gui.hpp"
#include "util/util.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <fstream>
#include <glm/glm.hpp>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#include <memory>
#include <optional>

using Resolution = glm::vec<2, int>;

namespace gui {
namespace {
struct Context {
	Resolution resolution;
	SDL_Window* window;
	SDL_GLContext gl_context;
	ImGuiIO* im_io;

	constexpr static const char window_title[] = "Вариант 31";

	explicit Context (Resolution res): resolution{res} {
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			FATAL("Failed to initialize SDL: {}", SDL_GetError());

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

		window = SDL_CreateWindow(window_title,
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				resolution.x, resolution.y,
				SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		if (!window)
			FATAL("Failed to create SDL window: {}", SDL_GetError());

		set_icon("icon");

		SDL_GL_SetSwapInterval(0);

		gl_context = SDL_GL_CreateContext(window);
		if (!gl_context)
			FATAL("Failed to create SDL GL context: {}", SDL_GetError());

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		im_io = &ImGui::GetIO();

		constexpr float font_size = 20;
		auto fonts = im_io->Fonts;
		fonts->AddFontFromFileTTF("DejaVuSansMono.ttf", font_size, nullptr,
				fonts->GetGlyphRangesCyrillic());

		// we don't use a steady framerate anyway
		im_io->ConfigInputTextCursorBlink = false;

		ImGui::StyleColorsLight();
		ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init();
	}

	~Context () {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();

		SDL_GL_DeleteContext(gl_context);
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
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void end_frame ()
{
	const auto& ctx = *global_context;
	glViewport(0, 0, ctx.resolution.x, ctx.resolution.y);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(ctx.window);
}
} // namespace gui
