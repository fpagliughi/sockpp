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
#include <openssl/x509.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "sockpp/platform.h"
#include "sockpp/result.h"
#include "sockpp/tls/error.h"
#include "sockpp/tls/openssl_certificate.h"
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
    /** The role for which a context or connection is used. */
    enum class role_t {
        DEFAULT = 0x00, ///< No role specified; use the context default.
        CLIENT  = 0x01, ///< Act as a TLS client.
        SERVER  = 0x02, ///< Act as a TLS server.
        BOTH    = CLIENT | SERVER, ///< Support both client and server roles.
    };

    /** Options for set_verify() */
    enum class verify_t { NONE, PEER };

    /** Options for set_mode() */
    enum mode_t {
        ENABLE_PARTIAL_WRITE = SSL_MODE_ENABLE_PARTIAL_WRITE,
        ACCEPT_MOVING_WRITE_BUFFER = SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER,
        AUTO_RETRY = SSL_MODE_AUTO_RETRY
        // TODO: Add the rest
    };

private:
    /** The OpenSSL context struct. */
    SSL_CTX* ctx_ = nullptr;
    /** The role (CLIENT/SERVER/BOTH) for which the context was created.  */
    role_t role_;

    mutable int status_ = 0;
    std::function<bool(const string&)> auth_callback_;
    /** Pinned certificate for allow_only_certificate(), if set. */
    std::optional<tls_certificate> pinned_cert_;

    // Non-copyable
    tls_context(const tls_context&) = delete;
    tls_context& operator=(const tls_context&) = delete;

    friend class tls_socket;

    /** Verify callback used when a certificate is pinned. */
    static int pin_verify_cb(X509_STORE_CTX* store_ctx, void* arg);

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
     * Wraps SSL_CTX_set_verify.
     * @param mode The verification mode.
     */
    void set_verify(verify_t mode) noexcept;
    /**
     * Set to retry read or write after non-application data handled.
     * @param on Turn auto retry on or off.
     */
    void set_auto_retry(bool on = true) noexcept;
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
     * The @p certData should be a PEM-encoded string containing one or more
     * CA certificates. This replaces the entire existing trust store.
     * @param certData PEM-encoded CA certificate(s).
     * @return Error code on failure.
     */
    result<> set_root_certs(const string& certData);
    /**
     * Configures whether the peer is required to present a valid certificate.
     *
     * @li For a CLIENT context, the default is to verify the server certificate.
     *   Pass @p require=false to disable this (you then take responsibility for
     *   validating the server certificate yourself).
     * @li For a SERVER context, the default is to not request a client
     *   certificate. Pass @p require=true to enable mutual TLS (mTLS).
     *
     * @param require Pass true to require a valid peer certificate, false
     *                to not require one.
     * @param sendCAList Pass true to send the list of trusted CA names to the
     *                   client in the TLS handshake (server only; derived from
     *                   the configured trust store).
     */
    void require_peer_cert(bool require, bool sendCAList = false);

    /**
     * Pins the connection to a specific peer certificate (cert-pinning).
     *
     * When set, the TLS handshake will succeed only if the peer presents this
     * exact certificate somewhere in its chain, in addition to normal CA
     * validation.
     *
     * Pass an invalid (default-constructed) certificate to clear the pin
     * and revert to normal CA-only validation.
     *
     * @param cert The certificate to pin to, or a default-constructed
     *             @ref tls_certificate to clear the pin.
     */
    void allow_only_certificate(const tls_certificate& cert);
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

    /**
     * Loads the local certificate chain and private key from PEM strings.
     * This is the in-memory equivalent of calling @ref set_cert_file and
     * @ref set_key_file. The certificate string may contain a full chain
     * (leaf cert followed by any intermediate certs).
     * @param cert_pem PEM-encoded certificate chain.
     * @param key_pem PEM-encoded private key.
     * @return Error code on failure.
     */
    result<> set_identity(const string& cert_pem, const string& key_pem);
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
    result<tls_socket> wrap_socket(stream_socket&& sock, const string& peer_name = string{});
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_context_h
