#include "gauss_gui.hpp"
#include "imcpp20.hpp"
#include "util/util.hpp"
#include <fstream>

using namespace ImGui;
using namespace ImScoped;

constexpr static ImColor bad_red(0.9f, 0.2f, 0.2f, 1.0f);
constexpr static ImGuiTableFlags matrix_table_flags
	= ImGuiTableFlags_SizingFixedFit
	| ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

// ======================================= Input =======================================

void Gauss::Input::widget ()
{
	if (auto bar = TabBar("tabs")) {
		if (auto item = TabItem("Загрузить из файла")) {
			bool do_load = InputText("Имя файла",
					filename_buffer, std::size(filename_buffer),
					ImGuiInputTextFlags_EnterReturnsTrue);
			do_load |= Button("Загрузить");
			if (do_load)
				last_file_status = load_from_file(filename_buffer);

			switch (last_file_status) {
			case File_load_status::ok:
				break;
			case File_load_status::unreadable:
				TextColored(bad_red, "Не удалось прочитать файл");
				break;
			case File_load_status::bad_dimensions:
				TextColored(bad_red, "Некорректные размерности матрицы в файле");
				break;
			case File_load_status::bad_data:
				TextColored(bad_red, "В файле не численные данные");
				break;
			}
		}

		if (auto item = TabItem("Ввести матрицу")) {
			Slider("строк",    &rows, 1u, max_rows, nullptr, ImGuiSliderFlags_AlwaysClamp);
			Slider("столбцов", &cols, 2u, max_cols, nullptr, ImGuiSliderFlags_AlwaysClamp);

			if (auto table = Table("input", cols)) {
				TableNextRow();
				BeginDisabled();
				for (unsigned col = 0; col < num_variables(); col++) {
					TableNextColumn();
					TextFmt("X{}", col+1);
				}
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
	if (auto table = Table("equations", cols, matrix_table_flags)) {
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

void Gauss::output_widget ()
{
	if (Button("Вычислить"))
		output.emplace(input);
	if (output)
		output->widget();
}

Gauss::Output::Output (const Input& in)
{
	rows = in.rows;
	cols = in.cols;

	// gauss_gather() gives the solution in a meaningless "raw" pre-permutation order
	auto raw_solution = std::make_unique<Number[]>(num_variables());
	std::span raw_solution_span { raw_solution.get(), size_t(num_variables()) };

	math::gauss_triangulate(view(), in.view(), permute_span());
	num_indeterminate_variables = math::gauss_gather(raw_solution_span, view());

	auto variables_view = view().subview(0, 0, num_equations(), num_variables());

	if (num_equations() == num_variables())
		determinant = math::triangular_determinant(variables_view);

	if (num_indeterminate_variables == 0) {
		mul_matrix_vector(mismatch_span(), variables_view, raw_solution_span);
		for (unsigned i = 0; i < num_equations(); i++)
			mismatch[i] -= view()[i][cols-1];
		for (unsigned i = 0; i < num_variables(); i++)
			solution[permute[i]] = raw_solution[i];
	}
}

void Gauss::Output::widget ()
{
	SeparatorText("Треугольный вид матрицы:");
	const float table_height = 2 * rows * ImGui::GetFontSize();
	if (auto table = Table("output", cols, matrix_table_flags, ImVec2(0, table_height))) {
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
				if (mat[row][col] != 0)
					TextFmtColored(bad_red, FMT_STRING("{}"), mat[row][col]);
			}

			for (unsigned col = diag; col < cols; col++) {
				TableNextColumn();
				TextFmt(FMT_STRING("{}"), mat[row][col]);
			}
		}
	}

	Separator();

	if (num_equations() == num_variables())
		TextFmt(FMT_STRING("Определитель подматрицы коэффициентов: {}"), determinant);
	else
		TextWrapped("Подматрица коэффициентов не квадратная. Определитель не имеет смысла");

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
