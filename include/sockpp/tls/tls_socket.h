/**
 * @file tls/tls_socket.h
 *
 * Abstract base class for TLS socket implementations (mbedTLS path).
 *
 * @author Frank Pagliughi
 * @date 2024
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

#ifndef __sockpp_tls_tls_socket_iface_h
#define __sockpp_tls_tls_socket_iface_h

#include <cstdint>
#include <string>

#include "sockpp/stream_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for TLS socket implementations.
 *
 * Derives from @ref stream_socket and adds the TLS-specific certificate
 * inspection API. Concrete implementations (e.g., @ref mbedtls_socket)
 * inherit from this class.
 *
 * The public @ref tls_socket name is an alias set to this class:
 *   `using tls_socket = tls_socket_iface;`
 */
class tls_socket_iface : public stream_socket
{
public:
    using stream_socket::stream_socket;

    /** Constructs a TLS socket by taking ownership of an existing stream socket. */
    explicit tls_socket_iface(stream_socket&& sock) noexcept
        : stream_socket(std::move(sock)) {}

    virtual ~tls_socket_iface() = default;

    /**
     * Returns the raw peer-certificate verification result flags.
     * A return value of zero means the peer certificate was accepted.
     */
    virtual uint32_t peer_certificate_status() = 0;

    /**
     * Returns a human-readable description of the peer-certificate
     * verification result, or an empty string on success.
     */
    virtual string peer_certificate_status_message() = 0;

    /**
     * Returns the DER-encoded certificate received from the peer, or an
     * empty string if none is available.
     */
    virtual string peer_certificate() = 0;
};

/**
 * For the mbedTLS backend, @c tls_socket is an alias for the abstract
 * base so that @ref mbedtls_context::wrap_socket() can return
 * `std::unique_ptr<tls_socket>` while the concrete object is a
 * @ref mbedtls_socket.
 */
using tls_socket = tls_socket_iface;

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_tls_socket_iface_h
