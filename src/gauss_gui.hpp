#ifndef GAUSS_GUI_HPP
#define GAUSS_GUI_HPP

#include "math.hpp"

class Gauss {
	using Number = double;
	constexpr static unsigned max_rows = 20;
	constexpr static unsigned max_cols = 20;

	constexpr static unsigned max_variables = max_cols-1;

	struct Sized_static_matrix {
		math::Static_matrix<Number, max_rows, max_cols> matrix{};
		unsigned rows, cols;

		Sized_static_matrix () = default;
		Sized_static_matrix (unsigned r, unsigned c): rows{r}, cols{c} {}

		auto view () { return matrix.subview(0, 0, rows, cols); }
		auto view () const { return matrix.subview(0, 0, rows, cols); }

		unsigned num_equations () const { return rows; }
		unsigned num_variables () const { return cols-1; }
	};

	struct Input: Sized_static_matrix {
		Input (): Sized_static_matrix(4, 4) {}
		void widget ();

		enum class File_load_status { ok, unreadable, bad_dimensions, bad_data };
		File_load_status load_from_file (const char*);

		constexpr static size_t filename_buffer_size = 256;
		char filename_buffer[filename_buffer_size] = "matrix";
		File_load_status last_file_status = File_load_status::ok;
	};

	struct Output: Sized_static_matrix {
		explicit Output (const Input& in);
		void widget () const;

		int num_indeterminate_variables;

		// Only calculated when the variable coefficient matrix is square
		Number determinant;

		// Only calculated when the solution is unique
		Number solution[max_variables]; // in correct order (permutation applied)
		size_t permute[max_variables];  // for displaying columns in the pre-permutation order
		Number mismatch[max_rows];

		auto solution_span () const { return std::span{ solution, size_t(num_variables()) }; }
		auto permute_span () { return std::span{ permute, size_t(num_variables()) }; }

		auto mismatch_span () const { return std::span{ mismatch, size_t(num_equations()) }; }
		auto mismatch_span ()       { return std::span{ mismatch, size_t(num_equations()) }; }
	};

	Input input;
	std::optional<Output> output;

public:
	void input_widget () { input.widget(); }
	void output_widget ();
};

#endif // GAUSS_GUI_HPP
