#pragma once

#include "gauss/matrix.hpp"

namespace gauss {
using Number = double;

constexpr unsigned max_rows = 20;
constexpr unsigned max_cols = 20;

constexpr unsigned max_variables = max_cols-1;

struct Sized_static_matrix {
	math::Static_matrix<Number, max_rows, max_cols> matrix{};
	unsigned rows;
	unsigned cols;

	Sized_static_matrix (unsigned r, unsigned c): rows{r}, cols{c} {}
	Sized_static_matrix (): Sized_static_matrix(0, 0) {}

	auto view () { return matrix.subview(0, 0, rows, cols); }
	auto view () const { return matrix.subview(0, 0, rows, cols); }

	unsigned num_equations () const { return rows; }
	unsigned num_variables () const { return cols-1; }
};

class Input: public Sized_static_matrix {
	enum class File_load_status { ok, unreadable, bad_dimensions, bad_data };
	File_load_status load_from_file (const char*);
	File_load_status last_file_load_status;

	constexpr static size_t path_buf_size = 256;
	char path_buf[path_buf_size] = "matrix";

public:
	Input (): Sized_static_matrix(4, 4) {}
	void widget ();
};

class Output: public Sized_static_matrix {
	int num_indeterminate_variables;

	// Only calculated when the variable coefficient matrix is square
	Number determinant;

	// Only calculated when the solution is unique
	Number solution[max_variables]; // in correct order (permutation applied)
	size_t permute[max_variables];  // for displaying columns in the pre-permutation order
	Number mismatch[max_rows];
	unsigned permutations = 0;

	auto solution_span () const { return std::span{ solution, size_t(num_variables()) }; }
	auto permute_span () { return std::span{ permute, size_t(num_variables()) }; }
	auto mismatch_span () const { return std::span{ mismatch, size_t(num_equations()) }; }
	auto mismatch_span ()       { return std::span{ mismatch, size_t(num_equations()) }; }

public:
	explicit Output (const Input& in);
	void widget () const;
};
}