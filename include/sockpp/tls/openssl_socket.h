/**
 * @file openssl_socket.h
 *
 * Socket type for OpenSSL TLS/SSL sockets.
 *
 * @author Frank Pagliughi
 * @date February 2023
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023-2024 Frank Pagliughi
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

#ifndef __sockpp_tls_openssl_socket_h
#define __sockpp_tls_openssl_socket_h

#include <openssl/ssl.h>

#include <optional>

#include "sockpp/stream_socket.h"
#include "sockpp/tls/openssl_certificate.h"
#include "sockpp/tls/openssl_context.h"
#include "sockpp/tls/openssl_error.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * A secure TLS socket implemented with OpenSSL.
 */
class tls_socket : public stream_socket
{
    /** The base class */
    using base = stream_socket;

    /** The SSL structure */
    SSL* ssl_;

    /**
     * Checks the return value from an OpenSSL I/O function and return a
     * result for the success or failure.
     * @param ret The return value from the function
     * @param nx The number of bytes read/written on success.
     * @return The result of the operation
     */
    result<size_t> tls_check_io(int ret, size_t nx) {
        return (ret <= 0) ? result<size_t>{tls_last_error()} : result<size_t>{nx};
    }

    // Non-copyable.
    tls_socket(const socket&) = delete;
    tls_socket& operator=(const socket&) = delete;

public:
    /**
     * Creates a new TLS socket from an existing stream socket.
     * @param ctx The TLS context
     * @param sock The insecure socket to wrap.
     * @throws tls_error on failure
     */
    tls_socket(const tls_context& ctx, stream_socket&& sock);
    /**
     * Creates a new TLS socket from an existing stream socket.
     * @param ctx The TLS context
     * @param sock The insecure socket to wrap.
     * @param ec The error code on failure
     */
    tls_socket(const tls_context& ctx, stream_socket&& sock, error_code& ec) noexcept;
    /**
     * Move constructor.
     * @param sock The other TLS socket to move into this one.
     */
    tls_socket(tls_socket&& sock) noexcept : base(std::move(sock)), ssl_{sock.ssl_} {
        sock.ssl_ = nullptr;
    }
    /**
     * Destructor.
     */
    ~tls_socket();
    /**
     * Gets the underlying OpenSSL `SSL` struct for the socket.
     * @return The underlying OpenSSL `SSL` struct for the socket. Note that
     *         this may be null if the socket failed on construction.
     */
    SSL* ssl() { return ssl_; }
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    tls_socket& operator=(tls_socket&& rhs);

    /**
     * Returns the peer's X.509 certificate.
     */
    std::optional<tls_certificate> peer_certificate();

#if 0
    /**
     *
     */
    uint32_t peer_certificate_status();
    /**
     * Returns an error message describing any problem with the peer's
     * certificate.
     */
    string peer_certificate_status_message();
#endif

    // I/O primitives

    using base::read;
    result<size_t> read(void* buf, size_t n);
    result<> read_timeout(const microseconds& to);

    using base::write;
    result<size_t> write(const void* buf, size_t n);
    result<size_t> write(const std::vector<iovec>& ranges) {
        return ranges.empty() ? 0 : write(ranges[0].iov_base, ranges[0].iov_len);
    }
    result<> write_timeout(const microseconds& to);

    result<> set_non_blocking(bool on) {
        // TODO: Implement
        (void)on;
        return none{};
    }

    /**
     * Determines if the socket received a shutdown from the peer.
     * @return @em true if the socet received a shutdown from the peer, @em
     *         false otherwise.
     */
    bool received_shutdown();
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_openssl_socket_h
