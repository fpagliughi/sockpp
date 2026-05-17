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

#include <cstring>
#include <limits>
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
            method = TLS_client_method();
            break;
        case role_t::SERVER:
            method = TLS_server_method();
            break;
        case role_t::BOTH:
            method = TLS_method();
            break;
            //        default:
            //            set_status(-1);
    }

    if (method) {
        ctx_ = SSL_CTX_new(method);
        SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);
        SSL_CTX_set_app_data(ctx_, this);
    }
}

tls_context::tls_context(tls_context&& ctx) noexcept
    : ctx_{ctx.ctx_},
      role_{ctx.role_},
      auth_callback_{std::move(ctx.auth_callback_)},
      pinned_cert_{std::move(ctx.pinned_cert_)},
      alpn_wire_{std::move(ctx.alpn_wire_)},
      psk_identity_{std::move(ctx.psk_identity_)},
      psk_key_{std::move(ctx.psk_key_)},
      psk_server_cb_{std::move(ctx.psk_server_cb_)} {
    ctx.ctx_ = nullptr;
    // Re-point callbacks whose arg is `this`.
    SSL_CTX_set_app_data(ctx_, this);
    if (!alpn_wire_.empty())
        SSL_CTX_set_alpn_select_cb(ctx_, &tls_context::alpn_select_cb, this);
}

tls_context::~tls_context() {
    if (ctx_)
        SSL_CTX_free(ctx_);
}

tls_context& tls_context::default_context() {
    static tls_context ctx{role_t::CLIENT};
    return ctx;
}

tls_context& tls_context::operator=(tls_context&& rhs) {
    if (&rhs != this) {
        std::swap(ctx_, rhs.ctx_);
        role_ = rhs.role_;
        auth_callback_ = std::move(rhs.auth_callback_);
        pinned_cert_ = std::move(rhs.pinned_cert_);
        alpn_wire_ = std::move(rhs.alpn_wire_);
        psk_identity_ = std::move(rhs.psk_identity_);
        psk_key_ = std::move(rhs.psk_key_);
        psk_server_cb_ = std::move(rhs.psk_server_cb_);
        // Re-point callbacks whose arg is `this`.
        if (ctx_) {
            SSL_CTX_set_app_data(ctx_, this);
            if (!alpn_wire_.empty())
                SSL_CTX_set_alpn_select_cb(ctx_, &tls_context::alpn_select_cb, this);
        }
    }
    return *this;
}

result<> tls_context::set_default_trust_locations() {
    return tls_check_res_none(SSL_CTX_set_default_verify_paths(ctx_));
}

result<> tls_context::set_trust_locations(
    const std::optional<string>& caFile, const std::optional<string>& caPath /*=std::nullopt*/
) {
    return tls_check_res_none(SSL_CTX_load_verify_locations(
        ctx_, caFile ? caFile.value().c_str() : nullptr,
        caPath ? caPath.value().c_str() : nullptr
    ));
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
    SSL_CTX_set_verify(ctx_, vmode, nullptr);
}

void tls_context::set_auto_retry(bool on /*=true*/) noexcept {
    if (on)
        SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);
    else
        SSL_CTX_clear_mode(ctx_, SSL_MODE_AUTO_RETRY);
}

result<> tls_context::set_cert_file(const string& certFile) {
    return tls_check_res_none(SSL_CTX_use_certificate_chain_file(ctx_, certFile.c_str()));
}

result<> tls_context::set_key_file(const string& keyFile) {
    return tls_check_res_none(
        SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM)
    );
}

result<> tls_context::set_root_certs(const string& certData) {
    if (certData.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return make_error_code(std::errc::value_too_large);

    auto bio_deleter = [](BIO* b) { BIO_free(b); };
    unique_ptr<BIO, decltype(bio_deleter)> bio{
        BIO_new_mem_buf(certData.data(), static_cast<int>(certData.size())), bio_deleter
    };
    if (!bio)
        return tls_last_error();

    // Build a fresh store to replace the existing one (true "set" semantics).
    auto store_deleter = [](X509_STORE* s) { X509_STORE_free(s); };
    unique_ptr<X509_STORE, decltype(store_deleter)> store{X509_STORE_new(), store_deleter};
    if (!store)
        return tls_last_error();

    ERR_clear_error();

    int loaded = 0;
    auto cert_deleter = [](X509* c) { X509_free(c); };

    for (;;) {
        // PEM_read_bio_X509_AUX handles both CERTIFICATE and TRUSTED CERTIFICATE blocks.
        unique_ptr<X509, decltype(cert_deleter)> cert{
            PEM_read_bio_X509_AUX(bio.get(), nullptr, nullptr, nullptr), cert_deleter
        };

        if (!cert) {
            // PEM_R_NO_START_LINE is the benign end-of-input sentinel; anything
            // else is a genuine parse error.
            unsigned long err = ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
                ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
                ERR_clear_error();
                break;
            }
            return tls_last_error();
        }

        if (X509_STORE_add_cert(store.get(), cert.get()) != 1) {
            unsigned long err = ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_X509 &&
                ERR_GET_REASON(err) == X509_R_CERT_ALREADY_IN_HASH_TABLE) {
                ERR_clear_error();  // duplicates in CA bundles are harmless
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
    SSL_CTX_set_cert_store(ctx_, store.release());

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
    SSL_CTX_set_verify(ctx_, vmode, nullptr);

    // Advertise trusted CA names to the client in the TLS CertificateRequest
    // message (server only, and only meaningful when requiring a client cert).
    if (sendCAList && require && (role_ == role_t::SERVER || role_ == role_t::BOTH)) {
        X509_STORE* store = SSL_CTX_get_cert_store(ctx_);
        if (store) {
            STACK_OF(X509_NAME)* ca_names = sk_X509_NAME_new_null();
            if (ca_names) {
                STACK_OF(X509_OBJECT)* objs = X509_STORE_get0_objects(store);
                int n = sk_X509_OBJECT_num(objs);
                for (int i = 0; i < n; ++i) {
                    X509_OBJECT* obj = sk_X509_OBJECT_value(objs, i);
                    X509* cert = X509_OBJECT_get0_X509(obj);
                    if (!cert)
                        continue;
                    X509_NAME* name = X509_NAME_dup(X509_get_subject_name(cert));
                    if (name && sk_X509_NAME_push(ca_names, name) == 0)
                        X509_NAME_free(name);
                }
                if (sk_X509_NAME_num(ca_names) > 0)
                    SSL_CTX_set_client_CA_list(ctx_, ca_names);
                else
                    sk_X509_NAME_pop_free(ca_names, X509_NAME_free);
            }
        }
    }
}

int tls_context::pin_verify_cb(X509_STORE_CTX* store_ctx, void* arg) {
    // Perform standard chain verification first.
    if (!X509_verify_cert(store_ctx))
        return 0;

    auto* self = static_cast<tls_context*>(arg);

    // Walk the verified chain looking for the pinned certificate.
    STACK_OF(X509)* chain = X509_STORE_CTX_get0_chain(store_ctx);
    int n = sk_X509_num(chain);
    for (int i = 0; i < n; ++i) {
        if (X509_cmp(sk_X509_value(chain, i), self->pinned_cert_->cert_) == 0)
            return 1;
    }

    // No cert in the chain matched the pin.
    X509_STORE_CTX_set_error(store_ctx, X509_V_ERR_CERT_REJECTED);
    return 0;
}

void tls_context::allow_only_certificate(const tls_certificate& cert) {
    if (cert.is_valid()) {
        pinned_cert_ = cert;
        SSL_CTX_set_cert_verify_callback(ctx_, pin_verify_cb, this);
    }
    else {
        pinned_cert_.reset();
        SSL_CTX_set_cert_verify_callback(ctx_, nullptr, nullptr);
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

// ---------------------------------------------------------------------------
// ALPN

// static
int tls_context::alpn_select_cb(
    SSL* /*ssl*/, const unsigned char** out, unsigned char* outlen, const unsigned char* in,
    unsigned int inlen, void* arg
) noexcept {
    auto* self = static_cast<tls_context*>(arg);
    // SSL_select_next_proto uses NPN/ALPN server-preference matching.
    // OPENSSL_NPN_NEGOTIATED means a common protocol was found.
    if (SSL_select_next_proto(
            const_cast<unsigned char**>(out), outlen, self->alpn_wire_.data(),
            static_cast<unsigned>(self->alpn_wire_.size()), in, inlen
        ) == OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_OK;

    return SSL_TLSEXT_ERR_ALERT_FATAL;  // no overlap — abort handshake
}

result<> tls_context::set_alpn_protocols(const vector<string>& protocols) {
    if (protocols.empty()) {
        alpn_wire_.clear();
        SSL_CTX_set_alpn_select_cb(ctx_, nullptr, nullptr);
        return {};
    }

    // Build the wire-format: each protocol is <1-byte-len><name-bytes>.
    vector<uint8_t> wire;
    for (const auto& proto : protocols) {
        if (proto.size() > 255)
            return errc::invalid_argument;
        wire.push_back(static_cast<uint8_t>(proto.size()));
        wire.insert(wire.end(), proto.begin(), proto.end());
    }
    alpn_wire_ = std::move(wire);

    // Client: advertise the protocol list in ClientHello.
    if (SSL_CTX_set_alpn_protos(
            ctx_, alpn_wire_.data(), static_cast<unsigned>(alpn_wire_.size())
        ) != 0)
        return tls_last_error();

    // Server: register a select callback that picks by server preference.
    SSL_CTX_set_alpn_select_cb(ctx_, &tls_context::alpn_select_cb, this);

    return {};
}

// ---------------------------------------------------------------------------

result<> tls_context::set_identity(const string& cert_pem, const string& key_pem) {
    static constexpr size_t INT_MAX_SZ = static_cast<size_t>(std::numeric_limits<int>::max());
    if (cert_pem.size() > INT_MAX_SZ || key_pem.size() > INT_MAX_SZ)
        return make_error_code(std::errc::value_too_large);

    auto bio_deleter = [](BIO* b) { BIO_free(b); };
    auto cert_deleter = [](X509* c) { X509_free(c); };

    // --- Certificate chain ---

    unique_ptr<BIO, decltype(bio_deleter)> cert_bio{
        BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())), bio_deleter
    };
    if (!cert_bio)
        return tls_last_error();

    // Load the leaf certificate.
    unique_ptr<X509, decltype(cert_deleter)> leaf{
        PEM_read_bio_X509_AUX(cert_bio.get(), nullptr, nullptr, nullptr), cert_deleter
    };
    if (!leaf)
        return tls_last_error();

    if (SSL_CTX_use_certificate(ctx_, leaf.get()) != 1)
        return tls_last_error();

    // Load any intermediate certs that follow in the PEM string.
    SSL_CTX_clear_extra_chain_certs(ctx_);
    ERR_clear_error();

    for (;;) {
        unique_ptr<X509, decltype(cert_deleter)> ca{
            PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr), cert_deleter
        };
        if (!ca) {
            unsigned long err = ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
                ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
                ERR_clear_error();
                break;
            }
            return tls_last_error();
        }
        // SSL_CTX_add_extra_chain_cert takes ownership on success.
        if (SSL_CTX_add_extra_chain_cert(ctx_, ca.get()) != 1)
            return tls_last_error();
        ca.release();
    }

    // --- Private key ---

    unique_ptr<BIO, decltype(bio_deleter)> key_bio{
        BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size())), bio_deleter
    };
    if (!key_bio)
        return tls_last_error();

    auto key_deleter = [](EVP_PKEY* k) { EVP_PKEY_free(k); };
    unique_ptr<EVP_PKEY, decltype(key_deleter)> key{
        PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr), key_deleter
    };
    if (!key)
        return tls_last_error();

    if (SSL_CTX_use_PrivateKey(ctx_, key.get()) != 1)
        return tls_last_error();

    // Verify that the certificate and private key are consistent.
    return tls_check_res_none(SSL_CTX_check_private_key(ctx_));
}

unsigned int tls_context::psk_client_cb(
    SSL* ssl, const char* /*hint*/, char* identity, unsigned int max_identity_len,
    unsigned char* psk, unsigned int max_psk_len
) noexcept {
    auto* self = static_cast<tls_context*>(SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl)));
    if (!self || self->psk_key_.empty())
        return 0;

    if (max_identity_len == 0)
        return 0;
    size_t id_len = std::min(self->psk_identity_.size(), size_t{max_identity_len - 1});
    std::memcpy(identity, self->psk_identity_.c_str(), id_len);
    identity[id_len] = '\0';

    size_t key_len = std::min(self->psk_key_.size(), size_t{max_psk_len});
    std::memcpy(psk, self->psk_key_.data(), key_len);
    return static_cast<unsigned int>(key_len);
}

unsigned int tls_context::psk_server_cb(
    SSL* ssl, const char* identity, unsigned char* psk, unsigned int max_psk_len
) noexcept {
    auto* self = static_cast<tls_context*>(SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl)));
    if (!self || !self->psk_server_cb_)
        return 0;

    binary key = self->psk_server_cb_(string{identity ? identity : ""});
    if (key.empty())
        return 0;

    size_t key_len = std::min(key.size(), size_t{max_psk_len});
    std::memcpy(psk, key.data(), key_len);
    return static_cast<unsigned int>(key_len);
}

result<> tls_context::set_psk(const string& identity, const binary& psk) {
    psk_identity_ = identity;
    psk_key_ = psk;
    SSL_CTX_set_app_data(ctx_, this);
    SSL_CTX_set_psk_client_callback(ctx_, psk_client_cb);
    // The old-style PSK callbacks are TLS 1.2-only; cap the version and
    // restrict to PSK cipher suites so the handshake can succeed.
    SSL_CTX_set_max_proto_version(ctx_, TLS1_2_VERSION);
    SSL_CTX_set_cipher_list(ctx_, "PSK");
    return {};
}

result<> tls_context::set_psk_callback(psk_server_callback cb) {
    psk_server_cb_ = std::move(cb);
    SSL_CTX_set_app_data(ctx_, this);
    SSL_CTX_set_psk_server_callback(ctx_, psk_server_cb_ ? psk_server_cb : nullptr);
    if (psk_server_cb_) {
        // The old-style PSK callbacks are TLS 1.2-only; cap the version and
        // restrict to PSK cipher suites so the handshake can succeed.
        SSL_CTX_set_max_proto_version(ctx_, TLS1_2_VERSION);
        SSL_CTX_set_cipher_list(ctx_, "PSK");
    }
    return {};
}

result<> tls_context::set_identity(
    const tls_certificate_chain& chain, const string& key_pem
) {
    if (key_pem.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return make_error_code(std::errc::value_too_large);

    if (chain.empty())
        return make_error_code(std::errc::invalid_argument);

    // Install the leaf certificate.
    if (SSL_CTX_use_certificate(ctx_, chain[0].cert_) != 1)
        return tls_last_error();

    // Replace any existing intermediate chain certs.
    SSL_CTX_clear_extra_chain_certs(ctx_);
    for (size_t i = 1; i < chain.size(); i++) {
        X509_up_ref(chain[i].cert_);  // SSL_CTX_add_extra_chain_cert takes ownership
        if (SSL_CTX_add_extra_chain_cert(ctx_, chain[i].cert_) != 1) {
            X509_free(chain[i].cert_);
            return tls_last_error();
        }
    }

    // Load the private key from PEM.
    auto bio_deleter = [](BIO* b) { BIO_free(b); };
    unique_ptr<BIO, decltype(bio_deleter)> key_bio{
        BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size())), bio_deleter
    };
    if (!key_bio)
        return tls_last_error();

    auto key_deleter = [](EVP_PKEY* k) { EVP_PKEY_free(k); };
    unique_ptr<EVP_PKEY, decltype(key_deleter)> key{
        PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr), key_deleter
    };
    if (!key)
        return tls_last_error();

    if (SSL_CTX_use_PrivateKey(ctx_, key.get()) != 1)
        return tls_last_error();

    return tls_check_res_none(SSL_CTX_check_private_key(ctx_));
}

result<> tls_context::set_min_tls_version(tls_version ver) {
    int v = (ver == tls_version::TLS_1_3) ? TLS1_3_VERSION : TLS1_2_VERSION;
    if (SSL_CTX_set_min_proto_version(ctx_, v) != 1)
        return tls_last_error();
    return {};
}

result<> tls_context::set_max_tls_version(tls_version ver) {
    int v = (ver == tls_version::TLS_1_3) ? TLS1_3_VERSION : TLS1_2_VERSION;
    if (SSL_CTX_set_max_proto_version(ctx_, v) != 1)
        return tls_last_error();
    return {};
}

result<> tls_context::set_ciphersuites(const vector<string>& suites) {
    string colon_list;
    for (const auto& s : suites) {
        if (!colon_list.empty())
            colon_list += ':';
        colon_list += s;
    }

    // Apply to TLS 1.3 suites and TLS 1.2 suites; succeed if at least one call succeeds.
    bool ok13 = (SSL_CTX_set_ciphersuites(ctx_, colon_list.c_str()) == 1);
    bool ok12 = (SSL_CTX_set_cipher_list(ctx_, colon_list.c_str()) == 1);
    if (!ok13 && !ok12)
        return tls_last_error();
    return {};
}

result<unique_ptr<tls_socket>> tls_context::wrap_socket(
    stream_socket&& sock, const string& peer_name /*=string()*/
) const {
    error_code ec;
    auto tls_sock = make_unique<tls_socket>(*this, std::move(sock), ec);
    if (ec)
        return ec;

    if (!peer_name.empty()) {
        // SNI: tells the server which hostname we're connecting to so it can
        // select the right certificate when it hosts multiple domains.
        if (auto res = tls_sock->set_host_name(peer_name); !res)
            return res.error();

        // Automatic hostname verification: OpenSSL will check that the
        // server's certificate actually matches the requested hostname.
        if (SSL_set1_host(tls_sock->ssl(), peer_name.c_str()) != 1)
            return tls_last_error();
    }

    // Perform the TLS handshake.
    int ret = (role_ == role_t::SERVER) ? SSL_accept(tls_sock->ssl())
                                        : SSL_connect(tls_sock->ssl());
    if (ret != 1)
        return tls_last_error();

    return tls_sock;
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
