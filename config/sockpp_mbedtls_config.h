/**
 * @file sockpp_mbedtls_config.h
 *
 * mbedTLS configuration for sockpp — targets mbedTLS 4.1.0.
 *
 * This file configures the structural modules (TLS protocol, X.509, PEM,
 * utilities, threading).  Cryptographic algorithm selection for mbedTLS 4.x
 * lives in config/psa/crypto_config.h via PSA_WANT_* macros.
 *
 * Usage
 * -----
 * Pass the absolute path to this file via MBEDTLS_CONFIG_FILE, and add the
 * sockpp config/ directory to the include path.  MBEDTLS_CONFIG_FILE is a
 * FILEPATH CMake variable; a bare filename would be resolved relative to the
 * build directory where it does not exist.  The PSA algorithm config is found
 * automatically because mbedTLS includes "psa/crypto_config.h" by relative
 * path and config/psa/crypto_config.h shadows the default.
 *
 *   cmake -B build \
 *         -DMBEDTLS_CONFIG_FILE=/path/to/sockpp/config/sockpp_mbedtls_config.h \
 *         -DCMAKE_C_FLAGS="-I/path/to/sockpp/config" \
 *         /path/to/mbedtls
 *
 * And point sockpp at the same mbedTLS install:
 *
 *   cmake -B build \
 *         -DSOCKPP_WITH_MBEDTLS=ON \
 *         -DCMAKE_PREFIX_PATH=/path/to/mbedtls/install \
 *         -DCMAKE_CXX_FLAGS="-DMBEDTLS_CONFIG_FILE='<sockpp_mbedtls_config.h>' \
 *                             -I/path/to/sockpp/config" \
 *         /path/to/sockpp
 *
 * mbedTLS 4.x vs 3.x
 * -------------------
 * If targeting mbedTLS 3.x (3.6 LTS), use sockpp_mbedtls3_config.h instead.
 * Key differences relevant to this file:
 *
 *   - MBEDTLS_USE_PSA_CRYPTO and MBEDTLS_PSA_CRYPTO_CONFIG are always ON in
 *     4.x and are no longer configuration options.  Do not define them here.
 *   - All cryptographic algorithm selection (AES, SHA, ECDH, RSA, etc.) moved
 *     to psa/crypto_config.h via PSA_WANT_* macros.  The legacy MBEDTLS_AES_C,
 *     MBEDTLS_SHA256_C, MBEDTLS_BIGNUM_C, MBEDTLS_RSA_C, etc. are no longer
 *     public configuration options.
 *   - MBEDTLS_ENTROPY_C and MBEDTLS_CTR_DRBG_C are removed; all random number
 *     generation goes through PSA (psa_generate_random()).
 *   - psa_crypto_init() MUST be called at application startup before any TLS
 *     or certificate operation.  See the Application notes section below.
 *   - mbedtls_ssl_set_hostname() is now mandatory for TLS client connections;
 *     skipping it causes handshake failure (not silent cert bypass as in 3.x).
 */

#ifndef SOCKPP_MBEDTLS_CONFIG_H
    #define SOCKPP_MBEDTLS_CONFIG_H

    /* =========================================================================
     * TLS protocol
     * ========================================================================= */

    /** Enable the TLS protocol. */
    #define MBEDTLS_SSL_TLS_C

    /** Enable the TLS client role. */
    #define MBEDTLS_SSL_CLI_C

    /** Enable the TLS server role. */
    #define MBEDTLS_SSL_SRV_C

    /** Enable TLS 1.2. */
    #define MBEDTLS_SSL_PROTO_TLS1_2

    /**
     * Enable TLS 1.3.
     * Requires mbedTLS 4.x (PSA backend, always on).
     */
    #define MBEDTLS_SSL_PROTO_TLS1_3

    /**
     * TLS 1.3 ephemeral key exchange mode.
     *
     * MANDATORY when MBEDTLS_SSL_PROTO_TLS1_3 is enabled and the connection uses
     * certificates (i.e. virtually all public internet servers).  Without this,
     * TLS 1.3 is compiled in but has no valid key exchange mode, causing an
     * internal error during the handshake.
     *
     * Requires: PSA_WANT_ALG_ECDH (or PSA_WANT_ALG_FFDH)
     *           MBEDTLS_X509_CRT_PARSE_C
     *           PSA_WANT_ALG_ECDSA or PSA_WANT_ALG_RSA_PSS
     */
    #define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED

    /**
     * TLS 1.3 PSK-only key exchange mode.
     * Used for session resumption without forward secrecy.  The PSK is derived
     * from a prior session's master secret (session tickets) or from an external
     * pre-shared key.  No certificate is required.
     *
     * Requires: PSA_WANT_ALG_TLS12_PSK_TO_MS (already enabled)
     */
    #define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED

    /**
     * TLS 1.3 PSK + ephemeral key exchange mode.
     * Combines a pre-shared key with an ephemeral ECDHE exchange for forward
     * secrecy.  Preferred over PSK-only when resuming sessions.
     *
     * Requires: PSA_WANT_ALG_ECDH, PSA_WANT_ALG_TLS12_PSK_TO_MS
     */
    #define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED

    /**
     * TLS 1.3 middlebox compatibility mode (RFC 8446 appendix D.4).
     * Recommended: makes TLS 1.3 traffic look like TLS 1.2 to legacy middle
     * boxes.  Adds a few bytes on the wire but does not affect interoperability
     * with correct TLS 1.3 peers.
     */
    #define MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE

    /**
     * Server Name Indication.
     * Required for hostname verification.  sockpp always calls
     * mbedtls_ssl_set_hostname(); in mbedTLS 4.x omitting that call causes
     * handshake failure rather than silently skipping CN/SAN verification.
     */
    #define MBEDTLS_SSL_SERVER_NAME_INDICATION

    /**
     * Keep the peer certificate in memory after the handshake.
     * Required when MBEDTLS_SSL_PROTO_TLS1_3 is enabled, and needed by
     * mbedtls_socket::peer_certificate() to retrieve the peer's certificate.
     */
    #define MBEDTLS_SSL_KEEP_PEER_CERTIFICATE

    /**
     * Encrypt-then-MAC (RFC 7366).
     * Recommended: strengthens TLS 1.2 CBC cipher suites against padding-oracle
     * and timing attacks.  No effect on AEAD suites or TLS 1.3.
     */
    #define MBEDTLS_SSL_ENCRYPT_THEN_MAC

    /**
     * Extended Master Secret (RFC 7627).
     * Recommended: defends against the Triple Handshake attack and related
     * protocol weaknesses.  Enable even when renegotiation is disabled.
     */
    #define MBEDTLS_SSL_EXTENDED_MASTER_SECRET

    /* =========================================================================
     * TLS 1.2 key exchange methods
     *
     * TLS 1.3 key exchange is handled automatically via the PSA layer.  TLS 1.2
     * still requires explicit selection of handshake modes here.
     * ========================================================================= */

    /** ECDHE key exchange authenticated with an RSA certificate. */
    #define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED

    /** ECDHE key exchange authenticated with an ECDSA certificate. */
    #define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

    /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED was removed in mbedTLS 4.0 (no forward
     * secrecy).  RSA certificate *authentication* is still supported via
     * MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED above. */

    /**
     * TLS 1.2 PSK key exchange.
     * Symmetric pre-shared key; no certificate required.  Useful for
     * constrained devices and DTLS deployments.
     */
    #define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

    /**
     * TLS 1.2 ECDHE-PSK key exchange.
     * Adds an ephemeral ECDHE exchange to PSK for forward secrecy.
     * Preferred over plain PSK when the device can afford the extra compute.
     *
     * Requires: PSA_WANT_ALG_ECDH (already enabled)
     */
    #define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED

    /* =========================================================================
     * X.509 certificates and public keys
     * ========================================================================= */

    /** Enable X.509 certificate parsing. */
    #define MBEDTLS_X509_CRT_PARSE_C

    /** Enable X.509 certificate use (verification). */
    #define MBEDTLS_X509_USE_C

    /** Enable public-key abstraction layer (used by X.509 and TLS). */
    #define MBEDTLS_PK_C

    /** Enable PEM parsing (required for PEM-encoded certificates and keys). */
    #define MBEDTLS_PEM_PARSE_C

    /** Enable PEM writing (required for tls_certificate::to_pem()). */
    #define MBEDTLS_PEM_WRITE_C

    /** Enable ASN.1 DER parser (required for X.509). */
    #define MBEDTLS_ASN1_PARSE_C

    /** Enable ASN.1 DER writer. */
    #define MBEDTLS_ASN1_WRITE_C

    /* =========================================================================
     * Optional TLS extensions
     * ========================================================================= */

    /** Session tickets (client-side resumption). */
    /* #define MBEDTLS_SSL_SESSION_TICKETS */

    /** ALPN — Application-Layer Protocol Negotiation (e.g. for HTTP/2). */
    #define MBEDTLS_SSL_ALPN

    /** Max fragment length negotiation. */
    /* #define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

    /* =========================================================================
     * DTLS — Datagram TLS (TLS over UDP)
     *
     * DTLS 1.2 is defined in RFC 6347; DTLS 1.3 in RFC 9147.
     * mbedTLS supports DTLS 1.2 (and experimentally DTLS 1.3 in 4.x).
     * DTLS is commonly paired with PSK for constrained IoT devices.
     * ========================================================================= */

    /**
     * Timing module.
     * Provides mbedtls_timing_get_timer() and the delay callback used by DTLS
     * for retransmission timeouts (mbedtls_ssl_set_timer_cb()).  Required when
     * MBEDTLS_SSL_PROTO_DTLS is enabled.
     */
    #define MBEDTLS_TIMING_C

    /**
     * Enable the DTLS protocol layer.
     * Requires MBEDTLS_SSL_PROTO_TLS1_2 (already enabled).
     * Requires: MBEDTLS_TIMING_C
     */
    #define MBEDTLS_SSL_PROTO_DTLS

    /**
     * DTLS cookie module.
     * Provides mbedtls_ssl_cookie_write() / mbedtls_ssl_cookie_check() used
     * by the hello-verify mechanism below.  Required when
     * MBEDTLS_SSL_DTLS_HELLO_VERIFY is enabled.
     */
    #define MBEDTLS_SSL_COOKIE_C

    /**
     * DTLS server hello-verify (RFC 6347 §4.2.1).
     * Sends a HelloVerifyRequest with a cookie before the full handshake,
     * preventing amplification attacks from spoofed client addresses.
     * Required for DTLS servers exposed to untrusted networks.
     * Requires: MBEDTLS_SSL_COOKIE_C
     */
    #define MBEDTLS_SSL_DTLS_HELLO_VERIFY

    /**
     * DTLS anti-replay protection (RFC 6347 §4.1.2.6).
     * Maintains a sliding window of received record sequence numbers and
     * discards duplicates, preventing replay attacks on DTLS connections.
     */
    #define MBEDTLS_SSL_DTLS_ANTI_REPLAY

    /**
     * DTLS bad-MAC record limit.
     * Closes the connection after a configurable number of records with a
     * bad MAC, hardening against fault-injection and padding-oracle attacks.
     * Limit is set at runtime via mbedtls_ssl_conf_dtls_badmac_limit().
     */
    #define MBEDTLS_SSL_DTLS_BADMAC_LIMIT

    /**
     * DTLS Connection ID extension (RFC 9146).
     * Allows the connection to survive client address changes (e.g. NAT
     * rebinding or mobile roaming) without a full re-handshake.
     * Optional — disable to save a few bytes on very constrained devices.
     */
    /* #define MBEDTLS_SSL_DTLS_CONNECTION_ID */

    /**
     * DTLS-SRTP (RFC 5764) — key material export for Secure RTP.
     * Required for WebRTC media encryption.  Not needed for generic DTLS.
     */
    /* #define MBEDTLS_SSL_DTLS_SRTP */

    /* =========================================================================
     * Utilities
     * ========================================================================= */

    /**
     * Human-readable error strings (enables mbedtls_strerror()).
     * Disable in production builds where code size matters.
     */
    #define MBEDTLS_ERROR_C

    /** Version information (enables mbedtls_version_get_number() etc.). */
    #define MBEDTLS_VERSION_C

    /**
     * Debug output support.
     * Disable for production builds.  Enable at runtime with:
     *   mbedtls_debug_set_threshold(2);  // 1=warning 2=info 3=debug 4=verbose
     *   mbedtls_ssl_conf_dbg(&conf, my_debug_cb, NULL);
     */
    /* #define MBEDTLS_DEBUG_C */

    /* =========================================================================
     * Threading
     *
     * In mbedTLS 4.x, threading configuration moved to psa/crypto_config.h.
     * See config/psa/crypto_config.h for MBEDTLS_THREADING_C and
     * MBEDTLS_THREADING_PTHREAD / MBEDTLS_THREADING_ALT.
     * ========================================================================= */

    /* =========================================================================
     * Network socket layer
     * ========================================================================= */

    /**
     * mbedTLS built-in TCP/UDP socket layer.
     *
     * sockpp does not use this — it supplies its own bio_send/bio_recv callbacks
     * so mbedTLS never touches the socket directly.  Enabled here so that the
     * installed library is useful to other consumers that call
     * mbedtls_net_connect() / mbedtls_net_accept() directly.
     */
    #define MBEDTLS_NET_C

#endif /* SOCKPP_MBEDTLS_CONFIG_H */

/*
 * =========================================================================
 * Application notes
 * =========================================================================
 *
 * 1. Startup sequence (mandatory in mbedTLS 4.x)
 * -----------------------------------------------
 * Call psa_crypto_init() once at application startup, before any TLS,
 * certificate parsing, or key operation:
 *
 *   #include <psa/crypto.h>
 *
 *   psa_status_t status = psa_crypto_init();
 *   if (status != PSA_SUCCESS) {
 *       // handle fatal initialisation failure
 *   }
 *
 * On a multi-threaded server, call psa_crypto_init() from the main thread
 * before spawning worker threads.  After that the PSA layer is thread-safe.
 *
 * 2. Cryptographic algorithm configuration
 * -----------------------------------------
 * In mbedTLS 4.x, cipher suites and hash/key-exchange algorithms are
 * selected via PSA_WANT_* macros in a separate crypto configuration file
 * (psa/crypto_config.h or an equivalent pointed to by
 * MBEDTLS_PSA_CRYPTO_CONFIG_FILE).  See sockpp_psa_crypto_config.h for a
 * starting-point config that enables the algorithms needed by sockpp.
 *
 * 3. Linking
 * ----------
 * Link against all three mbedTLS libraries:
 *
 *   target_link_libraries(myapp PRIVATE
 *       MbedTLS::mbedtls
 *       MbedTLS::mbedcrypto
 *       MbedTLS::mbedx509
 *   )
 *
 * 4. Windows THREADING_ALT implementation sketch
 * -----------------------------------------------
 *   #include <windows.h>
 *   void mbedtls_mutex_init  (mbedtls_threading_mutex_t *m) { InitializeCriticalSection(m); }
 *   void mbedtls_mutex_free  (mbedtls_threading_mutex_t *m) { DeleteCriticalSection(m); }
 *   int  mbedtls_mutex_lock  (mbedtls_threading_mutex_t *m) {
 *       EnterCriticalSection(m); return 0; }
 *   int  mbedtls_mutex_unlock(mbedtls_threading_mutex_t *m) {
 *       LeaveCriticalSection(m); return 0; }
 *   // call once before psa_crypto_init():
 *   mbedtls_threading_set_alt(
 *       mbedtls_mutex_init, mbedtls_mutex_free,
 *       mbedtls_mutex_lock, mbedtls_mutex_unlock);
 */
