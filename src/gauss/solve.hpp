#pragma once

#include "gauss/matrix.hpp"

namespace math {

// First step of the Gauss method: attempt to triangulate a matrix.
// `dest` and `src` must not overlap.
// The variables may be permuted if there are zeros on the main diagonal.
// The order of permutation will be written to `permute_variables`
// Returns: the number of swaps made between variables
template <typename T> unsigned gauss_triangulate
(Matrix_view<T> dest, Matrix_view<const T> src, std::span<size_t> permute_variables)
{
	assert(src.cols() > 1 && src.rows() > 0);
	assert(src.rows() == dest.rows() && src.cols() == dest.cols());

	unsigned permutations = 0;

	const size_t num_equations = src.rows();
	const size_t num_variables = src.cols()-1;
	assert(permute_variables.size() == num_variables);

	for (size_t i = 0; i < num_variables; i++)
		permute_variables[i] = i;

	// Top row is unchanged
	for (size_t i = 0; i < src.cols(); i++)
		dest[0][i] = src[0][i];

	Matrix<T> tmp(src);

	for (size_t equ = 0, var = 0; equ < num_equations && var < num_variables; equ++) {
		auto tmp_row = tmp[equ];

		{ // Find a nonzero main element
			size_t nonzero = var;
			while (nonzero < num_variables && tmp_row[permute_variables[nonzero]] == 0)
				nonzero++;
			if (nonzero == num_variables)
				continue;
			if (nonzero != var) {
				std::swap(permute_variables[nonzero], permute_variables[var]);
				permutations++;
			}
		}

		size_t main_var_id = permute_variables[var];
		T main_element = tmp_row[main_var_id];
		assert(main_element != 0);

		for (size_t nequ = equ+1; nequ < num_equations; nequ++) {
			auto nrow = tmp[nequ];
			T& left_element = nrow[main_var_id];

			const auto apply = [&] (size_t var_id) {
				nrow[var_id] -= (left_element * tmp_row[var_id]) / main_element;
			};

			for (size_t nvar = var+1; nvar < num_variables; nvar++)
				apply(permute_variables[nvar]);
			apply(num_variables);

			left_element = 0;
		}

		var++;
	}

	for (size_t equ = 0; equ < num_equations; equ++) {
		auto tmp_row = tmp[equ];
		auto dest_row = dest[equ];
		for (size_t var = 0; var < num_variables; var++)
			dest_row[var] = tmp_row[permute_variables[var]];
		dest_row[num_variables] = tmp_row[num_variables];
	}

	return permutations;
}


// Second step of the Gauss method: gather solutions from a triangulated matrix.
// `mat` must be triangular already.
//
// Return value:
//  < 0 means no solutions at all
//  = 0 means all variables are determined, they are put in `solution`
//  > 0 means at least N variables are independent (R^N solution space)
//
// The values in `raw_solution` are calculated WRT the current order of columns in the
// matrix, which likely doesn't match the real order of variables from before triangulation
// So, to get the real solution, `raw_solution` will need to be permuted back
template <typename T> int gauss_gather (std::span<T> raw_solution, Matrix_view<T> mat)
{
	assert(mat.cols() > 1 && mat.rows() > 0);

	size_t num_variables = mat.cols()-1;
	size_t num_equations = mat.rows();

	// "0*x1 + 0*x2 + ... = <nonzero>" -> no solution
	for (size_t i = 0; i < num_equations; i++) {
		if (auto row = mat[i]; row.back() != 0) {
			size_t j = i; // nonzero coefficients can only be left of the diagonal, inclusive
			while (j < num_variables && row[j] == 0)
				j++;
			if (j == num_variables)
				return -1;
		}
	}

	if (num_equations < num_variables) {
		// Fewer equations than variables: at least that many independent variables
		return num_variables - num_equations;
	} else if (num_equations > num_variables) {
		// The matrix being triangular, all the extra equations have zero coefficients.
		// We've already checked that their free coefficient is also zero, so just ignore them
		mat = mat.subview(0, 0, mat.cols(), mat.cols()-1);
		num_equations = num_variables;
	}

	for (size_t i = num_equations-1; i != size_t(-1); i--) {
		// can't calculate a variable if its coefficient is zero
		if (mat[i][i] == 0)
			return 1;

		T coef = mat[i][mat.cols()-1];
		for (size_t j = i+1; j < num_variables; j++)
			coef -= mat[i][j] * raw_solution[j];

		raw_solution[i] = coef / mat[i][i];
	}

	return 0;
}

// The determinant of a triangular matrix (product of main diagonal)
// `mat` must be square
template <typename T> T triangular_determinant (Matrix_view<T> mat)
{
	assert(mat.rows() == mat.cols());
	assert(mat.rows() > 0);
	T result = 1;
	for (size_t i = 0; i < mat.rows(); i++)
		result *= mat[i][i];
	return result;
}

// Multiply a matrix with a vector.
// The dimensions must be appropriate for the multiplication to make sense.
// `dest` and `vec` must not overlap
template <typename T>
void mul_matrix_vector (std::span<T> dest, Matrix_view<T> mat, std::span<T> vec)
{
	assert(mat.rows() == dest.size());
	assert(mat.cols() == vec.size());
	for (size_t i = 0; i < mat.rows(); i++) {
		dest[i] = 0;
		for (size_t j = 0; j < mat.cols(); j++)
			dest[i] += mat[i][j] * vec[j];
	}
}

} // namespace math