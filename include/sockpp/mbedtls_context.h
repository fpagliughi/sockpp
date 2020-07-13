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
// Copyright (c) 2014-2019 Frank Pagliughi
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

#ifndef __sockpp_mbedtls_socket_h
#define __sockpp_mbedtls_socket_h

#include "sockpp/tls_context.h"
#include "sockpp/tls_socket.h"
#include <memory>
#include <string>
#include <functional>

struct mbedtls_pk_context;
struct mbedtls_ssl_config;
struct mbedtls_x509_crt;

namespace sockpp {

    /**
     * A concrete implementation of \ref tls_context, using the mbedTLS library.
     * You probably don't want to use this class directly, unless you want to instantiate a
     * custom context so you can have different contexts for different sockets.
     */
    class mbedtls_context : public tls_context {
    public:
        mbedtls_context(role_t = CLIENT);
        ~mbedtls_context() override;

        void set_root_certs(const std::string &certData) override;

        /**
         * Callback function that looks up the trusted root certificate that signed a given cert.
         * If found, the root certificate should be stored in `root`; else leave `root` empty.
         * The function should return false if and only if a fatal error occurs.
         */
        using RootCertLocator = std::function<bool(std::string cert, std::string &root)>;

        void set_root_cert_locator(RootCertLocator loc);
        RootCertLocator root_cert_locator() const           {return root_cert_locator_;}

        void require_peer_cert(role_t, bool, bool) override;
        void allow_only_certificate(const std::string &certData) override;

        void allow_only_certificate(mbedtls_x509_crt *certificate);

        /**
         * Sets the identity certificate and private key using mbedTLS objects.
         */
        void set_identity(mbedtls_x509_crt *certificate,
                          mbedtls_pk_context *private_key);

        void set_identity(const std::string &certificate_data,
                          const std::string &private_key_data) override;

        std::unique_ptr<tls_socket> wrap_socket(std::unique_ptr<stream_socket> socket,
                                                role_t,
                                                const std::string &peer_name) override;

        role_t role();

        static mbedtls_x509_crt* get_system_root_certs();

        using Logger = std::function<void(int level, const char *filename, int line, const char *message)>;
        void set_logger(int threshold, Logger);
        
        const std::string& get_peer_certificate() const { return received_cert_data_; }

        /**
         * TLS "fatal alert" codes are mapped into error codes returned from the socket's last_error().
         * This mapping is done in mbedTLS style: a value of -0xF0xx, where xx is the hex value of the alert.
         * For example, MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED (49) is mapped to error code -0xF031.
         */
        static constexpr int FATAL_ERROR_ALERT_BASE = -0xF000;

    private:
        struct cert;
        struct key;

        int verify_callback(mbedtls_x509_crt *crt, int depth, uint32_t *flags);
        int trusted_cert_callback(void *context, mbedtls_x509_crt const *child,
                                  mbedtls_x509_crt **candidates);
        static std::unique_ptr<cert> parse_cert(const std::string &cert_data, bool partialOk);

        std::unique_ptr<mbedtls_ssl_config> ssl_config_;
        RootCertLocator root_cert_locator_;
        std::unique_ptr<cert> root_certs_;
        std::unique_ptr<cert> pinned_cert_;
        std::string received_cert_data_;

        std::unique_ptr<cert> identity_cert_;
        std::unique_ptr<key> identity_key_;
        Logger logger_;

        static cert *s_system_root_certs;

        friend class mbedtls_socket;
    };

}

#endif
