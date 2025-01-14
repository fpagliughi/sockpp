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

#include <iostream>

#include "sockpp/error.h"
#include "sockpp/platform.h"
#include "sockpp/types.h"

namespace sockpp {

/**
 * Type for a result that returns nothing.
 */
struct none
{
};

#if 0
/**
 * Writes out a none value.
 * @param os The output stream.
 * @return A reference to the output stream.
 */
inline std::ostream& operator<<(std::ostream& os, const none&) {
    os << "<none>";
    return os;
}
#endif

/////////////////////////////////////////////////////////////////////////////

/**
 * A result type that can contain a value of any type on successful
 * completion of an operation, or a std::error_code on failure.
 *
 * Objects can contain a value of type T on an operation's success, or a
 * standard `error_code` on failure. As an implementation detail, the class
 * isn't implemented with a union or variant, but rather both types are
 * available. When the error code indicates "success" (where it has an
 * internal value of zero), then the result value is considered valid. If
 * the code contains an error, that takes precedence and the result value is
 * set to the default for that type - zero for an int, empty for a string,
 * etc.
 *
 * The result can act as a boolean - @em true for a successful result, @em
 * false for an error/failure.
 *
 * The result can also be directly compared to values of type T or to any of
 * the standard error types, `std::error_code`, `std::error_condition`, or
 * `std::errc`. When comparing to type T, it will always fail if the result
 * is an error, otherwise it will do a comparison of the T values. Comparing
 * to an error will simply compare the error values.
 *
 * As a weird corner case, comparing an error code of zero - which indicates
 * "no error" - against a result will always be true for _any_ successful
 * result value. This should probably not be done as it could lead to some
 * confusion.
 */
template <typename T = none>
class result
{
    /** The return value of an operation, if successful */
    T val_{};
    /** The error returned from an operation, if failed */
    error_code err_{};

    /**
     * Private helper constructor to build a result.
     * @param val The value
     * @param err The error
     */
    result(const T& val, const error_code& err) : val_{val}, err_{err} {}
    /**
     * Private helper constructor to build a result.
     * @param val The value
     * @param err The error
     */
    result(T&& val, const error_code& err) : val_{std::move(val)}, err_{err} {}
    /**
     * OS-specific means to retrieve the last error from an operation.
     * This should be called after a failed system call to get the cause of
     * the error.
     *
     * On most systems, this is the current `errno`.
     *
     * On Windows this retrieves the error via `WSAGetLastError()`
     */
    static int get_last_errno() {
#if defined(_WIN32)
        return ::WSAGetLastError();
#else
        int err = errno;
        return err;
#endif
    }

public:
    /**
     * Default result is considered a success with default value.
     */
    result() = default;
    /**
     * Construct a success result with the specified value.
     * @param val The success value
     */
    result(const T& val) : val_{val} {}
    /**
     * Construct a success result with the specified value.
     * @param val The success value
     */
    result(T&& val) : val_{std::move(val)} {}
    /**
     * Creates a failed result from a portable error condition.
     * @param err The error
     */
    result(errc err) : err_{std::make_error_code(err)} {}
    /**
     * Creates a failed result from a portable error condition.
     * @param err The error
     */
    result(const error_code& err) : err_{err} {}
    /**
     * Creates a failed result from a portable error condition.
     * @param err The error
     */
    result(error_code&& err) : err_{std::move(err)} {}
    /**
     * Creates a failed result from an error code.
     * @param err The error code from an operation.
     * @return The result of an unsuccessful operation.
     */
    static result from_error(const error_code& err) { return result{T{}, err}; }
    /**
     * Creates a failed result from an platform-specific integer error code
     * and an optional category.
     * @param ec The platform-specific error code.
     * @param ecat The error category.
     * @return The result of an unsuccessful operation.
     */
    static result from_error(int ec, const error_category& ecat = std::system_category()) {
        return result{T{}, {ec, ecat}};
    }
    /**
     * Creates an unsuccessful result from a portable error condition.
     * @param err The error
     * @return The result of an unsuccessful operation.
     */
    static result from_error(errc err) { return result(err); }
    /**
     * Creates an unsuccessful result from an platform-specific integer
     * error code and an optional category.
     * @return The result for the last unsuccessful system operation.
     */
    static result from_last_error() { return result{last_error()}; }
    /**
     * Retrieves the last error from an operation.
     * This should be called after a failed system call to get the cause of
     * the error.
     */
    static error_code last_error() {
        int err = get_last_errno();
        return error_code{err, std::system_category()};
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
    bool is_ok() const { return !bool(err_); }
    /**
     * Determines if the result represents a successful operation.
     *
     * If true, then the success (value) variant of the result is valid.
     * @sa is_ok()
     * @return @em true if the result is from a successful operation, @em
     *  	   false if the operation failed.
     */
    explicit operator bool() const { return !bool(err_); }
    /**
     * Gets the value from a successful operation.
     *
     * This is only valid if the operation was a success. If not, it returns
     * the default value for type T.
     * @return A const reference to the success value.
     */
    const T& value() const { return val_; };
    /**
     * Gets the value if the result is a success, otherwise throws a system
     * error exception corresponding to the internal error code if it hold
     * an error.
     * @return A const reference to the success value.
     * @throws std::system_error if the result is an error
     */
    const T& value_or_throw() const {
        if (err_)
            throw std::system_error{err_};
        return val_;
    }
    /**
     * Releases the value from this result.
     *
     * The value is only valid if the operation was successful. Releasing
     * the value will move it out of this result, leaving an unknown, but
     * valid value in its place. This might be necessary to retrieve a value
     * that only implements move semantics (such as a socket), or if the
     * caller would rather not copy the object.
     *
     * The result should not be used after releasing the value.
     *
     * @return The success value.
     */
    T&& release() { return std::move(val_); }
    /**
     * Releases the value if the result is a success, otherwise throws a
     * system error exception corresponding to the internal error code if it
     * hold an error.
     *
     * The result should not be used after releasing the value.
     *
     * @return A const reference to the success value.
     * @throws std::system_error if the result is an error
     */
    T&& release_or_throw() {
        if (err_)
            throw std::system_error{err_};
        return std::move(val_);
    }
    /**
     * Gets the error code from a failed operation.
     *
     * This is only valid if the operation failed. If not, it returns the
     * default error code which should have a value of zero (success).
     * @return A const reference to the error code.
     */
    const error_code& error() const { return err_; }
    /**
     * Gets the message corresponding to the current error.
     * Equivalent to `error().message()`
     * @return The message corresponding to the current error.
     */
    std::string error_message() const { return err_.message(); }
};

/**
 * Create a successful result with the specified value.
 *
 * @param val The successful return value from the operation.
 * @return A success result.
 */
template <typename T>
result<T> success(const T& val) {
    return result<T>(val);
}

/**
 * Create a successful result with the specified value.
 *
 * @param val The successful return value from the operation.
 * @return A success result.
 */
template <typename T>
result<T> success(T&& val) {
    return result<T>(std::move(val));
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
 * @param err The portable error condition.
 * @return A failed result.
 */
template <typename T>
result<T> error(errc err) {
    return result<T>{err};
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
result<T> error(int ec, const error_category& ecat = std::system_category()) {
    return result<T>::from_error(ec, ecat);
}

/**
 * Compare the result to a value.
 *
 * This fails if the result is an error or if the values don't maych
 * @param res A result.
 * @param val An value of the same type
 * @return @em true if the result is a success and the values are equal,
 *         false otherwise.
 */
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool operator==(const result<T>& res, const T& val) noexcept {
    return res && res.value() == val;
}

/**
 * Compare the result to a value.
 *
 * This fails if the result is an error or if the values don't maych
 * @param val An value of the same type
 * @param res A result.
 * @return @em true if the result is a success and the values are equal,
 *         false otherwise.
 */
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool operator==(const T& val, const result<T>& res) noexcept {
    return res && res.value() == val;
}

/**
 * Compare the result to a value.
 *
 * This fails if the result is an error or if the values don't maych
 * @param res A result.
 * @param val An value of the same type
 * @return @em true if the result is a failure or the values are not equal,
 *         false otherwise.
 */
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool operator!=(const result<T>& res, const T& val) noexcept {
    return !res || res.value() != val;
}

/**
 * Compare the result to a value.
 *
 * This fails if the result is an error or if the values don't maych
 * @param res A result.
 * @param val An value of the same type
 * @return @em true if the result is a failure or the values are not equal,
 *         false otherwise.
 */
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool operator!=(const T& val, const result<T>& res) noexcept {
    return !res || res.value() != val;
}

/**
 * Compare the result to an error code.
 *
 * @param res A result.
 * @param err An error code
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator==(const result<T>& res, const error_code& err) noexcept {
    return res.error() == err;
}

/**
 * Compare the result to an error code.
 *
 * @param err An error code
 * @param res A result.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator==(const error_code& err, const result<T>& res) noexcept {
    return err == res.error();
}

/**
 * Compare the result to an error.
 *
 * @param res A result.
 * @param err A portable error condition.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator==(const result<T>& res, const errc& err) noexcept {
    return res.error() == err;
}

/**
 * Compare the result to an error.
 *
 * @param err A portable error condition.
 * @param res A result.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator==(const errc& err, const result<T>& res) noexcept {
    return err == res.error();
}

/**
 * Compare the result to an error code.
 *
 * @param res A result.
 * @param err An error code
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator!=(const result<T>& res, const error_code& err) noexcept {
    return res.error() != err;
}

/**
 * Compare the result to an error code.
 *
 * @param err An error code
 * @param res A result.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator!=(const error_code& err, const result<T>& res) noexcept {
    return res.error() != err;
}

/**
 * Compare the result to an error.
 *
 * @param res A result.
 * @param err A portable error condition.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator!=(const result<T>& res, const errc& err) noexcept {
    return res.error() != err;
}

/**
 * Compare the result to an error.
 *
 * @param err A portable error condition.
 * @param res A result.
 * @return @em true if the error code matches the one in the result, false
 *  	   otherwise.
 */
template <typename T>
bool operator!=(const errc& err, const result<T>& res) noexcept {
    return res.error() != err;
}

#if 0
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
        os << res.value();
    }
    else {
        os << res.error_message();
    }
    return os;
}
#endif

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_result_h
