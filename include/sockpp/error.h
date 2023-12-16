/**
 * @file error.h
 *
 * Error classes for the sockpp library.
 *
 * @author	Frank Pagliughi
 * @author  SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	October 2023
 */

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

#ifndef __sockpp_error_h
#define __sockpp_error_h

#include <string>
#include <system_error>

#include "sockpp/platform.h"

namespace sockpp {

/** A sockpp error_code is a std error_code. */
using error_code = std::error_code;

/** A sockpp error_category is a std error_category. */
using error_category = std::error_category;

/** A sockpp error condition is a std error condition  */
using errc = std::errc;
}  // namespace sockpp

/////////////////////////////////////////////////////////////////////////////
// gai_errc & boilerplate for std::error_code

#if !defined(_WIN32)

namespace sockpp {
/**
 * Errors returned from the getaddressinfo() function.
 *
 * These are the possible errors returned from `getaddressinfo()` placed
 * in a class enumeration to give them a string, specific type.
 *
 * Note that this is not used in Windows since it returns errors that
 * are managed by its GetLastError.
 */
enum class gai_errc {
    host_not_found_try_again = EAI_AGAIN,
    invalid_argument = EAI_BADFLAGS,
    no_recovery = EAI_FAIL,
    address_family_not_supported = EAI_FAMILY,
    no_memory = EAI_MEMORY,
    host_not_found = EAI_NONAME,
    addr_family_not_supported = EAI_FAMILY,
    no_addr_in_family = EAI_ADDRFAMILY,
    no_network_add = EAI_NODATA,
    service_not_found = EAI_SERVICE,
    socket_type_not_supported = EAI_SOCKTYPE,
    system_error = EAI_SYSTEM,
};
}  // namespace sockpp

namespace std {
/**
 * Tell the C++ STL metaprogramming that enum `gai_errc` is registered
 * with the standard error code system
 */
template <>
struct is_error_code_enum<::sockpp::gai_errc> : true_type
{
};
}  // namespace std

namespace detail {
/**
 * A custom error code category derived from std::error_category.
 */
class gai_errc_category : public std::error_category
{
public:
    /**
     * Gets short descriptive name for the category.
     */
    const char *name() const noexcept override final { return "GetAddrInfoError"; }

    /**
     * Gets the string representation of the error.
     */
    std::string message(int c) const override final {
        const char *s = gai_strerror(c);
        return s ? std::string(s) : std::string();
    }
};
}  // namespace detail

namespace sockpp {
// Declare a global function returning a static instance of the custom category
const ::detail::gai_errc_category &gai_errc_category();

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline ::std::error_code make_error_code(gai_errc e) {
    if (e == gai_errc::system_error)
        return {errno, std::system_category()};

    return {static_cast<int>(e), gai_errc_category()};
}
}  // namespace sockpp
#endif  // !defined(_WIN32)

#endif  // __sockpp_exception_h
