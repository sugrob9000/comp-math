#include <algorithm>
#include <fstream>
#include <gui.hpp>
#include <imhelper.hpp>
#include <points-input.hpp>
#include <util/util.hpp>

using namespace ImGui;
using namespace ImScoped;

bool Points_input::widget ()
{
	bool dirty = false;

	switch (last_file_load_status) {
	case File_load_status::ok:
		break;
	case File_load_status::unreadable:
		TextColored(gui::error_text_color, "Не удалось прочитать файл");
		break;
	case File_load_status::bad_data:
		TextColored(gui::error_text_color, "В файле не численные данные");
		break;
	}

	bool do_load_file = InputText("Имя файла", path_buf, std::size(path_buf),
			ImGuiInputTextFlags_EnterReturnsTrue);
	do_load_file |= SmallButton("Загрузить");
	if (do_load_file) {
		try_load_file();
		if (last_file_load_status == File_load_status::ok)
			dirty = true;
	}

	if (SmallButton("Нормализовать")) {
		std::ranges::sort(points, {}, &dvec2::x);
		points.erase(std::unique(points.begin(), points.end()), points.end());
		dirty = true;
	}

	constexpr float drag_speed = 0.03;
	int removed = -1;

	{
		ImScoped::StyleColor colors {
			{ ImGuiCol_Button, 0xFF'9999FF },
			{ ImGuiCol_ButtonHovered, 0xFF'7777FF },
			{ ImGuiCol_ButtonActive, 0xFF'4444FF }
		};
		for (size_t i = 0; i < points.size(); i++) {
			ImScoped::ID id(i);
			dirty |= DragN("##add", glm::value_ptr(points[i]), 2, drag_speed);
			SameLine();
			if (SmallButton("x"))
				removed = i;
		}
	}

	if (removed != -1) {
		points.erase(points.begin() + removed);
		dirty = true;
	}

	Separator();
	DragN("##add", glm::value_ptr(new_point_input), 2, drag_speed);
	SameLine();
	if (SmallButton("+")) {
		points.push_back(new_point_input);
		dirty = true;
	}

	return dirty;
}

void Points_input::try_load_file ()
{
	last_file_load_status = [this] {
		std::ifstream file(path_buf);
		if (!file)
			return File_load_status::unreadable;

		std::vector<dvec2> temp;

		for (dvec2 v; file >> v.x >> v.y; )
			temp.push_back(v);

		if (!file.eof())
			return File_load_status::bad_data;

		points = temp;
		return File_load_status::ok;
	} ();
}

Points_input::Points_input (std::string_view initial_path)
{
	const size_t n = initial_path.size();

	if (n + 1 > path_buf_size) FATAL("Points_input: initial path too long!");

	std::copy_n(initial_path.begin(), n, path_buf);
	path_buf[n] = '\0';
	try_load_file();
}