/**
 * @file tls/mbedtls_connector.h
 *
 * mbedTLS implementation of the `tls_connector` class.
 *
 * @author Frank Pagliughi
 * @date 2026
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2026 Frank Pagliughi
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

#ifndef __sockpp_tls_mbedtls_connector_h
#define __sockpp_tls_mbedtls_connector_h

#include "sockpp/sock_address.h"
#include "sockpp/tls/mbedtls_socket.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * A TLS client socket that connects to a remote server.
 *
 * Inherits all I/O from @ref mbedtls_socket.  Constructors perform the
 * TCP connection and TLS handshake in one step; @ref connect() and
 * @ref tls_connect() allow deferred or two-phase connection.
 */
class tls_connector : public mbedtls_socket
{
    using base = mbedtls_socket;

    // Non-copyable
    tls_connector(const tls_connector&) = delete;
    tls_connector& operator=(const tls_connector&) = delete;

public:
    /**
     * Creates an unconnected TLS connector.
     * Call @ref connect() or @ref tls_connect() to establish a connection.
     * @param ctx The mbedTLS context.
     * @throws tls_error on SSL context initialisation failure.
     */
    explicit tls_connector(mbedtls_context& ctx) : base{ctx, string{}} {}

    /**
     * Creates an unconnected TLS connector (non-throwing).
     * @param ctx The mbedTLS context.
     * @param ec Receives the error code on failure.
     */
    tls_connector(mbedtls_context& ctx, error_code& ec) noexcept
        : base{ctx, string{}, ec} {}

    /**
     * Creates a TLS connector and attempts to connect to the server.
     * @param ctx The mbedTLS context.
     * @param addr The address of the remote server.
     * @throws std::system_error on TCP connection failure.
     * @throws tls_error on TLS handshake failure.
     */
    tls_connector(mbedtls_context& ctx, const sock_address& addr);

    /**
     * Creates a TLS connector, connects, and presents @p hostname for SNI.
     * @param ctx The mbedTLS context.
     * @param addr The address of the remote server.
     * @param hostname The SNI host name to verify against the server certificate.
     * @throws std::system_error on TCP connection failure.
     * @throws tls_error on TLS handshake failure.
     */
    tls_connector(mbedtls_context& ctx, const sock_address& addr, const string& hostname);

    /**
     * Creates a TLS connector, connects, and presents @p hostname for SNI
     * (non-throwing).
     * @param ctx The mbedTLS context.
     * @param addr The address of the remote server.
     * @param hostname The SNI host name (may be empty).
     * @param ec Receives the error code on failure.
     */
    tls_connector(
        mbedtls_context& ctx, const sock_address& addr, const string& hostname,
        error_code& ec
    ) noexcept;

    /**
     * Creates a TLS connector by wrapping an existing stream socket.
     * Performs the TLS handshake immediately.
     * @param ctx The mbedTLS context.
     * @param sock The connected, insecure stream socket.
     * @throws tls_error on handshake failure.
     */
    tls_connector(mbedtls_context& ctx, stream_socket&& sock)
        : base{std::move(sock), ctx, string{}} {}

    /**
     * Creates a TLS connector by wrapping an existing stream socket (non-throwing).
     * @param ctx The mbedTLS context.
     * @param sock The connected, insecure stream socket.
     * @param ec Receives the error code on failure.
     */
    tls_connector(mbedtls_context& ctx, stream_socket&& sock, error_code& ec) noexcept;

    /**
     * Move constructor.
     * @param other The connector to move into this one.
     */
    tls_connector(tls_connector&& other) noexcept : base(std::move(other)) {}

    /**
     * Destructor.
     */
    ~tls_connector() {}

    /**
     * Attempts to connect to the specified server and run the TLS handshake.
     * Uses the hostname set during construction (if any) for SNI.
     * @param addr The remote server address.
     * @return An error code on failure, or an empty (success) result.
     */
    result<> connect(const sock_address& addr) noexcept;

    /**
     * Attempts to connect to the specified server with a timeout, then runs
     * the TLS handshake.
     * @param addr The remote server address.
     * @param timeout The duration after which to give up. Zero means never.
     * @return An error code on failure, or an empty (success) result.
     */
    result<> connect(const sock_address& addr, microseconds timeout) noexcept;

    /**
     * Attempts to connect with a duration timeout (template overload).
     * @param addr The remote server address.
     * @param relTime The duration after which to give up.
     * @return An error code on failure, or an empty (success) result.
     */
    template <class Rep, class Period>
    result<> connect(
        const sock_address& addr, const duration<Rep, Period>& relTime
    ) noexcept {
        return connect(addr, microseconds(relTime));
    }

    /**
     * Runs the TLS handshake on the currently attached underlying socket.
     * @return An error code on failure, or an empty (success) result.
     */
    result<> tls_connect() noexcept { return base::tls_connect(); }

    /**
     * Attaches @p sock as the underlying stream socket and runs the TLS handshake.
     * @param sock A connected, insecure stream socket.
     * @return An error code on failure, or an empty (success) result.
     */
    result<> tls_connect(stream_socket&& sock) noexcept {
        return base::tls_connect(std::move(sock));
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_connector_h
