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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <memory>

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

void tls_context::set_auto_retry(bool on /*=true*/) noexcept {
    if (on)
        ::SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);
    else
        ::SSL_CTX_clear_mode(ctx_, SSL_MODE_AUTO_RETRY);
}

result<> tls_context::set_cert_file(const string& certFile) {
    return tls_check_res_none(::SSL_CTX_use_certificate_chain_file(ctx_, certFile.c_str()));
}

result<> tls_context::set_key_file(const string& keyFile) {
    return tls_check_res_none(
        ::SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM)
    );
}

result<> tls_context::set_root_certs(const string& certData) {
    auto bio_deleter = [](BIO* b) { ::BIO_free(b); };
    std::unique_ptr<BIO, decltype(bio_deleter)> bio{
        ::BIO_new_mem_buf(certData.data(), static_cast<int>(certData.size())), bio_deleter
    };
    if (!bio)
        return tls_last_error();

    // Build a fresh store to replace the existing one (true "set" semantics).
    auto store_deleter = [](X509_STORE* s) { ::X509_STORE_free(s); };
    std::unique_ptr<X509_STORE, decltype(store_deleter)> store{
        ::X509_STORE_new(), store_deleter
    };
    if (!store)
        return tls_last_error();

    ::ERR_clear_error();

    int loaded = 0;
    auto cert_deleter = [](X509* c) { ::X509_free(c); };

    for (;;) {
        // PEM_read_bio_X509_AUX handles both CERTIFICATE and TRUSTED CERTIFICATE blocks.
        std::unique_ptr<X509, decltype(cert_deleter)> cert{
            ::PEM_read_bio_X509_AUX(bio.get(), nullptr, nullptr, nullptr), cert_deleter
        };

        if (!cert) {
            // PEM_R_NO_START_LINE is the benign end-of-input sentinel; anything
            // else is a genuine parse error.
            unsigned long err = ::ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
                ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
                ::ERR_clear_error();
                break;
            }
            return tls_last_error();
        }

        if (::X509_STORE_add_cert(store.get(), cert.get()) != 1) {
            unsigned long err = ::ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_X509 &&
                ERR_GET_REASON(err) == X509_R_CERT_ALREADY_IN_HASH_TABLE) {
                ::ERR_clear_error();  // duplicates in CA bundles are harmless
            }
            else {
                return tls_last_error();
            }
        }
        else {
            ++loaded;
        }
    }

    if (loaded == 0)
        return errc::invalid_argument;

    // Install the new store; SSL_CTX_set_cert_store takes ownership.
    ::SSL_CTX_set_cert_store(ctx_, store.release());

    return none{};
}

void tls_context::require_peer_cert(bool require, bool sendCAList /*=false*/) {
    // SSL_VERIFY_PEER alone requests the peer cert and verifies it if provided.
    // On a server, SSL_VERIFY_FAIL_IF_NO_PEER_CERT makes the handshake fail
    // when the client sends no cert; without it the connection silently succeeds.
    int vmode = SSL_VERIFY_NONE;
    if (require) {
        vmode = SSL_VERIFY_PEER;
        if (role_ == role_t::SERVER || role_ == role_t::BOTH)
            vmode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    ::SSL_CTX_set_verify(ctx_, vmode, nullptr);

    // Advertise trusted CA names to the client in the TLS CertificateRequest
    // message (server only, and only meaningful when requiring a client cert).
    if (sendCAList && require && (role_ == role_t::SERVER || role_ == role_t::BOTH)) {
        X509_STORE* store = ::SSL_CTX_get_cert_store(ctx_);
        if (store) {
            STACK_OF(X509_NAME)* ca_names = sk_X509_NAME_new_null();
            if (ca_names) {
                STACK_OF(X509_OBJECT)* objs = ::X509_STORE_get0_objects(store);
                int n = sk_X509_OBJECT_num(objs);
                for (int i = 0; i < n; ++i) {
                    X509_OBJECT* obj = sk_X509_OBJECT_value(objs, i);
                    X509* cert = ::X509_OBJECT_get0_X509(obj);
                    if (!cert)
                        continue;
                    X509_NAME* name = ::X509_NAME_dup(::X509_get_subject_name(cert));
                    if (name && sk_X509_NAME_push(ca_names, name) == 0)
                        ::X509_NAME_free(name);
                }
                if (sk_X509_NAME_num(ca_names) > 0)
                    ::SSL_CTX_set_client_CA_list(ctx_, ca_names);
                else
                    sk_X509_NAME_pop_free(ca_names, ::X509_NAME_free);
            }
        }
    }
}

int tls_context::pin_verify_cb(X509_STORE_CTX* store_ctx, void* arg) {
    // Perform standard chain verification first.
    if (!::X509_verify_cert(store_ctx))
        return 0;

    auto* self = static_cast<tls_context*>(arg);

    // Walk the verified chain looking for the pinned certificate.
    STACK_OF(X509)* chain = ::X509_STORE_CTX_get0_chain(store_ctx);
    int n = sk_X509_num(chain);
    for (int i = 0; i < n; ++i) {
        if (::X509_cmp(sk_X509_value(chain, i), self->pinned_cert_->cert_) == 0)
            return 1;
    }

    // No cert in the chain matched the pin.
    ::X509_STORE_CTX_set_error(store_ctx, X509_V_ERR_CERT_REJECTED);
    return 0;
}

void tls_context::allow_only_certificate(const tls_certificate& cert) {
    if (cert.is_valid()) {
        pinned_cert_ = cert;
        ::SSL_CTX_set_cert_verify_callback(ctx_, pin_verify_cb, this);
    }
    else {
        pinned_cert_.reset();
        ::SSL_CTX_set_cert_verify_callback(ctx_, nullptr, nullptr);
    }
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

result<> tls_context::set_identity(const string& cert_pem, const string& key_pem) {
    auto bio_deleter = [](BIO* b) { ::BIO_free(b); };
    auto cert_deleter = [](X509* c) { ::X509_free(c); };

    // --- Certificate chain ---

    std::unique_ptr<BIO, decltype(bio_deleter)> cert_bio{
        ::BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())), bio_deleter
    };
    if (!cert_bio)
        return tls_last_error();

    // Load the leaf certificate.
    std::unique_ptr<X509, decltype(cert_deleter)> leaf{
        ::PEM_read_bio_X509_AUX(cert_bio.get(), nullptr, nullptr, nullptr), cert_deleter
    };
    if (!leaf)
        return tls_last_error();

    if (::SSL_CTX_use_certificate(ctx_, leaf.get()) != 1)
        return tls_last_error();

    // Load any intermediate certs that follow in the PEM string.
    ::SSL_CTX_clear_extra_chain_certs(ctx_);
    ::ERR_clear_error();

    for (;;) {
        std::unique_ptr<X509, decltype(cert_deleter)> ca{
            ::PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr), cert_deleter
        };
        if (!ca) {
            unsigned long err = ::ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
                ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
                ::ERR_clear_error();
                break;
            }
            return tls_last_error();
        }
        // SSL_CTX_add_extra_chain_cert takes ownership on success.
        if (::SSL_CTX_add_extra_chain_cert(ctx_, ca.get()) != 1)
            return tls_last_error();
        ca.release();
    }

    // --- Private key ---

    std::unique_ptr<BIO, decltype(bio_deleter)> key_bio{
        ::BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size())), bio_deleter
    };
    if (!key_bio)
        return tls_last_error();

    auto key_deleter = [](EVP_PKEY* k) { ::EVP_PKEY_free(k); };
    std::unique_ptr<EVP_PKEY, decltype(key_deleter)> key{
        ::PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr), key_deleter
    };
    if (!key)
        return tls_last_error();

    if (::SSL_CTX_use_PrivateKey(ctx_, key.get()) != 1)
        return tls_last_error();

    // Verify that the certificate and private key are consistent.
    return tls_check_res_none(::SSL_CTX_check_private_key(ctx_));
}

result<tls_socket> tls_context::wrap_socket(
    stream_socket&& sock, const string& peer_name /*=string()*/
) {
    error_code ec;
    tls_socket tls_sock{*this, std::move(sock), ec};
    if (ec)
        return ec;

    if (!peer_name.empty()) {
        // SNI: tells the server which hostname we're connecting to so it can
        // select the right certificate when it hosts multiple domains.
        if (auto res = tls_sock.set_host_name(peer_name); !res)
            return res.error();

        // Automatic hostname verification: OpenSSL will check that the
        // server's certificate actually matches the requested hostname.
        if (::SSL_set1_host(tls_sock.ssl(), peer_name.c_str()) != 1)
            return tls_last_error();
    }

    // Perform the TLS handshake.
    int ret = (role_ == role_t::SERVER) ? ::SSL_accept(tls_sock.ssl())
                                        : ::SSL_connect(tls_sock.ssl());
    if (ret != 1)
        return tls_last_error();

    return tls_sock;
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
