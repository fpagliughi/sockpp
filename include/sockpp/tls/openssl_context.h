/**
 * @file openssl_context.h
 *
 * Context object for OpenSSL TLS (SSL) sockets.
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

#ifndef __sockpp_tls_openssl_context_h
#define __sockpp_tls_openssl_context_h

#include <openssl/ssl.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "sockpp/platform.h"
#include "sockpp/result.h"
#include "sockpp/tls/error.h"
#include "sockpp/types.h"
#include "sockpp/version.h"

namespace sockpp {

class tls_socket;
class stream_socket;

/////////////////////////////////////////////////////////////////////////////

/**
 * Context for OpenSSL TLS/SSL connections.
 *
 * Acts as a factory for \ref tls_socket objects.
 *
 * A single context can be shared by any number of \ref tls_socket instances.
 * A context must remain in scope as long as any socket using it remains in scope.
 */
class tls_context
{
public:
    enum class role_t {
        DEFAULT = 0x00,
        CLIENT = 0x01,
        SERVER = 0x02,
        BOTH = CLIENT | SERVER,
    };

    enum class verify_t { NONE, PEER };

private:
    /** The OpenSSL context struct. */
    SSL_CTX* ctx_ = nullptr;
    /** The role (CLIENT/SERVER/BOTH) for which the context was created.  */
    role_t role_;

    mutable int status_ = 0;
    std::function<bool(const string&)> auth_callback_;

    // Non-copyable
    tls_context(const tls_context&) = delete;
    tls_context& operator=(const tls_context&) = delete;

    friend class tls_socket;

public:
    /**
     * Create an OpenSSL context for the specified role.
     * @param role Whether the context will be used to make client or server
     *             connections, or both.
     */
    explicit tls_context(role_t role = role_t::CLIENT);
    /**
     * Move constructor.
     * @param ctx The other context to move into this one.
     */
    tls_context(tls_context&& ctx) : ctx_{ctx.ctx_}, role_{ctx.role_} { ctx.ctx_ = nullptr; }
    /**
     * Destructor closes the underlying OpenSSL context.
     */
    ~tls_context();
    /**
     * A singleton context that can be used if you don't need any per-connection
     * configuration.
     */
    static tls_context& default_context();
    /**
     * Creates a new client context.
     * @return A new client context.
     */
    static tls_context client() { return tls_context{role_t::CLIENT}; }
    /**
     * Creates a new server context.
     * @return A new server context.
     */
    static tls_context server() { return tls_context{role_t::SERVER}; }
    /**
     * Move assignment.
     * @param rhs The other context to move into this one.
     * @return A reference to this context.
     */
    tls_context& operator=(tls_context&& rhs);
    /**
     * Specify that application should use the default locations of the CA
     * certificates.
     * @return @em true on success, @em false on failure.
     */
    result<> set_default_trust_locations();
    /**
     * Sets a file of CA certificates as the trust store. The file should be
     * in PEM format.
     * @param caFile The path to a file of CA certificates to be used as the
     *               trust store.
     * @return The error code on failure.
     */
    result<> set_trust_file(const string& caFile) {
        return tls_check_res_none(
            ::SSL_CTX_load_verify_locations(ctx_, caFile.c_str(), nullptr)
        );
    }
    /**
     * Sets a directory containing CA certificate files as the trust store.
     * @param caPath The  directory containing CA certificate files as the
     *               trust store.
     * @return The error code on failure.
     */
    result<> set_trust_path(const string& caPath) {
        return tls_check_res_none(
            ::SSL_CTX_load_verify_locations(ctx_, nullptr, caPath.c_str())
        );
    }
    /**
     * Sets a trust file and/or a trust path at the same time.
     * @param caFile The path to a file of CA certificates to be used as the
     *               trust store.
     * @param caPath The  directory containing CA certificate files as the
     *               trust store.
     * @return The error code on failure
     */
    result<> set_trust_locations(
        const std::optional<string>& caFile,
        const std::optional<string>& caPath = std::nullopt
    );
    /**
     * Sets the verify flag in the context to the specified mode.
     * This wraps <A
     * href="https://www.openssl.org/docs/manmaster/man3/SSL_CTX_set_verify.html">
     * SSL_CTX_set_verify
     * @param mode The verification mode.
     */
    void set_verify(verify_t mode) noexcept;
    /**
     * Load the certificate chain from a file.
     * @param certFile The certificate chain file.
     * @return Error code on failure.
     */
    result<> set_cert_file(const string& certFile);
    /**
     * Set the private key from a file.
     * @param keyFile The private key file.
     * @return Error code on failure.
     */
    result<> set_key_file(const string& keyFile);
    /**
     * Overrides the set of trusted root certificates used for validation.
     */
    void set_root_certs(const string& certData);
    /**
     * Configures whether the peer is required to present a valid
     * certificate, for a connection using the given role.
     *
     * @li For the CLIENT role the default is true; if you change to false,
     *   you take responsibility for validating the server certificate
     *   yourself!
     * @li For the SERVER role the default is false; you can change it to
     *   true to require client certificate authentication.
     *
     * @param role  The role you are configuring this setting for
     * @param require Pass true to require a valid peer certificate, false
     *  			  to not require.
     * @param sendCAList Pass true to automatically generate a list of
     *  				 trusted CAs for the received client cert, if
     *  				 possible (only applies when role == SERVER)
     */
    void require_peer_cert(role_t role, bool require, bool sendCAList);

    /**
     * Requires that the peer have the exact certificate given.
     * This is known as "cert-pinning". It's more secure, but requires that the client
     * update its copy of the certificate whenever the server updates it.
     * @param certData The X.509 certificate in DER or PEM form; or an empty string for
     *  				no pinning (the default).
     */
    void allow_only_certificate(const string& certData);
    /**
     * A function that can be called during the TLS handshake to examine the peer's
     * certificate.
     * @param certData  The certificate's data, in DER encoding.
     * @return  True to accept the cert, false to reject it and abort the connection.
     */
    using auth_callback = std::function<bool(const string& certData)>;
    /**
     * Registers a callback to be invoked during the TLS handshake, that can examine
     * the peer cert (if any) and accept or reject it.
     */
    void set_auth_callback(auth_callback cb) { auth_callback_ = std::move(cb); }

    /**
     * Returns the authentication callback, if any.
     */
    const auth_callback& get_auth_callback() const { return auth_callback_; }

    void set_identity(const string& certificate_data, const string& private_key_data);
    /**
     * Creates a new \ref tls_socket instance that wraps the given connector
     * socket.
     *
     * The \ref tls_socket takes ownership of the base socket and will close
     * it when it's closed. When this method returns, the TLS handshake will
     * already have completed; be sure to check the stream's status, since
     * the handshake may have failed.
     * @param sock The underlying connector socket that TLS will use for
     *             I/O.
     * @param peer_name  The peer's canonical hostname, or other
     *  				distinguished name, to be used for certificate
     *  				validation.
     * @return A new \ref tls_socket to use for secure I/O.
     */
    tls_socket wrap_socket(stream_socket&& sock, const string& peer_name = string{});
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_context_h
