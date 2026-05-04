/**
 * @file psa/crypto_config.h
 *
 * PSA cryptographic algorithm configuration for sockpp — mbedTLS 4.1.0.
 *
 * In mbedTLS 4.x all cipher/hash/key-type selection moved from
 * mbedtls_config.h to this PSA configuration file.  The macros here have
 * the form:
 *
 *   PSA_WANT_ALG_<algorithm>    — enable a specific algorithm
 *   PSA_WANT_KEY_TYPE_<type>    — enable a key type (e.g. RSA, ECC)
 *   PSA_WANT_ECC_<curve>        — enable a specific elliptic curve
 *
 * The macros selected below cover all cipher suites that TLS 1.2 and
 * TLS 1.3 can negotiate with the common server certificate types (RSA and
 * ECDSA) encountered in practice.
 *
 * Usage
 * -----
 * This file is found automatically by mbedTLS when the sockpp config/
 * directory is on the compiler include path:
 *
 *   cmake -B build \
 *         -DMBEDTLS_CONFIG_FILE="sockpp_mbedtls_config.h" \
 *         -DCMAKE_C_FLAGS="-I/path/to/sockpp/config" \
 *         /path/to/mbedtls
 *
 * mbedTLS headers include "psa/crypto_config.h" by relative path; adding
 * the config/ directory to the include path shadows the default file with
 * this one.  No separate CMake variable is needed.
 *
 * See also: sockpp_mbedtls_config.h (structural module configuration).
 */

#ifndef SOCKPP_PSA_CRYPTO_CONFIG_H
#define SOCKPP_PSA_CRYPTO_CONFIG_H

/* =========================================================================
 * Hash algorithms
 *
 * SHA-256 and SHA-384 are required by TLS 1.2 signature algorithms and
 * TLS 1.3 cipher suites.  SHA-1 is needed only for legacy TLS 1.2 suites
 * and for X.509 certificate verification of older CAs; disable it if your
 * deployment does not require legacy compatibility.
 * ========================================================================= */

#define PSA_WANT_ALG_SHA_256
#define PSA_WANT_ALG_SHA_384
#define PSA_WANT_ALG_SHA_512

/** Required for HMAC-based PRF in TLS 1.2. */
#define PSA_WANT_ALG_HMAC

/** Required for HKDF key-derivation in TLS 1.3. */
#define PSA_WANT_ALG_HKDF
#define PSA_WANT_ALG_HKDF_EXPAND
#define PSA_WANT_ALG_HKDF_EXTRACT

/** Legacy — needed only when verifying SHA-1-signed certificates. */
/* #define PSA_WANT_ALG_SHA_1 */

/* =========================================================================
 * Symmetric ciphers
 *
 * AES-GCM covers the mandatory TLS 1.3 cipher suite (AES_128_GCM_SHA256)
 * and the most common TLS 1.2 AEAD suites.  AES-CCM adds further TLS 1.2
 * suites.  ChaCha20-Poly1305 is widely preferred when hardware AES is
 * absent (e.g. ARM Cortex-M without AES accelerator).
 * ========================================================================= */

#define PSA_WANT_KEY_TYPE_AES

/** AES-GCM — mandatory for TLS 1.3; dominant in TLS 1.2 AEAD suites. */
#define PSA_WANT_ALG_GCM
#define PSA_WANT_ALG_AEAD_WITH_SHORTENED_TAG

/** AES-CCM — optional TLS 1.2/1.3 suites. */
#define PSA_WANT_ALG_CCM

/** AES-CBC — required for legacy TLS 1.2 CBC cipher suites. */
#define PSA_WANT_ALG_CBC_NO_PADDING
#define PSA_WANT_ALG_CBC_PKCS7

/** ChaCha20-Poly1305 — preferred on devices without AES hardware. */
#define PSA_WANT_KEY_TYPE_CHACHA20
#define PSA_WANT_ALG_CHACHA20_POLY1305
#define PSA_WANT_ALG_STREAM_CIPHER

/* =========================================================================
 * Asymmetric keys — RSA
 *
 * Required to authenticate TLS connections that present RSA certificates
 * (still the majority of public-internet servers) and to perform RSA key
 * exchange in TLS 1.2.
 * ========================================================================= */

#define PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT

/** RSA-PKCS#1 v1.5 signature/verify — TLS 1.2 certificate authentication. */
#define PSA_WANT_ALG_RSA_PKCS1V15_CRYPT
#define PSA_WANT_ALG_RSA_PKCS1V15_SIGN
#define PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW

/** RSA-PSS — TLS 1.3 signature scheme and modern TLS 1.2 suites. */
#define PSA_WANT_ALG_RSA_PSS
#define PSA_WANT_ALG_RSA_PSS_ANY_SALT

/** RSA-OAEP — used for RSA key encapsulation. */
#define PSA_WANT_ALG_RSA_OAEP

/* =========================================================================
 * Asymmetric keys — Elliptic Curve (ECDSA / ECDH)
 *
 * Required for ECDSA certificate authentication and ECDHE key exchange,
 * which dominate modern TLS deployments.
 * ========================================================================= */

#define PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT

/** ECDSA — certificate signature verification. */
#define PSA_WANT_ALG_ECDSA
#define PSA_WANT_ALG_ECDSA_ANY
#define PSA_WANT_ALG_DETERMINISTIC_ECDSA

/** ECDH — key exchange in TLS 1.2 ECDHE suites. */
#define PSA_WANT_ALG_ECDH

/* -------------------------------------------------------------------------
 * Elliptic curves
 *
 * P-256 and P-384 are required by TLS 1.3 (mandatory key-share groups).
 * X25519 is the preferred Diffie-Hellman group in TLS 1.3.
 * P-521 and X448 are optional but provide higher security margins.
 * secp256k1 and Brainpool curves are rarely needed outside specialised
 * deployments; disable them to reduce code size.
 * ------------------------------------------------------------------------- */

/** NIST P-256 (secp256r1) — mandatory TLS 1.3 key share. */
#define PSA_WANT_ECC_SECP_R1_256

/** NIST P-384 (secp384r1) — TLS 1.3 key share; common cert signing curve. */
#define PSA_WANT_ECC_SECP_R1_384

/** NIST P-521 — optional high-security curve. */
#define PSA_WANT_ECC_SECP_R1_521

/** X25519 (Curve25519) — preferred TLS 1.3 DH group. */
#define PSA_WANT_ECC_MONTGOMERY_255

/** X448 (Curve448) — optional high-security DH. */
/* #define PSA_WANT_ECC_MONTGOMERY_448 */

/* =========================================================================
 * Key derivation
 * ========================================================================= */

/** TLS 1.2 PRF (based on HMAC). */
#define PSA_WANT_ALG_TLS12_PRF
#define PSA_WANT_ALG_TLS12_PSK_TO_MS

/** TLS 1.3 key schedule. */
#define PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS

/* =========================================================================
 * Random number generation
 *
 * In mbedTLS 4.x all random generation goes through the PSA layer.
 * psa_generate_random() draws from the platform entropy source automatically;
 * no explicit MBEDTLS_ENTROPY_C / MBEDTLS_CTR_DRBG_C defines are needed.
 * ========================================================================= */

/* No PSA_WANT_* macros are required to enable the default RNG.
 * The entropy source (getrandom / BCryptGenRandom) is selected by the
 * platform driver; no application-level configuration is needed. */

/* =========================================================================
 * Threading
 *
 * In mbedTLS 4.x, threading configuration must appear in psa/crypto_config.h,
 * not in mbedtls_config.h.
 *
 * Rules:
 *   - mbedtls_ssl_context (one per connection) — never shared between threads.
 *   - mbedtls_ssl_config (one per role)        — read-only safe after init.
 *   - PSA key store / RNG                      — protected by the PSA layer.
 * ========================================================================= */

/** Master switch for threading support. */
#define MBEDTLS_THREADING_C

#if defined(_WIN32) && !defined(__MINGW32__)
/** Windows (MSVC): implement the four mutex functions with CRITICAL_SECTION,
 *  then call mbedtls_threading_set_alt() before psa_crypto_init(). */
    #define MBEDTLS_THREADING_ALT
#else
/** Linux, macOS, MinGW/MSYS2: use POSIX threads. */
    #define MBEDTLS_THREADING_PTHREAD
#endif

/* =========================================================================
 * Storage
 *
 * PSA_WANT_KEY_STORAGE_PERSIST enables persistent key storage through the
 * ITS (Internal Trusted Storage) backend.  Not needed for sockpp — all TLS
 * session keys are ephemeral.
 * ========================================================================= */

/* #define PSA_WANT_KEY_STORAGE_PERSIST */

#endif /* SOCKPP_PSA_CRYPTO_CONFIG_H */
