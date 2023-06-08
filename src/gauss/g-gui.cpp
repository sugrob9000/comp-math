#include <fstream>
#include <gauss/g-gui.hpp>
#include <gauss/solve.hpp>
#include <gui.hpp>
#include <imhelper.hpp>
#include <util/util.hpp>

using namespace ImGui;
using namespace ImScoped;

constexpr static ImGuiWindowFlags static_window_flags
	= ImGuiWindowFlags_NoCollapse
	| ImGuiWindowFlags_NoResize
	| ImGuiWindowFlags_NoMove
	| ImGuiWindowFlags_NoSavedSettings
	| ImGuiWindowFlags_NoBringToFrontOnFocus;

/*
 * Create a table which uses some fraction of the remaining window height
 * e.g., factor = 0.5: use up half the remaining window height
 *       factor = 1.0: use up the entire remaining height
 */
static auto matrix_table (const char* label, unsigned columns, float factor)
{
	return Table(label, columns,
			ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
			ImVec2(0, (1.0f - factor) * GetContentRegionAvail().y));
}

// ======================================= Input =======================================

void Gauss::Input::widget ()
{
	if (auto bar = TabBar("tabs")) {
		if (auto item = TabItem("Загрузить из файла")) {
			bool do_load_file = InputText("Имя файла", path_buf, std::size(path_buf),
					ImGuiInputTextFlags_EnterReturnsTrue);
			do_load_file |= Button("Загрузить");
			if (do_load_file)
				last_file_load_status = load_from_file(path_buf);

			switch (last_file_load_status) {
			case File_load_status::ok:
				break;
			case File_load_status::unreadable:
				TextColored(gui::error_text_color, "Не удалось прочитать файл");
				break;
			case File_load_status::bad_dimensions:
				TextColored(gui::error_text_color, "Некорректные размерности матрицы в файле");
				break;
			case File_load_status::bad_data:
				TextColored(gui::error_text_color, "В файле не численные данные");
				break;
			}
		}

		if (auto item = TabItem("Ввести матрицу")) {
			Slider("строк",    &rows, 1u, max_rows, nullptr, ImGuiSliderFlags_AlwaysClamp);
			Slider("столбцов", &cols, 2u, max_cols, nullptr, ImGuiSliderFlags_AlwaysClamp);

			if (auto table = matrix_table("input", cols, 0.5)) {
				constexpr float col_width = 100;
				for (unsigned col = 0; col < num_variables(); col++) {
					TableSetupColumn(fmt::format(FMT_STRING("X{}"), col+1).c_str(),
							ImGuiTableColumnFlags_WidthFixed, col_width);
				}
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, col_width);
				BeginDisabled();
				TableHeadersRow();
				EndDisabled();

				auto mat = view();
				for (unsigned row = 0; row < rows; row++) {
					TableNextRow();
					for (unsigned col = 0; col < cols; col++) {
						TableNextColumn();
						InputNumber(GenerateId(row, col), &mat[row][col]);
					}
				}
			}
		}
	}

	SeparatorText("Уравнения");
	if (auto table = matrix_table("equations", cols, 1.0)) {
		auto mat = view();
		for (unsigned row = 0; row < rows; row++) {
			TableNextRow();
			bool first_term = true;
			for (unsigned col = 0; col+1 < cols; col++) {
				TableNextColumn();
				Number value = mat[row][col];
				if (value == 0)
					continue;
				char sign = (value > 0) ? '+' : '-';
				if (sign == '+' && first_term) {
					sign = ' ';
					first_term = false;
				}
				TextFmt(FMT_STRING("{} {}·X{} "), sign, std::abs(value), col+1);
			}
			TableNextColumn();
			TextFmt(FMT_STRING("{} = {}"), first_term ? "0" : "", mat[row][cols-1]);
		}
	}
}

auto Gauss::Input::load_from_file (const char* filename) -> File_load_status
{
	std::ifstream file(filename);
	if (!file)
		return File_load_status::unreadable;

	Sized_static_matrix temp;
	if (!(file >> temp.rows >> temp.cols))
		return File_load_status::bad_data;
	if (temp.rows < 1 || temp.rows > max_rows || temp.cols < 2 || temp.cols > max_cols)
	 	return File_load_status::bad_dimensions;

	for (unsigned row = 0; row < temp.rows; row++) {
		for (unsigned col = 0; col < temp.cols; col++) {
			if (!(file >> temp.view()[row][col]))
				return File_load_status::bad_data;
		}
	}

	this->Sized_static_matrix::operator=(temp);
	return File_load_status::ok;
}

// ======================================= Output =======================================

Gauss::Output::Output (const Input& in): Sized_static_matrix(in.rows, in.cols)
{
	// gauss_gather() gives the solution in a meaningless "raw" pre-permutation order
	auto raw_solution = std::make_unique<Number[]>(num_variables());
	std::span raw_solution_span { raw_solution.get(), size_t(num_variables()) };

	permutations = math::gauss_triangulate(view(), in.view(), permute_span());
	num_indeterminate_variables = math::gauss_gather(raw_solution_span, view());

	auto variables_view = view().subview(0, 0, num_equations(), num_variables());

	if (num_equations() == num_variables()) {
		determinant = math::triangular_determinant(variables_view);
		if (permutations % 2 == 1)
			determinant = -determinant;
	}

	if (num_indeterminate_variables == 0) {
		mul_matrix_vector(mismatch_span(), variables_view, raw_solution_span);
		for (unsigned i = 0; i < num_equations(); i++)
			mismatch[i] -= view()[i][cols-1];
		for (unsigned i = 0; i < num_variables(); i++)
			solution[permute[i]] = raw_solution[i];
	}
}

void Gauss::Output::widget () const
{
	bool triangulation_error = false;

	SeparatorText("Треугольный вид матрицы:");
	if (auto table = matrix_table("output", cols, 0.5)) {
		auto mat = view();

		TableNextRow();
		BeginDisabled();
		for (unsigned col = 0; col < num_variables(); col++) {
			TableNextColumn();
			TextFmt("X{}", permute[col]+1);
		}
		EndDisabled();

		for (unsigned row = 0; row < rows; row++) {
			TableNextRow();
			const unsigned diag = std::min(cols, row);

			for (unsigned col = 0; col < diag; col++) {
				TableNextColumn();
				if (mat[row][col] != 0) {
					TextFmtColored(gui::error_text_color, FMT_STRING("{}"), mat[row][col]);
					triangulation_error = true;
				}
			}

			for (unsigned col = diag; col < cols; col++) {
				TableNextColumn();
				TextFmt(FMT_STRING("{}"), mat[row][col]);
			}
		}
	}

	if (triangulation_error)
		TextColored(gui::error_text_color, "Ошибка при триангуляции. Матрица не треугольная.");

	Separator();

	if (num_equations() == num_variables())
		TextFmt(FMT_STRING("Определитель подматрицы коэффициентов: {}"), determinant);
	else
		TextWrapped("Подматрица коэффициентов не квадратная. Определитель не имеет смысла.");

	if (num_indeterminate_variables < 0) {
		TextUnformatted("Система несовместна.");
	} else if (num_indeterminate_variables > 0) {
		TextFmtWrapped(
				FMT_STRING("Бесконечное количество решений: как минимум {} независимых переменных."),
				num_indeterminate_variables);
	} else {
		TextFmtWrapped(FMT_STRING("Решение: {:.5}"), fmt::join(solution_span(), ", "));
		TextFmtWrapped(FMT_STRING("Невязка: {:.7}"), fmt::join(mismatch_span(), ", "));
	}
}

void Gauss::gui_frame ()
{
	const auto viewport = ImGui::GetMainViewport();
	const float x = viewport->WorkPos.x;
	const float y = viewport->WorkPos.y;
	const float width = viewport->WorkSize.x;
	const float height = viewport->WorkSize.y;

	ImGui::SetNextWindowPos({ x, y });
	ImGui::SetNextWindowSize({ width / 2, height });
	if (auto w = ImScoped::Window("Ввод (вариант 31)", nullptr, static_window_flags))
		input.widget();

	ImGui::SetNextWindowPos({ x + width / 2, y });
	ImGui::SetNextWindowSize({ width / 2, height });
	if (auto w = ImScoped::Window("Вывод", nullptr, static_window_flags)) {
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
}