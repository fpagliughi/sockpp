/**
 * @file mbedtls_error.h
 *
 * TLS (SSL) errors using mbedTLS.
 *
 * @author Frank Pagliughi
 * @date December 2023
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2023 Frank Pagliughi
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

#ifndef __sockpp_tls_mbedtls_error_h
#define __sockpp_tls_mbedtls_error_h

#include <system_error>

#include "mbedtls/error.h"

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
    const char* name() const noexcept override final { return "MbedTLSError"; }

    /**
     * Gets the string representation of the error.
     */
    std::string message(int c) const override final {
        char buf[512];
        mbedtls_strerror(c, buf, sizeof(buf));
        return std::string(buf);
    }
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

class tls_error : public ::std::system_error
{
public:
    tls_error(int err) : system_error{make_tls_error_code(err)} {}
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_error_h
