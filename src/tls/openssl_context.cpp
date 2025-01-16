// openssl_context.cpp
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

#include "sockpp/tls/openssl_context.h"

#include "sockpp/tls/openssl_socket.h"

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_context::tls_context(role_t role /*=role_t::CLIENT*/) : role_{role} {
    const SSL_METHOD* method = nullptr;
    switch (role) {
        case role_t::DEFAULT:
        case role_t::CLIENT:
            method = ::TLS_client_method();
            break;
        case role_t::SERVER:
            method = ::TLS_server_method();
            break;
        case role_t::BOTH:
            method = ::TLS_method();
            break;
            //        default:
            //            set_status(-1);
    }

    if (method) {
        ctx_ = ::SSL_CTX_new(method);
        ::SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);
    }
}

tls_context::~tls_context() {
    if (ctx_)
        ::SSL_CTX_free(ctx_);
}

tls_context& tls_context::default_context() {
    static tls_context ctx{role_t::CLIENT};
    return ctx;
}

tls_context& tls_context::operator=(tls_context&& rhs) {
    if (&rhs != this) {
        std::swap(ctx_, rhs.ctx_);
        role_ = rhs.role_;
    }
    return *this;
}

result<> tls_context::set_default_trust_locations() {
    return tls_check_res_none(::SSL_CTX_set_default_verify_paths(ctx_));
}

result<> tls_context::set_trust_locations(
    const std::optional<string>& caFile, const std::optional<string>& caPath /*=std::nullopt*/
) {
    return tls_check_res_none(
        ::SSL_CTX_load_verify_locations(
            ctx_, caFile ? caFile.value().c_str() : nullptr,
            caPath ? caPath.value().c_str() : nullptr
        )
    );
}

void tls_context::set_verify(verify_t mode) noexcept {
    int vmode = SSL_VERIFY_NONE;
    switch (mode) {
        case verify_t::PEER:
            vmode = SSL_VERIFY_PEER;
            break;

        case verify_t::NONE:
        default:
            vmode = SSL_VERIFY_NONE;
            break;
    }
    ::SSL_CTX_set_verify(ctx_, vmode, nullptr);
}

result<> tls_context::set_cert_file(const string& certFile) {
    return tls_check_res_none(::SSL_CTX_use_certificate_chain_file(ctx_, certFile.c_str()));
}

result<> tls_context::set_key_file(const string& keyFile) {
    return tls_check_res_none(
        ::SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM)
    );
}

void tls_context::set_root_certs(const string& certData) {
    // TODO: Implement
    (void)certData;
}

void tls_context::require_peer_cert(role_t role, bool require, bool sendCAList) {
    // TODO: Implement
    (void)role;
    (void)require;
    (void)sendCAList;
}

void tls_context::allow_only_certificate(const std::string& certData) {
    // TODO: Implement
    (void)certData;
}

#if 0
void openssl_contest::set_auth_callback(auth_callback cb)
{
	auth_callback_ = std::move(cb);
}

const tls_context::auth_callback& get_auth_callback() const
{
	return auth_callback_;
}
#endif

void tls_context::set_identity(
    const string& certificate_data, const string& private_key_data
) {
    // TODO: Implement
    (void)certificate_data;
    (void)private_key_data;
}

tls_socket tls_context::wrap_socket(
    stream_socket&& sock,
    // role_t role /*=0*/,
    const string& peer_name /*=string()*/
) {
    tls_socket tls_sock{*this, std::move(sock)};
    if (!peer_name.empty()) {
        // TODO: implement (verify peer)
    }
    return tls_sock;
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
