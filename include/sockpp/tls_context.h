/**
 * @file tls_context.h
 *
 * Context object for TLS (SSL) sockets.
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

#ifndef __sockpp_tls_context_h
#define __sockpp_tls_context_h

#include "sockpp/platform.h"
#include <functional>
#include <memory>
#include <string>

namespace sockpp {
    class connector;
    class stream_socket;
    class tls_socket;

    /**
     * Context / configuration for TLS (SSL) connections; also acts as a factory for
     * \ref tls_socket objects.
     *
     * A single context can be shared by any number of \ref tls_socket instances.
     * A context must remain in scope as long as any socket using it remains in scope.
     */
    class tls_context
    {
    public:
        enum role_t {
            CLIENT = 0,
            SERVER = 1
        };
        /**
         * A singleton context that can be used if you don't need any per-connection
         * configuration.
         */
        static tls_context& default_context();

        virtual ~tls_context() =default;

        /**
         * Tells whether the context is initialized and valid. Check this after constructing
         * an instance and do not use if not valid.
         * @return Zero if valid, a nonzero error code if initialization failed.
         *         The code may be a POSIX code, or one specific to the TLS library.
         */
        int status() const {
            return status_;
        }

        operator bool() const {
            return status_ == 0;
        }

        /**
         * Overrides the set of trusted root certificates used for validation.
         */
        virtual void set_root_certs(const std::string &certData) =0;

        /**
         * Configures whether the peer is required to present a valid certificate, for a connection
         * using the given role.
         * * For the CLIENT role the default is true; if you change to false, you take
         *   responsibility for validating the server certificate yourself!
         * * For the SERVER role the default is false; you can change it to true to require
         *   client certificate authentication.
         * @param role  The role you are configuring this setting for
         * @param require Pass true to require a valid peer certificate, false to not require.
         * @param sendCAList Pass true to automatically generate a list of trusted CAs for
         *                   the received client cert, if possible (only applies when role == SERVER)
         */
        virtual void require_peer_cert(role_t role, bool require, bool sendCAList) =0;

        /**
         * Requires that the peer have the exact certificate given.
         * This is known as "cert-pinning". It's more secure, but requires that the client
         * update its copy of the certificate whenever the server updates it.
         * @param certData The X.509 certificate in DER or PEM form; or an empty string for
         *                  no pinning (the default).
         */
        virtual void allow_only_certificate(const std::string &certData) =0;

        /**
         * A function that can be called during the TLS handshake to examine the peer's certificate.
         * @param certData  The certificate's data, in DER encoding.
         * @return  True to accept the cert, false to reject it and abort the connection.
         */
        using auth_callback = std::function<bool(const std::string &certData)>;

        /**
         * Registers a callback to be invoked during the TLS handshake, that can examine
         * the peer cert (if any) and accept or reject it.
         */
        void set_auth_callback(auth_callback cb) {
            auth_callback_ = std::move(cb);
        }

        /**
         * Returns the authentication callback, if any.
         */
        const auth_callback& get_auth_callback() const {
            return auth_callback_;
        }

        virtual void set_identity(const std::string &certificate_data,
                                  const std::string &private_key_data) =0;

        /**
         * Creates a new \ref tls_socket instance that wraps the given connector socket.
         * The \ref tls_socket takes ownership of the base socket and will close it when
         * it's closed.
         * When this method returns, the TLS handshake will already have completed;
         * be sure to check the stream's status, since the handshake may have failed.
         * @param socket The underlying connector socket that TLS will use for I/O.
         * @param role CLIENT or SERVER mode.
         * @param peer_name  The peer's canonical hostname, or other distinguished name,
         *                  to be used for certificate validation.
         * @return A new \ref tls_socket to use for secure I/O.
         */
        virtual std::unique_ptr<tls_socket> wrap_socket(std::unique_ptr<stream_socket> socket,
                                                        role_t role,
                                                        const std::string &peer_name) =0;

    protected:
        tls_context() =default;

        /**
         * Sets the error status of the context. Call this if initialization fails.
         */
        void set_status(int s) const {
            status_ = s;
        }

    private:
        tls_context(const tls_context&) =delete;

        mutable int status_ =0;
        std::function<bool(const std::string&)> auth_callback_;
    };

}

#endif
