/**
 * @file tls/mbedtls_acceptor.h
 *
 * mbedTLS implementation of the `tls_acceptor` class.
 *
 * @date May 2026
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

#ifndef __sockpp_tls_mbedtls_acceptor_h
#define __sockpp_tls_mbedtls_acceptor_h

#include "sockpp/acceptor.h"
#include "sockpp/tls/mbedtls_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * A TLS server that accepts incoming connections and performs the TLS
 * handshake as a server.
 *
 * Create a server-role context, load a certificate and private key into it
 * via @c set_identity(), then pass the context to this acceptor.  Each call
 * to @c accept() accepts one TCP connection and completes the TLS handshake,
 * returning a @c tls_socket ready for encrypted I/O.
 */
class tls_acceptor : public acceptor
{
    using base = acceptor;

    /** Non-owning pointer to the TLS context. */
    mbedtls_context* ctx_;

    // Non-copyable
    tls_acceptor(const tls_acceptor&) = delete;
    tls_acceptor& operator=(const tls_acceptor&) = delete;

    /** Common helper: wraps a raw stream socket into a TLS socket. */
    result<tls_socket> wrap(result<stream_socket> raw);

public:
    /**
     * Creates an unbound TLS acceptor.
     * Call @c open() to bind it to an address before accepting connections.
     * @param ctx A server-role TLS context.
     */
    explicit tls_acceptor(mbedtls_context& ctx) : ctx_{&ctx} {}

    /**
     * Creates a TLS acceptor and starts it listening on the specified address.
     * @param ctx A server-role TLS context.
     * @param addr The address to which the acceptor should be bound.
     * @param backlog The listener queue size.
     * @throws std::system_error on bind or listen failure.
     */
    tls_acceptor(mbedtls_context& ctx, const sock_address& addr, int backlog = DFLT_QUE_SIZE);

    /**
     * Creates a TLS acceptor and starts it listening on the specified address
     * (non-throwing).
     * @param ctx A server-role TLS context.
     * @param addr The address to which the acceptor should be bound.
     * @param backlog The listener queue size.
     * @param ec Receives the error code on failure.
     */
    tls_acceptor(
        mbedtls_context& ctx, const sock_address& addr, int backlog, error_code& ec
    ) noexcept;

    /**
     * Move constructor.
     * @param other The acceptor to move into this one.
     */
    tls_acceptor(tls_acceptor&& other) noexcept : base{std::move(other)}, ctx_{other.ctx_} {
        other.ctx_ = nullptr;
    }

    /**
     * Move assignment.
     * @param rhs The acceptor to move into this one.
     * @return A reference to this object.
     */
    tls_acceptor& operator=(tls_acceptor&& rhs) noexcept {
        if (this != &rhs) {
            base::operator=(std::move(rhs));
            ctx_ = rhs.ctx_;
            rhs.ctx_ = nullptr;
        }
        return *this;
    }

    /**
     * Accepts an incoming TCP connection and performs the TLS server
     * handshake.
     * @param peer_addr If non-null, receives the address of the connecting client.
     * @return A @c tls_socket ready for encrypted I/O, or an error code on failure.
     */
    result<tls_socket> accept(sock_address* peer_addr = nullptr);

    /**
     * Accepts an incoming TCP connection with a timeout, then performs the
     * TLS server handshake.
     *
     * The timeout applies only to the TCP accept step; the TLS handshake
     * blocks until complete.
     *
     * @param timeout Maximum time to wait for an incoming TCP connection.
     * @param peer_addr If non-null, receives the address of the connecting client.
     * @return A @c tls_socket ready for encrypted I/O, or an error code on failure.
     */
    result<tls_socket> accept(microseconds timeout, sock_address* peer_addr = nullptr);

    /**
     * Accepts an incoming TCP connection with a duration timeout.
     * @param timeout Maximum time to wait for an incoming TCP connection.
     * @param peer_addr If non-null, receives the address of the connecting client.
     * @return A @c tls_socket ready for encrypted I/O, or an error code on failure.
     */
    template <class Rep, class Period>
    result<tls_socket> accept(
        const duration<Rep, Period>& timeout, sock_address* peer_addr = nullptr
    ) {
        return accept(microseconds(timeout), peer_addr);
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_acceptor_h
