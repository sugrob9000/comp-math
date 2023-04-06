#pragma once

#include "util/util.hpp"
#include <memory>
#include <span>
#include <algorithm>

namespace math {
// All matrices are presumed row-major. Triangular means upper triangular.

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
} // namespace math