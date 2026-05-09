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

#include <openssl/objects.h>

#include "sockpp/tls/openssl_context.h"
#include "sockpp/tls/openssl_error.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_socket::tls_socket(const tls_context& ctx) : ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_)
        throw tls_error::from_last_error();
}

tls_socket::tls_socket(const tls_context& ctx, error_code& ec) noexcept
    : ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_)
        ec = tls_last_error();
}

tls_socket::tls_socket(const tls_context& ctx, stream_socket&& sock)
    : base{std::move(sock)}, ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_)
        throw tls_error::from_last_error();

    if (::SSL_set_fd(ssl_, handle()) <= 0) {
        auto err = tls_error::from_last_error();
        ::SSL_free(ssl_);
        ssl_ = nullptr;
        throw err;
    }
}

tls_socket::tls_socket(const tls_context& ctx, stream_socket&& sock, error_code& ec) noexcept
    : base{std::move(sock)}, ssl_{::SSL_new(ctx.ctx_)} {
    if (!ssl_) {
        ec = tls_last_error();
    }
    else if (::SSL_set_fd(ssl_, handle()) <= 0) {
        ec = tls_last_error();
        ::SSL_free(ssl_);
        ssl_ = nullptr;
    }
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

result<> tls_socket::attach(stream_socket&& sock) noexcept {
    base::operator=(std::move(sock));
    return tls_check_res_none(::SSL_set_fd(ssl_, handle()));
}

std::optional<tls_certificate> tls_socket::peer_certificate() {
    if (X509* cert = SSL_get1_peer_certificate(ssl_); cert != nullptr)
        return tls_certificate{cert};
    return std::nullopt;
}

tls_certificate_chain tls_socket::peer_certificate_chain() {
    tls_certificate_chain chain;

    // Grab the leaf cert first (always correct for both client and server).
    X509* leaf_raw = ::SSL_get1_peer_certificate(ssl_);
    if (leaf_raw)
        chain.push_back(tls_certificate{leaf_raw});

    // SSL_get_peer_cert_chain() returns the chain as sent by the peer.
    // On the client side it includes the leaf (sk[0] == leaf_raw pointer);
    // on the server side it omits the leaf.  Skip any entry that matches
    // the leaf pointer to avoid duplicates.
    STACK_OF(X509)* sk = ::SSL_get_peer_cert_chain(ssl_);
    if (!sk)
        return chain;

    int n = ::sk_X509_num(sk);
    for (int i = 0; i < n; i++) {
        X509* cert = ::sk_X509_value(sk, i);
        if (!cert || cert == leaf_raw)
            continue;
        ::X509_up_ref(cert);
        chain.push_back(tls_certificate{cert});
    }
    return chain;
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

uint32_t tls_socket::peer_certificate_status() {
    return (uint32_t)::SSL_get_verify_result(ssl_);
}

string tls_socket::peer_certificate_status_message() {
    return string{::X509_verify_cert_error_string(::SSL_get_verify_result(ssl_))};
}

result<> tls_socket::set_host_name(const string& hostname) {
    return tls_check_res_none(::SSL_set_tlsext_host_name(ssl_, hostname.c_str()));
}

result<> tls_socket::auto_retry(bool on /*=true*/) {
    long ret;
    if (on) {
        ret = ::SSL_set_mode(ssl_, SSL_MODE_AUTO_RETRY);
    }
    else {
        ret = ::SSL_clear_mode(ssl_, SSL_MODE_AUTO_RETRY);
    }
    return tls_check_res_none(ret);
}

result<size_t> tls_socket::read(void* buf, size_t n) {
    size_t nx;
    int ret = ::SSL_read_ex(ssl_, buf, n, &nx);
    return tls_check_io(ret, nx);
}

result<> tls_socket::read_timeout(const microseconds& to) {
    return stream_socket::read_timeout(to);
}

result<size_t> tls_socket::write(const void* buf, size_t n) {
    size_t nx;
    int ret = ::SSL_write_ex(ssl_, buf, n, &nx);
    return tls_check_io(ret, nx);
}

result<> tls_socket::write_timeout(const microseconds& to) {
    return stream_socket::write_timeout(to);
}

bool tls_socket::received_shutdown() {
    return (::SSL_get_shutdown(ssl_) & SSL_RECEIVED_SHUTDOWN) == SSL_RECEIVED_SHUTDOWN;
}

result<> tls_socket::send_close_notify() {
    if (!ssl_)
        return {};
    int ret = ::SSL_shutdown(ssl_);
    // ret == 1: bidirectional shutdown complete
    // ret == 0: our close_notify sent; peer's not yet received — success for us
    // ret < 0: error
    if (ret < 0)
        return tls_last_error();
    return {};
}

string tls_socket::negotiated_version() const {
    if (!ssl_)
        return {};
    const char* v = ::SSL_get_version(ssl_);
    return (v && *v && strcmp(v, "unknown") != 0) ? v : string{};
}

string tls_socket::negotiated_cipher() const {
    if (!ssl_)
        return {};
    const char* c = ::SSL_get_cipher(ssl_);
    return (c && *c && strcmp(c, "(NONE)") != 0) ? c : string{};
}

string tls_socket::negotiated_group() const {
    if (!ssl_)
        return {};
    int nid = (int)::SSL_get_negotiated_group(ssl_);
    if (nid <= 0)
        return {};
    const char* name = ::OBJ_nid2sn(nid);
    return name ? name : string{};
}

string tls_socket::negotiated_alpn_protocol() const {
    if (!ssl_)
        return {};
    const unsigned char* proto = nullptr;
    unsigned int proto_len = 0;
    ::SSL_get0_alpn_selected(ssl_, &proto, &proto_len);
    return (proto && proto_len > 0) ? string{reinterpret_cast<const char*>(proto), proto_len}
                                    : string{};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
