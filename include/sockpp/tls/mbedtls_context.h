/**
 * @file mbedtls_context.h
 *
 * TLS context implementation using mbedTLS.
 *
 * @author Jens Alfke
 * @author Couchbase, Inc.
 * @author Frank Pagliughi
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

#include "sockpp/result.h"
#include "sockpp/types.h"

struct mbedtls_pk_context;
struct mbedtls_ssl_config;
struct mbedtls_x509_crt;

namespace sockpp {

class stream_socket;
class mbedtls_socket;

using root_cert_locator_cb = std::function<bool(string cert, string& root)>;

/////////////////////////////////////////////////////////////////////////////

/**
 * TLS context for the mbedTLS backend.
 *
 * Acts as a factory for @ref mbedtls_socket objects and holds all
 * shared configuration (trust store, identity certificate, etc.).
 * A single context can be used by multiple sockets simultaneously.
 *
 * The public name @c tls_context is an alias for this class:
 * @code
 *   using tls_context = mbedtls_context;
 * @endcode
 */
class mbedtls_context
{
    struct cert;
    struct key;

    mutable int status_ = 0;
    std::function<bool(const string&)> auth_callback_;

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

protected:
    /** Records an initialization status code (non-zero means failure). */
    void set_status(int s) { status_ = s; }

public:
    /** The role for which a context or connection is used. */
    enum role_t {
        UNKNOWN = 0, ///< No role specified; use the context default.
        CLIENT  = 1, ///< Act as a TLS client.
        SERVER  = 2, ///< Act as a TLS server.
    };

    /** Options for set_verify(). */
    enum class verify_t { NONE, PEER };

    /**
     * A function called during the TLS handshake to examine the peer certificate.
     * @param certData  The DER-encoded certificate.
     * @return @em true to accept the cert, @em false to reject and abort.
     */
    using auth_callback = std::function<bool(const string& certData)>;

    /**
     * Creates an mbedTLS context for the specified role.
     * @param role Whether the context will be used for client or server connections.
     */
    explicit mbedtls_context(role_t role = CLIENT);
    ~mbedtls_context();

    // Non-copyable
    mbedtls_context(const mbedtls_context&) = delete;
    mbedtls_context& operator=(const mbedtls_context&) = delete;

    // Movable
    mbedtls_context(mbedtls_context&&) = default;
    mbedtls_context& operator=(mbedtls_context&&) = default;

    /** Returns the current status code (0 = success). */
    int status() const { return status_; }

    // ---- Auth callback ----

    /**
     * Registers a callback invoked during the TLS handshake that can accept or
     * reject the peer certificate.
     */
    void set_auth_callback(auth_callback cb) { auth_callback_ = std::move(cb); }

    /** Returns the authentication callback, if any. */
    const auth_callback& get_auth_callback() const { return auth_callback_; }

    // ---- Trust store ----

    /**
     * Sets the trusted root certificates from a PEM-encoded string.
     * @param certData PEM-encoded CA certificate data.
     */
    void set_root_certs(const string& certData);

    /**
     * Configures the context to use the system default certificate verification paths.
     * @return @em true on success.
     */
    bool set_default_verify_paths() { return true; }

    /**
     * Loads the system default CA trust locations.
     * Delegates to set_default_verify_paths().
     */
    result<> set_default_trust_locations() {
        return set_default_verify_paths() ? result<>{} : result<>{std::errc::no_such_file_or_directory};
    }

    /**
     * Loads a PEM CA bundle file into the trust store.
     * @param caFile Path to a PEM-format CA certificate file.
     */
    result<> set_trust_file(const string& caFile);

    /**
     * Loads all PEM CA certificate files from a directory into the trust store.
     * @param caPath Directory containing PEM CA certificate files.
     */
    result<> set_trust_path(const string& caPath);

    /**
     * Callback function that looks up the trusted root certificate that
     * signed a given cert.
     *
     * If found, the root certificate should be stored in `root`; else leave
     * `root` empty. The function should return false if and only if a fatal
     * error occurs.
     */
    void set_root_cert_locator(root_cert_locator_cb loc);

    /** Returns the root certificate locator callback, if one has been set. */
    root_cert_locator_cb root_cert_locator() const { return root_cert_locator_cb_; }

    // ---- Peer certificate policy ----

    /**
     * Configures whether a peer certificate is required and verified.
     * @param role The role (CLIENT or SERVER) to which this requirement applies.
     * @param required Whether a certificate must be presented.
     * @param verified Whether the presented certificate must pass verification.
     */
    void require_peer_cert(role_t role, bool required, bool verified);

    /**
     * Restricts accepted connections to peers presenting a specific certificate.
     * @param certData PEM-encoded certificate data.
     */
    void allow_only_certificate(const string& certData);

    /**
     * Restricts accepted connections to peers presenting a specific certificate.
     * @param certificate Pointer to a parsed mbedTLS certificate structure.
     */
    void allow_only_certificate(mbedtls_x509_crt* certificate);

    // ---- Verify mode ----

    /**
     * Sets the peer verification mode.
     * @param mode NONE disables verification; PEER requires a valid peer certificate.
     */
    void set_verify(verify_t mode);

    /** No-op stub for API compatibility with the OpenSSL backend. */
    void set_auto_retry(bool /*on*/ = true) {}

    // ---- Identity (local certificate + key) ----

    /**
     * Sets the identity certificate and private key using mbedTLS objects.
     * @param certificate The certificate chain to present to peers.
     * @param private_key The private key corresponding to the certificate.
     */
    void set_identity(mbedtls_x509_crt* certificate, mbedtls_pk_context* private_key);

    /**
     * Sets the identity certificate and private key from PEM-encoded strings.
     * @param certificate_data PEM-encoded certificate chain.
     * @param private_key_data PEM-encoded private key.
     */
    void set_identity(const string& certificate_data, const string& private_key_data);

    /**
     * Loads the local certificate chain from a PEM file.
     * @param certFile Path to the certificate chain file.
     */
    result<> set_cert_file(const string& certFile);

    /**
     * Loads the local private key from a PEM file.
     * @param keyFile Path to the private key file.
     */
    result<> set_key_file(const string& keyFile);

    // ---- Socket factory ----

    /**
     * Wraps an existing stream socket in a TLS layer.
     * @param sock The insecure stream socket to wrap.
     * @param role The role for this connection (CLIENT, SERVER, or UNKNOWN).
     * @param peer_name The expected peer host name for SNI and certificate verification.
     * @return A new TLS socket wrapping the given stream socket.
     */
    std::unique_ptr<mbedtls_socket> wrap_socket(
        stream_socket&& sock, role_t role = UNKNOWN, const string& peer_name = string{}
    );

    // ---- Accessors ----

    /** Returns the role for which this context was created. */
    role_t role();

    /** Returns a pointer to the system root certificate store, or nullptr if unavailable. */
    static mbedtls_x509_crt* get_system_root_certs();

    /** Returns the DER-encoded certificate received from the peer during the last handshake. */
    const string& get_peer_certificate() const { return received_cert_data_; }

    /**
     * TLS "fatal alert" codes are mapped into error codes returned from the socket's
     * last_error(). This mapping is done in mbedTLS style: a value of -0xF0xx, where xx is
     * the hex value of the alert. For example, MBEDTLS_SSL_ALERT_MSG_ACCESS_DENIED (49) is
     * mapped to error code -0xF031.
     */
    static constexpr int FATAL_ERROR_ALERT_BASE = -0xF000;
};

/**
 * For the mbedTLS backend, @c tls_context is an alias for @ref mbedtls_context
 * so that @ref tls_context_builder stores and moves the concrete type directly.
 */
using tls_context = mbedtls_context;

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_context_h
