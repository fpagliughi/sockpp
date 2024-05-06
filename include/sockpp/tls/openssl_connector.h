/**
 * @file tls/openssl_connector.h
 *
 * OpenSSL implementation of the `tls_connector` class.
 *
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

#ifndef __sockpp_tls_openssl_connector_h
#define __sockpp_tls_openssl_connector_h

#include "sockpp/connector.h"
#include "sockpp/tls/socket.h"

namespace sockpp {

class tls_connector : public tls_socket
{
    using base = tls_socket;

public:
    /**
     * Creates a TLS connector and connects to the server.
     * @param ctx The TLS context.
     * @param addr The address of the remote server.
     * @throws std::system_error If it fails to find the server
     * @throws tls_error If it fails to make a secure TLS connection
     */
    tls_connector(const tls_context& ctx, const sock_address& addr)
        : base{ctx, connector{addr}} {
        if (auto res = tls_connect(); !res)
            throw tls_error{res.error()};
    }
    /**
     * Creates a TLS connector and connects to the server.
     * @param ctx The TLS context.
     * @param addr The address of the remote server.
     * @param ec Gets the error code on failure
     */
    tls_connector(const tls_context& ctx, const sock_address& addr, error_code& ec) noexcept
        : base{ctx, connector{addr}, ec} {
        if (!ec) {
            if (auto res = tls_connect(); !res)
                ec = res.error();
        }
    }
    /**
     * Creates a new TLS socket from an existing stream socket.
     * @param ctx The TLS context
     * @param sock The insecure socket to wrap.
     * @throws tls_error on failure
     */
    tls_connector(const tls_context& ctx, stream_socket&& sock)
        : base{ctx, std::move(sock)} {}
    /**
     * Creates a new TLS connector from an existing stream socket.
     * @param ctx The TLS context
     * @param sock The insecure socket to wrap.
     * @param ec The error code on failure
     */
    tls_connector(const tls_context& ctx, stream_socket&& sock, error_code& ec) noexcept
        : base{ctx, std::move(sock), ec} {}
    /**
     * Move constructor.
     * @param sock The other TLS connector to move into this one.
     */
    tls_connector(tls_connector&& conn) noexcept : base(std::move(conn)) {}
    /**
     * Destructor.
     */
    ~tls_connector() {}
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    tls_connector& operator=(tls_connector&& rhs) {
        if (&rhs != this) {
            base::operator=(std::move(rhs));
        }
        return *this;
    }
    /**
     * Connect the TLS session.
     * This assumes that the underlying, insecure, connection has already
     * been made.
     * @return The error code on failure.
     */
    result<> tls_connect() noexcept { return tls_check_res_none(::SSL_connect(ssl())); }
    /**
     * Connect the TLS session.
     * @return The error code on failure.
     */
    result<> connect() noexcept { return tls_check_res_none(::SSL_connect(ssl())); }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_openssl_connector_h
