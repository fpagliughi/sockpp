/// @file result.h
///
/// Type(s) for return values that can indicate a success or failure.
///
/// @date	January 2023

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------

#ifndef __sockpp_result_h
#define __sockpp_result_h

#include "sockpp/platform.h"
#include <system_error>
#include <iostream>

namespace sockpp {

/** A sockpp error_code is a std error_code. */
using error_code = std::error_code;

/** A sockpp error_category is a std error_category. */
using error_category = std::error_category;

using errc = std::errc;

/////////////////////////////////////////////////////////////////////////////

/**
 * A result type that can contain a value of any type on successful
 * completion of an operation, or a std::error_code on failure.
 */
template <typename T>
class result {
	/** The return value of an operation, if successful */
	T val_;
	/** The error returned from an operation, if failed */
	error_code err_;

	result(const T& val, const error_code& err) : val_{val}, err_{err} {}

	/**
	 * OS-specific means to retrieve the last error from an operation.
	 * This should be called after a failed system call to get the cause of
	 * the error.
	 */
	static int get_last_error() {
		#if defined(_WIN32)
			return ::WSAGetLastError();
		#else
			int err = errno;
			return err;
		#endif
	}

	friend class socket;

public:
	/**
	 * Default result is considered a success with default value.
	 */
	result() =default;
	/**
	 * Construct a "success" result with the specified value.
	 * @param val The success return value
	 */
	result(const T& val) : val_{val} {}
	/**
	 * Creates an unsuccessful result from an error code.
	 * @param err The error code from an operation.
	 * @return The result of an unsucessful operation.
	 */
	static result from_error(const error_code& err) {
		return result{ T{}, err };
	}
	/**
	 * Creates an unsuccessful result from an platform-specific integer
	 * error code and an optional category.
	 * @param ec The platform-specific error code.
	 * @param ecat The error category.
	 * @return The result of an unsuccessful operation.
	 */
	static result from_error(
		int ec,
		const error_category& ecat=std::system_category()
    ) {
		return result{ T{}, {ec, ecat} };
	}
	/**
	 * Determines if the result represents a failed operation.
	 *
	 * If true, then the error variant of the result is valid.
	 * @return @em true if the result is from a failed operation, @em false
	 *  	   if the operation succeeded.
	 */
	bool is_error() const { return bool(err_); }
	/**
	 * Determines if the result represents a successful operation.
	 *
	 * If true, then the success (value) variant of the result is valid.
	 * @return @em true if the result is from a successful operation, @em
	 *  	   false if the operation failed.
	 */
	bool is_ok() const { return !is_error(); }
	/**
	 * Determines if the result represents a successful operation.
	 *
	 * If true, then the success (value) variant of the result is valid.
	 * @sa is_ok()
	 * @return @em true if the result is from a successful operation, @em
	 *  	   false if the operation failed.
	 */
	operator bool() const { return is_ok(); }
	/**
	 * Gets the value from a successful operation.
	 *
	 * This is only valid if the operation was a success. If not, it returns
	 * the default value for type T.
	 * @return A const reference to the success value.
	 */
	const T& value() const { return val_; };
	/**
	 * Gets the error code from a failed operation.
	 *
	 * This is only valid if the operation failed. If not, it returns the
	 * default error code which should have a value of zero (success).
	 * @return A const reference to the error code.
	 */
	const error_code& error() const { return err_; }
};

/**
 * Create a successful result with the specified value.
 *
 * @param val The succesful return value from the operation.
 * @return A success result.
 */
template <typename T>
result<T> success(const T& val) {
	return result<T>(val);
}

/**
 * Create a failed result with the specified error code.
 *
 * @param err The error code from the operation.
 * @return A failed result.
 */
template <typename T>
result<T> error(const error_code& err) {
	return result<T>::from_error(err);
}

/**
 * Create a failed result with the specified platform-specific integer
 * error code.
 *
 * @param ec The platform-specific error code.
 * @param ecat The error category.
 * @return A failed result.
 */
template <typename T>
result<T> error(int ec, const error_category& ecat=std::system_category()) {
	return result<T>::from_error(ec, ecat);
}

/**
 * Writes out the result.
 *
 * For a successful operation, writes out the result value. For a failed
 * operation, writes out the error message.
 *
 * This requires type T to have a stream inserter.
 *
 * @param os The output stream.
 * @param res The result to output.
 * @return A reference to the output stream.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const result<T>& res) {
	if (res.is_ok()) {
		os << res.val();
	}
	else {
		os << res.err().message();
	}
	return os;
}

/** The result of an I/O operation that should return an int. */
using ioresult = result<int>;

/////////////////////////////////////////////////////////////////////////////

#if 0
/**
 * Result of a thread-safe I/O operation.
 *
 * Most I/O operations in the OS will return >=0 on sccess and -1 on error.
 * In the case of an error, the calling thread must read an `errno` variable
 * immediately, before any other system calls, to get the cause of an error
 * as an integer value defined by the constants ENOENT, EINTR, EBUSY, etc.
 *
 * Most sockpp calls retain this known behavior, but will cache this errno
 * value to be read later by a call to @ref socket::last_error() or similar.
 * But this makes the @ref socket classes themselves non-thread-safe since
 * the cached errno variable can't be shared across threads.
 *
 * Several new "thread-safe" variants of I/O functions simply retrieve the
 * errno immediately on a failed I/O call and instead of caching the value,
 * return it immediately in this ioresult.
 * See: @ref stream_socket::read_r, @ref stream_socket::read_n_r, @ref
 * stream_socket::write_r, etc.
 */
class ioresult {
	/** Byte count, or 0 on error or EOF */
	size_t cnt_ = 0;
	/** errno value (0 if no error or EOF) */
	int err_ = 0;

	/**
	 * OS-specific means to retrieve the last error from an operation.
	 * This should be called after a failed system call to get the caue of
	 * the error.
	 */
	static int get_last_error();

	friend class socket;

public:
	/** Creates an empty result */
	ioresult() = default;

	ioresult(size_t c, int e) : cnt_{c}, err_{e} {}
	/**
	 * Creates a result from the return value of a low-level I/O function.
	 * @param n The number of bytes read or written. If <0, then an error is
	 *  		assumed and obtained from socket::get_last_error().
	 *
	 */
	explicit inline ioresult(ssize_t n) {
		if (n < 0)
			err_ = get_last_error();
		else
			cnt_ = size_t(n);
	}

	/** Sets the error value */
	void set_error(int e) { err_ = e; }

	/** Increments the count */
	void incr(size_t n) { cnt_ += n; }

	/** Determines if the result is OK (not an error) */
	bool is_ok() const { return err_ == 0; }
	/** Determines if the result is an error */
	bool is_err() const { return err_ != 0; }

	/** Determines if the result is OK */
	explicit operator bool() const { return is_ok(); }

	/** Gets the count */
	size_t count() const { return cnt_; }

	/** Gets the error */
	int error() const { return err_; }
};
#endif

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_result_h

