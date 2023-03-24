#ifndef MAT_HPP
#define MAT_HPP

#include "util/util.hpp"
#include <span>
#include <memory>
#include <algorithm>

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
	[[nodiscard]] size_t rows () const { return Rows; }
	[[nodiscard]] size_t cols () const { return Cols; }

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

template <typename T> bool is_nonzero (T t) { return t != 0; }
template <typename T> bool is_zero (T t) { return t == 0; }

// Attempt to triangulate a matrix
template <typename T> void gauss_triangulate (Matrix_view<T> mat)
{
	assert(mat.cols() > 1 && mat.rows() > 0);

	const size_t num_equations = mat.rows();

	auto permute_equations = std::make_unique<size_t[]>(num_equations);
	for (size_t i = 0; i < num_equations; i++)
		permute_equations[i] = i;

	{ // Sort equations by number of leading zeros
		auto first_nonzero = std::make_unique<size_t[]>(num_equations);
		for (size_t i = 0; i < num_equations; i++) {
			auto row = mat[i];
			first_nonzero[i] = std::find_if(row.begin(), row.end(), is_nonzero<T>) - row.begin();
		}
		std::sort(permute_equations.get(), permute_equations.get()+mat.rows(),
				[&] (size_t a, size_t b) {
					return first_nonzero[a] < first_nonzero[b];
				});
	}

	for (size_t i = 0; i < num_equations; i++) {
		const size_t main_equ_id = permute_equations[i];
		const T main_element = mat[main_equ_id][i];
		if (main_element == 0) continue; // FIXME: handle this case?

		for (size_t ii = i+1; ii < num_equations; ii++) {
			const size_t equ_id = permute_equations[ii];
			T& element = mat[equ_id][i];

			// If i'th element is 0 in this equation, it's 0 in all the later ones
			if (element == 0) break;

			// Subtract from the free coefficient column as well, so count up to cols
			for (size_t j = i+1; j < mat.cols(); j++)
				mat[equ_id][j] -= (mat[main_equ_id][j] * element) / main_element;
			element = 0;
		}
	}

	{ // Apply equation permutation
		for (size_t i = 0; i+1 < num_equations; i++) {
			size_t j = i;
			do {
				j = permute_equations[j];
 			} while (j < i);
			std::swap_ranges(mat[i].begin(), mat[i].end(), mat[j].begin());
		}
	}
}

// Takes a triangular matrix.
// Return value:
//  < 0 means no solutions at all
//    0 means all variables are determined, they are put in `solution`
//  > 0 means at least N variables are independent
template <typename T> int gauss_gather (std::span<T> solution, Matrix_view<T> mat)
{
	assert(mat.cols() > 1 && mat.rows() > 0);

	size_t num_variables = mat.cols()-1;
	size_t num_equations = mat.rows();

	// "0*x1 + 0*x2 + ... = <nonzero>" -> no solution
	for (size_t i = 0; i < num_equations; i++) {
		auto row = mat[i];
		if (std::all_of(row.begin(), row.end()-1, is_zero<T>) && row.back() != 0)
			return -1;
	}

	if (num_equations > num_variables) {
		// The matrix being triangular, all the extra equations have zero coefficients
		// so check that the free coefficients are also zero
		for (size_t i = num_variables; i < num_equations; i++) {
			if (mat[i][mat.cols()-1] != 0)
				return -1;
		}

		// if they are, just ignore the extra equations from here on
		mat = mat.subview(0, 0, mat.cols(), mat.cols()-1);
		num_equations = num_variables;
	} else if (num_equations < num_variables) {
		// Fewer equations than variables: at least that many independent variables
		return num_variables - num_equations;
	}

	for (size_t i = num_equations-1; i != size_t(-1); i--) {
		// can't calculate a variable if its coefficient is zero
		if (mat[i][i] == 0)
			return 1;

		T coef = mat[i][mat.cols()-1];
		for (size_t j = i+1; j < num_variables; j++)
			coef -= mat[i][j] * solution[j];

		solution[i] = coef / mat[i][i];
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
