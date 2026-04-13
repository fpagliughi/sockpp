/**
 * @file mbedtls_socket.h
 *
 * TLS (SSL) socket implementation using mbedTLS.
 *
 * @author Jens Alfke
 * @author Couchbase, Inc.
 * @author www.couchbase.com
 *
 * @date August 2019
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2024 Frank Pagliughi
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

#ifndef __sockpp_tls_mbedtls_context_h
#define __sockpp_tls_mbedtls_context_h

#include <functional>
#include <memory>
#include <string>

#include "sockpp/tls/tls_context.h"
#include "sockpp/tls/tls_socket.h"

struct mbedtls_pk_context;
struct mbedtls_ssl_config;
struct mbedtls_x509_crt;

namespace sockpp {

using root_cert_locator_cb = std::function<bool(string cert, string& root)>;

/////////////////////////////////////////////////////////////////////////////

/**
 * A concrete implementation of \ref tls_context, using the mbedTLS library.
 * You probably don't want to use this class directly, unless you want to instantiate a
 * custom context so you can have different contexts for different sockets.
 */
class mbedtls_context : public tls_context
{
    struct cert;
    struct key;

    std::unique_ptr<mbedtls_ssl_config> ssl_config_;
    root_cert_locator_cb root_cert_locator_cb_;
    std::unique_ptr<cert> root_certs_;
    std::unique_ptr<cert> pinned_cert_;
    bool pinned_cert_validation_result_{false};
    string received_cert_data_;

    std::unique_ptr<cert> identity_cert_;
    std::unique_ptr<key> identity_key_;

    static cert* s_system_root_certs;

    friend class mbedtls_socket;

    int verify_callback(mbedtls_x509_crt* crt, int depth, uint32_t* flags);

    int trusted_cert_callback(
        void* context, mbedtls_x509_crt const* child, mbedtls_x509_crt** candidates
    );

    static std::unique_ptr<cert> parse_cert(const string& cert_data, bool partialOk);

public:
    explicit mbedtls_context(role_t role = CLIENT);
    ~mbedtls_context() override;

    void set_root_certs(const string& certData) override;
    bool set_default_verify_paths() override { return true; }

    /**
     * Callback function that looks up the trusted root certificate that
     * signed a given cert.
     *
     * If found, the root certificate should be stored in `root`; else leave
     * `root` empty. The function should return false if and only if a fatal
     * error occurs.
     */
    void set_root_cert_locator(root_cert_locator_cb loc);
    root_cert_locator_cb root_cert_locator() const { return root_cert_locator_cb_; }

    void require_peer_cert(role_t, bool, bool) override;
    void allow_only_certificate(const string& certData) override;

    void allow_only_certificate(mbedtls_x509_crt* certificate);

    /**
     * Sets the identity certificate and private key using mbedTLS objects.
     */
    void set_identity(mbedtls_x509_crt* certificate, mbedtls_pk_context* private_key);

    void set_identity(
        const string& certificate_data, const string& private_key_data
    ) override;

    std::unique_ptr<tls_socket> wrap_socket(
        stream_socket&& sock, role_t role = UNKNOWN, const string& peer_name = string{}
    ) override;

    role_t role();

    static mbedtls_x509_crt* get_system_root_certs();

    const string& get_peer_certificate() const { return received_cert_data_; }

    /**
     * TLS "fatal alert" codes are mapped into error codes returned from the socket's
     * last_error(). This mapping is done in mbedTLS style: a value of -0xF0xx, where xx is
     * the hex value of the alert. For example, MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED (49) is
     * mapped to error code -0xF031.
     */
    static constexpr int FATAL_ERROR_ALERT_BASE = -0xF000;
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_context_h
