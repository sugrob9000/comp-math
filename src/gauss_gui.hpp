#ifndef GAUSS_GUI_HPP
#define GAUSS_GUI_HPP

#include "math.hpp"

class Gauss {
	using Element = double;
	constexpr static int max_rows = 20;
	constexpr static int max_cols = 20;

	using Static_matrix = math::Static_matrix<Element, max_rows, max_cols>;
	using Matrix_view = math::Matrix_view<Element>;
	constexpr static int max_variables = max_cols-1;

	struct Sized_static_matrix {
		Static_matrix matrix{};
		int rows = 4, cols = 4;

		auto view () { return matrix.subview(0, 0, rows, cols); }
		int num_equations () const { return rows; }
		int num_variables () const { return cols-1; }
	};

	struct Input: Sized_static_matrix {
		void widget ();

		enum class File_load_status { ok, unreadable, bad_dimensions, bad_data };
		File_load_status load_from_file (const char*);

		constexpr static size_t filename_buffer_size = 256;
		char filename_buffer[filename_buffer_size] = "matrix";
		File_load_status last_file_status = File_load_status::ok;
	};

	struct Output: Sized_static_matrix {
		Output (const Input& in);
		void widget ();

		auto solution_span () { return std::span{ solution, size_t(num_variables()) }; }
		auto mismatch_span () { return std::span{ mismatch, size_t(num_equations()) }; }

		// < 0 means no solutions at all
		//   0 means all variables are determined
		// > 0 means at least N variables are independent
		int num_indeterminate_variables;

		// Only calculated if appropriate
		Element determinant;
		Element solution[max_variables];
		Element mismatch[max_rows];
	};

	Input input;
	std::optional<Output> output;

public:
	void input_widget () { input.widget(); }
	void output_widget ();
};

#endif // GAUSS_GUI_HPP
