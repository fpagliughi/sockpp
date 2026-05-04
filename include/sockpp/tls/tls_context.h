/**
 * @file tls/tls_context.h
 *
 * Abstract base class for TLS context implementations (mbedTLS path).
 *
 * @author Frank Pagliughi
 * @date 2024
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2024 Frank Pagliughi
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

#ifndef __sockpp_tls_tls_context_iface_h
#define __sockpp_tls_tls_context_iface_h

#include <functional>
#include <memory>
#include <string>

#include "sockpp/result.h"
#include "sockpp/types.h"

namespace sockpp {

class stream_socket;
class tls_socket_iface;

/////////////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for TLS context implementations.
 *
 * This defines the interface shared by all TLS backend contexts (e.g., mbedTLS).
 * Concrete implementations inherit from this class.
 *
 * The public @ref tls_context name is an alias for the concrete implementation
 * selected at build time (e.g., `using tls_context = mbedtls_context`).
 */
class tls_context_iface
{
    mutable int status_ = 0;
    std::function<bool(const string&)> auth_callback_;

protected:
    /**
     * Sets the internal status code. Called by derived classes to record
     * initialization failures.
     */
    void set_status(int s) { status_ = s; }

public:
    /** The role for which a context or connection is used. */
    enum role_t {
        UNKNOWN = 0, ///< No role specified; use the context default.
        CLIENT  = 1, ///< Act as a TLS client.
        SERVER  = 2, ///< Act as a TLS server.
    };

    /** Options for set_verify() */
    enum class verify_t { NONE, PEER };

    /**
     * A function that can be called during the TLS handshake to examine the
     * peer's certificate.
     * @param certData  The certificate's data, in DER encoding.
     * @return  True to accept the cert, false to reject it and abort the connection.
     */
    using auth_callback = std::function<bool(const string& certData)>;

    virtual ~tls_context_iface() = default;

    /** Returns the current status code (0 = success). */
    int status() const { return status_; }

    /**
     * Registers a callback to be invoked during the TLS handshake, that can
     * examine the peer cert (if any) and accept or reject it.
     */
    void set_auth_callback(auth_callback cb) { auth_callback_ = std::move(cb); }

    /** Returns the authentication callback, if any. */
    const auth_callback& get_auth_callback() const { return auth_callback_; }

    // ---- Pure virtual interface ----

    /**
     * Sets the trusted root certificates from a PEM-encoded string.
     * @param certData PEM-encoded CA certificate data.
     */
    virtual void set_root_certs(const string& certData) = 0;

    /**
     * Configures the context to use the system default certificate
     * verification paths.
     * @return @em true on success.
     */
    virtual bool set_default_verify_paths() = 0;

    /**
     * Configures whether a peer certificate is required and verified.
     * @param role The role to which this requirement applies.
     * @param required Whether a certificate must be presented.
     * @param verified Whether the presented certificate must pass verification.
     */
    virtual void require_peer_cert(role_t role, bool required, bool verified) = 0;

    /**
     * Restricts accepted connections to peers presenting a specific certificate.
     * @param certData PEM-encoded certificate data.
     */
    virtual void allow_only_certificate(const string& certData) = 0;

    /**
     * Sets the identity certificate and private key from PEM-encoded strings.
     * @param cert_data PEM-encoded certificate chain.
     * @param key_data PEM-encoded private key.
     */
    virtual void set_identity(const string& cert_data, const string& key_data) = 0;

    /**
     * Wraps an existing stream socket in a TLS layer.
     * @param sock The insecure stream socket to wrap.
     * @param role The role for this connection (CLIENT, SERVER, or UNKNOWN).
     * @param peer_name The expected peer host name for SNI and certificate verification.
     * @return A new TLS socket wrapping the given stream socket.
     */
    virtual std::unique_ptr<tls_socket_iface> wrap_socket(
        stream_socket&& sock, role_t role = UNKNOWN, const string& peer_name = string{}
    ) = 0;

    // ---- Methods for tls_context_builder compatibility ----
    // These have default (no-op / success) implementations for backends that
    // do not support file-based trust-store loading.

    /** Loads the system default CA trust locations. */
    virtual result<> set_default_trust_locations() { return {}; }
    /**
     * Loads a PEM CA bundle file into the trust store.
     * @param caFile Path to a PEM-format CA certificate file.
     */
    virtual result<> set_trust_file(const string& /*caFile*/) { return {}; }
    /**
     * Loads a directory of PEM CA certificate files into the trust store.
     * @param caPath Directory containing PEM CA certificate files.
     */
    virtual result<> set_trust_path(const string& /*caPath*/) { return {}; }
    /**
     * Sets the verification mode.
     * @param mode The verification mode (NONE or PEER).
     */
    virtual void set_verify(verify_t /*mode*/) {}
    /**
     * Sets whether the TLS layer should automatically retry after a
     * non-application record is processed.
     * @param on Turn auto-retry on or off.
     */
    virtual void set_auto_retry(bool /*on*/ = true) {}
    /**
     * Loads the local certificate chain from a PEM file.
     * @param certFile Path to the certificate chain file.
     */
    virtual result<> set_cert_file(const string& /*certFile*/) { return {}; }
    /**
     * Loads the local private key from a PEM file.
     * @param keyFile Path to the private key file.
     */
    virtual result<> set_key_file(const string& /*keyFile*/) { return {}; }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_tls_context_iface_h
