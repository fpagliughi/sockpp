// openssl_socket.cpp
//
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

#include "sockpp/tls/openssl_socket.h"

#include "sockpp/tls/openssl_context.h"
#include "sockpp/tls/openssl_error.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_socket::tls_socket(const tls_context& ctx, stream_socket&& sock)
    : base{std::move(sock)}, ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_)
        throw tls_error::from_last_error();

    if (auto res = tls_check_res(::SSL_set_fd(ssl_, handle())); !res)
        throw res;
}

tls_socket::tls_socket(const tls_context& ctx, stream_socket&& sock, error_code& ec) noexcept
    : base{std::move(sock)}, ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_ || ::SSL_set_fd(ssl_, handle()) <= 0)
        ec = tls_last_error();
}

tls_socket::~tls_socket() {
    if (ssl_)
        ::SSL_free(ssl_);
}

tls_socket& tls_socket::operator=(tls_socket&& rhs) {
    if (&rhs != this) {
        base::operator=(std::move(rhs));
        ssl_ = rhs.ssl_;

        rhs.ssl_ = nullptr;
    }
    return *this;
}

std::optional<tls_certificate> tls_socket::peer_certificate() {
    // TODO: Implement this
    X509* cert = SSL_get1_peer_certificate(ssl_);

    if (!cert)
        return std::nullopt;

    return tls_certificate{cert};
}

#if 0
uint32_t tls_socket::peer_certificate_status() {
    // TODO: Implement this?
    return 0;
}

// Returns an error message describing any problem with
// the peer's certificate.
string tls_socket::peer_certificate_status_message() {
    // TODO: Implement this?
    return string{};
}
#endif

result<size_t> tls_socket::read(void* buf, size_t n) {
    size_t nx;
    int ret = ::SSL_read_ex(ssl_, buf, n, &nx);
    return tls_check_io(ret, nx);
}

result<> tls_socket::read_timeout(const microseconds& to) {
    // TODO: Is this adequate?
    return stream_socket::read_timeout(to);
}

result<size_t> tls_socket::write(const void* buf, size_t n) {
    size_t nx;
    int ret = ::SSL_write_ex(ssl_, buf, n, &nx);
    return tls_check_io(ret, nx);
}

result<> tls_socket::write_timeout(const microseconds& to) {
    // TODO: Is this adequate?
    return stream_socket::write_timeout(to);
}

bool tls_socket::received_shutdown() {
    return (::SSL_get_shutdown(ssl_) & SSL_RECEIVED_SHUTDOWN) == SSL_RECEIVED_SHUTDOWN;
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
