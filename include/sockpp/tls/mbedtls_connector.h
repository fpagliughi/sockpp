/**
 * @file tls/mbedtls_connector.h
 *
 * mbedTLS implementation of the `tls_connector` class.
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

#ifndef __sockpp_tls_mbedtls_connector_h
#define __sockpp_tls_mbedtls_connector_h

#include <memory>
#include <string>

#include "sockpp/connector.h"
#include "sockpp/result.h"
#include "sockpp/tls/mbedtls_context.h"
#include "sockpp/tls/mbedtls_socket.h"
#include "sockpp/tls/tls_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * A TLS client connector for the mbedTLS backend.
 *
 * Manages the lifecycle of an @ref mbedtls_socket.  Call @ref tls_connect()
 * with an already-connected stream socket to perform the TLS handshake, then
 * use @ref read() and @ref write() for I/O.
 */
class tls_connector
{
    mbedtls_context& ctx_;
    string hostname_;
    std::unique_ptr<mbedtls_socket> sock_;

    // Non-copyable
    tls_connector(const tls_connector&) = delete;
    tls_connector& operator=(const tls_connector&) = delete;

public:
    /**
     * Creates an unconnected TLS connector.
     * @param ctx The mbedTLS context.
     */
    explicit tls_connector(mbedtls_context& ctx) : ctx_{ctx} {}

    /**
     * Move constructor.
     * @param other The connector to move into this one.
     */
    tls_connector(tls_connector&& other) noexcept
        : ctx_{other.ctx_}, hostname_{std::move(other.hostname_)},
          sock_{std::move(other.sock_)} {}

    /**
     * Sets the SNI host name for the TLS handshake.
     * Must be called before @ref tls_connect().
     * @param hostname The server's host name.
     */
    result<> set_host_name(const string& hostname) {
        hostname_ = hostname;
        return {};
    }

    /**
     * Wraps an existing TCP stream socket in TLS.
     * Performs the TLS handshake immediately.
     * @param sock The connected, insecure stream socket to wrap.
     * @return Error code on failure.
     */
    result<> tls_connect(stream_socket&& sock) noexcept;

    /**
     * Connects to the specified address and performs the TLS handshake.
     * @param addr The remote server address.
     * @return Error code on failure.
     */
    result<> connect(const sock_address& addr) noexcept;

    /** Returns the underlying TLS socket, if connected. */
    mbedtls_socket* socket() { return sock_.get(); }

    /** Returns @em true if the connector has a live TLS socket. */
    bool is_connected() const { return sock_ != nullptr && sock_->is_open(); }

    // ---- I/O passthrough ----

    result<size_t> read(void* buf, size_t n) {
        return sock_ ? sock_->read(buf, n) : result<size_t>{std::errc::not_connected};
    }

    result<size_t> write(const void* buf, size_t n) {
        return sock_ ? sock_->write(buf, n) : result<size_t>{std::errc::not_connected};
    }

    result<> close() {
        if (sock_)
            return sock_->close();
        return {};
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_connector_h
