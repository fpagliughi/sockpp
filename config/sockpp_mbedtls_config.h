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

/* =========================================================================
 * X.509 certificates and public keys
 * ========================================================================= */

/** Enable X.509 certificate parsing. */
#define MBEDTLS_X509_CRT_PARSE_C

/** Enable X.509 certificate use (verification). */
#define MBEDTLS_X509_USE_C

/** Enable the trusted-certificate callback API (mbedtls_ssl_conf_ca_cb). */
#define MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK

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
/* #define MBEDTLS_SSL_ALPN */

/** Max fragment length negotiation. */
/* #define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

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
 * NOT needed — sockpp provides its own socket I/O layer
 * ========================================================================= */

/* Do NOT define MBEDTLS_NET_C.  sockpp supplies bio_send/bio_recv callbacks
 * so mbedTLS never touches the socket directly. */

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
