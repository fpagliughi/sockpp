/**
 * @file openssl_error.h
 *
 * TLS (SSL) socket errors for OpenSSL.
 *
 * @author Frank Pagliughi
 * @date January 2024
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2024 Frank Pagliughi
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

#ifndef __sockpp_openssl_error_h
#define __sockpp_openssl_error_h

// Note that OpenSSL uses unsigned long's as their error type. On modern
// 64-bit systems/compilers, longs are commonly 64-bits while ints are only
// 32-bits. But, apparently, the library only uses 32-bits of the values
// in the form:
//     0xLLFFFRRR
// (L)ibrary, (F)unction, & (R)eason
//
// Thus it seems that we can use the OpenSSL error values as the int in
// these error_code's within a single category.
//

#include <openssl/err.h>

#include <system_error>

#include "sockpp/result.h"

namespace detail {
/**
 * A custom error code category derived from std::error_category.
 */
class tls_errc_category : public std::error_category
{
public:
    /**
     * Gets short descriptive name for the category.
     */
    const char* name() const noexcept override final { return "OpenSSLError"; }
    /**
     * Gets the string representation of the error.
     */
    std::string message(int c) const override final;
};
}  // namespace detail

namespace sockpp {
// Declare a global function returning a static instance of the custom category
const ::detail::tls_errc_category& tls_errc_category();

#if 0
// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline ::std::error_code make_error_code(tls_errc e) {
    if (e == tls_errc::system_error)
        return {errno, std::system_category()};

    return {static_cast<int>(e), tls_errc_category()};
}
#endif

inline ::std::error_code make_tls_error_code(int err) { return {err, tls_errc_category()}; }

/**
 * Gets the last error from OpenSSL.
 * @return An error code for the last error in OpenSSL
 */
inline ::std::error_code tls_last_error() {
    auto err = ::ERR_get_error();
    return {(int)err, tls_errc_category()};
}

/**
 * An error from a secure operation implemented with OpenSSL.
 */
class tls_error : public ::std::system_error
{
public:
    tls_error(::std::error_code err) : ::std::system_error{err} {}
    tls_error(int err) : ::std::system_error{make_tls_error_code(err)} {}

    static tls_error from_last_error() { return tls_error{tls_last_error()}; }
};

/**
 * Checks the value and if less than zero, returns the error.
 * @tparam T A signed integer type of any size
 * @tparam Tout A integer type that can be created from a T type.
 * @param ret The return value from a library or system call.
 * @return The error code from the last error, if any.
 */
template <typename T, typename Tout = T>
static result<Tout> tls_check_res(T ret) {
    return (ret <= 0) ? result<Tout>{tls_last_error()} : result<Tout>{Tout(ret)};
}
/**
 * Checks the value and if less than zero, returns the error.
 * @tparam T A signed integer type of any size
 * @param ret The return value from a library or system call.
 * @return The error code from the last error, if any.
 */
template <typename T>
static result<> tls_check_res_none(T ret) {
    return (ret <= 0) ? result<>{tls_last_error()} : result<>{none{}};
}

}  // namespace sockpp

#endif  // __sockpp_openssl_error_h
