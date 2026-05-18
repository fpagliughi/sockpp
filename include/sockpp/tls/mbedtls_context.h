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
#include <vector>

#include "sockpp/result.h"
#include "sockpp/tls/mbedtls_certificate.h"
#include "sockpp/types.h"

struct mbedtls_pk_context;
struct mbedtls_ssl_config;
struct mbedtls_ssl_context;
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
    unsigned mode_flags_ = 0;
    std::function<bool(const string&)> auth_callback_;

    unique_ptr<mbedtls_ssl_config> ssl_config_;
    root_cert_locator_cb root_cert_locator_cb_;
    unique_ptr<cert> root_certs_;
    unique_ptr<cert> pinned_cert_;
    bool pinned_cert_validation_result_{false};
    string received_cert_data_;

    unique_ptr<cert> identity_cert_;
    unique_ptr<key> identity_key_;

    /** ALPN protocol name strings (kept alive for mbedtls_ssl_conf_alpn_protocols). */
    vector<string> alpn_protocols_;
    /** Null-terminated pointer array passed to mbedtls_ssl_conf_alpn_protocols. */
    vector<const char*> alpn_proto_ptrs_;

    /**
     * Server-side PSK lookup callback (set by set_psk_callback()).
     * Stored here so it survives moves; re-registered via reregister_callbacks().
     */
    std::function<binary(const string&)> psk_server_cb_;

    /**
     * Cipher suite ID array passed to mbedtls_ssl_conf_ciphersuites().
     * Must remain alive for the lifetime of ssl_config_.
     */
    vector<int> ciphersuite_ids_;

    /** Thunk registered with mbedtls_ssl_conf_psk_cb(). */
    static int psk_server_cb_thunk(
        void* p_info, mbedtls_ssl_context* ssl, const uchar* identity, size_t identity_len
    );

    static cert* s_system_root_certs;

    friend class mbedtls_socket;

    int verify_callback(mbedtls_x509_crt* crt, int depth, uint32_t* flags);

    /** Re-registers ssl_config_ callbacks with the current `this` after a move. */
    void reregister_callbacks();

    int trusted_cert_callback(
        void* context, mbedtls_x509_crt const* child, mbedtls_x509_crt** candidates
    );

    static unique_ptr<cert> parse_cert(const string& cert_data, bool partialOk);

protected:
    /** Records an initialization status code (non-zero means failure). */
    void set_status(int s) { status_ = s; }

public:
    /** The role for which a context or connection is used. */
    enum role_t {
        UNKNOWN = 0,  ///< No role specified; use the context default.
        CLIENT = 1,   ///< Act as a TLS client.
        SERVER = 2,   ///< Act as a TLS server.
    };

    /** Options for set_verify(). */
    enum class verify_t { NONE, PEER };

    /** TLS protocol version selector for set_min/max_tls_version(). */
    enum class tls_version {
        TLS_1_2,  ///< TLS 1.2
        TLS_1_3,  ///< TLS 1.3
    };

    /**
     * Options for set_mode().
     *
     * Values are intentionally equal to the corresponding OpenSSL SSL_MODE_*
     * constants so that code using integer literals is portable across backends.
     *
     * - ENABLE_PARTIAL_WRITE: mbedTLS always allows partial writes; this flag
     *   is accepted but is effectively a no-op.
     * - ACCEPT_MOVING_WRITE_BUFFER: not applicable to mbedTLS; no-op.
     * - AUTO_RETRY: stored as a flag; future BIO-layer integration may consult
     *   it to suppress WANT_READ/WANT_WRITE errors from the application.
     */
    enum mode_t {
        ENABLE_PARTIAL_WRITE = 0x00000001,
        ACCEPT_MOVING_WRITE_BUFFER = 0x00000002,
        AUTO_RETRY = 0x00000004,
    };

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

    /**
     * Returns a shared default client context.
     * Useful when no per-connection configuration is required.
     * The context is initialised once and reused across calls.
     */
    static mbedtls_context& default_context();
    /**
     * Creates a new client context.
     * @return A new client context.
     */
    static mbedtls_context client() { return mbedtls_context{CLIENT}; }
    /**
     * Creates a new server context.
     * @return A new server context.
     */
    static mbedtls_context server() { return mbedtls_context{SERVER}; }

    // Non-copyable
    mbedtls_context(const mbedtls_context&) = delete;
    mbedtls_context& operator=(const mbedtls_context&) = delete;

    // Movable
    mbedtls_context(mbedtls_context&&) noexcept;
    mbedtls_context& operator=(mbedtls_context&&) noexcept;

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
     * No-op on the mbedTLS backend.
     *
     * System root certificates are loaded once at construction time via
     * get_system_root_certs().  There is nothing to do when this method is
     * called explicitly; it always returns true.
     *
     * @return @em true always.
     */
    bool set_default_verify_paths() { return true; }

    /**
     * No-op on the mbedTLS backend.
     *
     * System root certificates are loaded at construction time, so there is
     * nothing to do here.  Provided for API compatibility with the OpenSSL
     * backend.
     */
    result<> set_default_trust_locations() { return {}; }

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
     * @param required Whether a certificate must be presented.
     * @param sendCAList Pass true to send the list of trusted CA names to the
     *                   client in the TLS handshake (server only).
     */
    void require_peer_cert(bool required, bool sendCAList = false);

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
    /**
     * Sets one or more mode flags.
     *
     * ENABLE_PARTIAL_WRITE and AUTO_RETRY are stored; see @ref mode_t for
     * which flags have behavioural effect.  ACCEPT_MOVING_WRITE_BUFFER is
     * silently ignored (not applicable to mbedTLS).
     * @param mode Bitmask of @ref mode_t flags to set.
     */
    void set_mode(mode_t mode) noexcept { mode_flags_ |= static_cast<unsigned>(mode); }
    /**
     * Clears one or more mode flags.
     * @param mode Bitmask of @ref mode_t flags to clear.
     */
    void clear_mode(mode_t mode) noexcept { mode_flags_ &= ~static_cast<unsigned>(mode); }

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
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_identity(const string& certificate_data, const string& private_key_data);

    /**
     * Sets the local identity certificate chain and private key.
     * Equivalent to concatenating the PEM of each cert in @p chain and
     * calling the string-based overload.
     * @param chain Certificate chain (leaf first, then intermediates).
     * @param key_pem PEM-encoded private key.
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_identity(const tls_certificate_chain& chain, const string& key_pem);

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

    // ---- ALPN ----

    /**
     * Sets the ALPN protocol list for this context.
     *
     * On a client context, these protocols are advertised in the ClientHello.
     * On a server context, mbedTLS will select the first protocol from the
     * client's offer that also appears in @p protocols (server preference order).
     *
     * The protocol names and the pointer array are stored in the context and
     * must remain valid for as long as any socket uses this context.
     *
     * @param protocols Ordered list of protocol names (e.g. @c {"h2", "http/1.1"}).
     *                  Pass an empty vector to disable ALPN.
     * @return An error code on failure, or an empty result on success.
     */
    result<> set_alpn_protocols(const vector<string>& protocols);

    // ---- PSK ----

    /**
     * A function called on the server side to look up the PSK for a given
     * client identity.  Return an empty binary to reject the identity.
     */
    using psk_server_callback = std::function<binary(const string& identity)>;

    /**
     * Configures a TLS Pre-Shared Key (PSK) for client connections.
     *
     * The @p identity string is sent to the server; the server must know
     * the corresponding key.  The PSK data is copied into the mbedTLS
     * ssl_config and need not be kept alive after this call returns.
     *
     * @param identity  The PSK identity string.
     * @param psk       The raw PSK key bytes.
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_psk(const string& identity, const binary& psk);

    /**
     * Registers a server-side PSK lookup callback.
     *
     * When a client connects with a PSK cipher suite the callback is invoked
     * with the client-supplied identity string.  It should return the
     * corresponding key bytes, or an empty binary to reject the identity.
     *
     * Pass @c nullptr to clear a previously registered callback.
     *
     * @param cb  The callback, or @c nullptr to clear.
     * @return An empty result always (kept as @c result<> for API symmetry).
     */
    result<> set_psk_callback(psk_server_callback cb);

    // ---- Protocol version ----

    /**
     * Sets the minimum acceptable TLS protocol version.
     * @param ver The minimum TLS version to accept.
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_min_tls_version(tls_version ver);

    /**
     * Sets the maximum acceptable TLS protocol version.
     * @param ver The maximum TLS version to accept.
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_max_tls_version(tls_version ver);

    // ---- Cipher suites ----

    /**
     * Restricts the set of cipher suites the context will negotiate.
     *
     * Accepts mbedTLS-style cipher suite names (e.g.
     * @c "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256").  Any unrecognised names
     * are silently skipped; an error is returned only if the resulting list
     * is empty.
     *
     * The list is stored in the context and must remain valid for its
     * lifetime.
     *
     * @param suites Ordered list of cipher suite names.
     * @return An empty result on success, or an error code on failure.
     */
    result<> set_ciphersuites(const vector<string>& suites);

    // ---- Socket factory ----

    /**
     * Wraps an existing stream socket in a TLS layer and runs the handshake.
     * @param sock The insecure stream socket to wrap.
     * @param peer_name The expected peer host name for SNI and certificate verification.
     * @return A heap-allocated TLS socket on success, or an error code on failure.
     */
    result<unique_ptr<mbedtls_socket>> wrap_socket(
        stream_socket&& sock, const string& peer_name = string{}
    );

    // ---- Accessors ----

    /** Returns the role for which this context was created. */
    role_t role();

    /** Returns a pointer to the system root certificate store, or nullptr if unavailable. */
    static mbedtls_x509_crt* get_system_root_certs();

    /** Returns the DER-encoded certificate received from the peer during the last handshake.
     */
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
