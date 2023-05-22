#include "approx/ap-gui.hpp"
#include "gui.hpp"

using namespace ImGui;
using namespace ImScoped;

void Approx::gui_frame ()
{
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();

	Graph_draw_context draw(graph);
	draw.background();
}

void Approx::settings_widget ()
{
	TextUnformatted("Вид");
	graph.settings_widget();
}

void Approx::result_window () const
{

}