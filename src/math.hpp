#ifndef MAT_HPP
#define MAT_HPP

#include "util/util.hpp"
#include <memory>
#include <span>

namespace math {
// All matrices are presumed row-major. Triangular means upper triangular.

// ==================================== Matrix types ====================================

template <typename T> class Matrix_view {
	T* ptr_;
	size_t rows_, cols_, stride_;

	template <typename E> friend class Matrix;

	using Row_reference = std::span<T>;

	class Row_iterator {
		T* ptr_;
		size_t cols_, stride_;

	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = ptrdiff_t;
		using value_type = Row_reference;
		using reference = Row_reference;
		using size_type = size_t;

		Row_iterator (T* ptr, size_t cols, size_t stride)
			: ptr_{ptr}, cols_{cols}, stride_{stride} {}
		Row_iterator (const Row_iterator&) = default;

		Row_iterator& operator++ () { ptr_ += stride_; return *this; }
		Row_iterator& operator-- () { ptr_ -= stride_; return *this; }

		Row_reference operator* () const { return { ptr_, cols_ }; }

		bool operator== (const Row_iterator& lhs) const { return this->ptr_ == lhs.ptr_; }

		ptrdiff_t operator- (const Row_iterator lhs) const {
			assert(this->cols_ == lhs.cols_ && this->stride_ == lhs.stride_);
			ptrdiff_t dist = this->ptr_ - lhs.ptr_;
			assert(dist % stride_ == 0);
			return dist / stride_;
		}
	};

public:
	[[nodiscard]] size_t rows () const { return rows_; }
	[[nodiscard]] size_t cols () const { return cols_; }
	[[nodiscard]] size_t stride () const { return stride_; }

	Matrix_view (T* ptr, size_t rows, size_t cols, size_t stride)
		: ptr_{ptr}, rows_{rows}, cols_{cols}, stride_{stride} {}
	Matrix_view (Matrix_view&&) noexcept = default;
	Matrix_view (const Matrix_view&) = default;
	Matrix_view& operator= (Matrix_view&&) noexcept = default;
	Matrix_view& operator= (const Matrix_view&) = default;

	using row_ref = Row_reference;
	using iterator = Row_iterator;

	row_ref operator[] (size_t row) const {
		assert(row < rows_);
		return { ptr_ + stride_*row, cols_ };
	}

	iterator begin () const { return { ptr_, cols_, stride_ }; };
	iterator end () const { return { ptr_ + rows_*stride_, cols_, stride_ }; };

	Matrix_view subview (size_t start_row, size_t start_col, size_t rows, size_t cols) const {
		assert(start_row < rows_ && start_col < cols_);
		assert(start_row + rows <= rows_ && start_col + cols <= cols_);
		return Matrix_view { ptr_ + start_row * stride_ + start_col, rows, cols, stride_ };
	}
};

template <typename T, size_t Rows, size_t Cols> class Static_matrix {
	T storage[Rows * Cols];
public:
	[[nodiscard]] constexpr size_t rows () const { return Rows; }
	[[nodiscard]] constexpr size_t cols () const { return Cols; }

	operator Matrix_view<const T> () const { return { storage, Rows, Cols, Cols }; }
	operator Matrix_view<T> () { return { storage, Rows, Cols, Cols }; }

	decltype(auto) operator[] (size_t row) { return Matrix_view<T>{*this}[row]; }
	decltype(auto) operator[] (size_t row) const { return Matrix_view<const T>{*this}[row]; }

	auto subview (size_t start_row, size_t start_col, size_t rows, size_t cols) {
		return Matrix_view<T>{*this}.subview(start_row, start_col, rows, cols);
	}

	auto subview (size_t start_row, size_t start_col, size_t rows, size_t cols) const {
		return Matrix_view<const T>{*this}.subview(start_row, start_col, rows, cols);
	}
};

template <typename T> class Matrix {
	size_t rows_, cols_;
	std::unique_ptr<T[]> storage;
public:
	[[nodiscard]] size_t rows () const { return rows_; }
	[[nodiscard]] size_t cols () const { return cols_; }

	Matrix (size_t rows, size_t cols)
		: rows_{rows}, cols_{cols}, storage{ new T[rows * cols] } {}

	Matrix (Matrix&&) noexcept = default;
	Matrix (const Matrix& lhs): Matrix(lhs.rows(), lhs.cols()) {
		std::copy_n(lhs.storage.get(), rows_ * cols_, this->storage.get());
	}

	template <typename E>
	explicit Matrix (const Matrix_view<E>& lhs): Matrix(lhs.rows(), lhs.cols()) {
		T* const d = this->storage.get();
		E* const s = lhs.ptr_;
		if (lhs.stride_ == cols_) {
			std::copy_n(s, cols_ * rows_, d);
		} else {
			for (size_t row = 0; row < rows_; row++) {
				T* const dst = d + row * cols_;
				E* const src = s + row * lhs.stride_;
				std::copy_n(src, cols_, dst);
			}
		}
	}

	operator Matrix_view<const T> () const { return { storage.get(), rows_, cols_, cols_ }; }
	operator Matrix_view<T> () { return { storage.get(), rows_, cols_, cols_ }; }

	decltype(auto) operator[] (size_t row) { return Matrix_view<T>{*this}[row]; }
	decltype(auto) operator[] (size_t row) const { return Matrix_view<const T>{*this}[row]; }

	auto subview (size_t start_row, size_t start_col, size_t rows, size_t cols) {
		return Matrix_view<T>{*this}.subview(start_row, start_col, rows, cols);
	}

	auto subview (size_t start_row, size_t start_col, size_t rows, size_t cols) const {
		return Matrix_view<const T>{*this}.subview(start_row, start_col, rows, cols);
	}
};


// ===================================== Operations =====================================

// Attempt to triangulate a matrix. `dest` and `src` must not overlap
template <typename T> void gauss_triangulate
(Matrix_view<T> dest, Matrix_view<const T> src, std::span<size_t> permute_variables)
{
	assert(src.cols() > 1 && src.rows() > 0);
	assert(src.rows() == dest.rows() && src.cols() == dest.cols());

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
			if (nonzero != var)
				std::swap(permute_variables[nonzero], permute_variables[var]);
		}

		size_t var_id = permute_variables[var];
		T main_element = tmp_row[var_id];
		assert(main_element != 0);

		for (size_t nequ = equ+1; nequ < num_equations; nequ++) {
			auto nrow = tmp[nequ];
			T& left_element = nrow[var_id];

			//const auto mutate = [&] (size_t var_idx) { }; // TODO

			for (size_t nvar = var+1; nvar < num_variables; nvar++) {
				size_t nvar_id = permute_variables[nvar];
				nrow[nvar_id] -= (left_element * tmp_row[nvar_id]) / main_element;
			}
			// Last column doesn't participate in permutation: handle specially
			nrow[num_variables] -= (left_element * tmp_row[num_variables]) / main_element;
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

template <typename T> T triangular_determinant (Matrix_view<T> mat)
{
	assert(mat.rows() == mat.cols());
	assert(mat.rows() > 0);
	T result = 1;
	for (size_t i = 0; i < mat.rows(); i++)
		result *= mat[i][i];
	return result;
}

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

#endif // MAT_HPP
