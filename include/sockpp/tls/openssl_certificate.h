/**
 * @file openssl_certificate.h
 *
 * Socket type for OpenSSL TLS/SSL sockets.
 *
 * @author Frank Pagliughi
 * @date January 2025
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2025-2026 Frank Pagliughi
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

#ifndef __sockpp_tls_openssl_certificate_h
#define __sockpp_tls_openssl_certificate_h

#include <openssl/ssl.h>

#include <chrono>
#include <vector>

#include "sockpp/result.h"
#include "sockpp/tls/certificate.h"
#include "sockpp/tls/openssl_error.h"
#include "sockpp/types.h"

namespace sockpp {

// Forward declarations
class tls_context;
class tls_socket;

/////////////////////////////////////////////////////////////////////////////

/**
 * An X509 certificate implemented with OpenSSL.
 */
class tls_certificate
{
    /** The certificate library struct */
    X509* cert_ = nullptr;

    friend class tls_context;
    friend class tls_socket;

    /** Takes ownership of an existing X509 pointer. */
    tls_certificate(X509* cert) : cert_{cert} {}

public:
    /**
     * Creates an empty, invalid certificate.
     */
    tls_certificate() = default;
    /**
     * Copy constructor. Increments the OpenSSL reference count.
     */
    tls_certificate(const tls_certificate& other) : cert_{other.cert_} {
        if (cert_)
            ::X509_up_ref(cert_);
    }
    /**
     * Move constructor.
     */
    tls_certificate(tls_certificate&& other) noexcept : cert_{other.cert_} {
        other.cert_ = nullptr;
    }
    /**
     * Destructor. Decrements the OpenSSL reference count.
     */
    ~tls_certificate() { ::X509_free(cert_); }
    /**
     * Parses a PEM-encoded certificate string.
     * @param pem A PEM-encoded X.509 certificate.
     * @return The certificate, or an error code on failure.
     */
    static result<tls_certificate> from_pem(const string& pem);
    /**
     * Parses a DER-encoded certificate blob.
     * @param der A DER-encoded X.509 certificate.
     * @return The certificate, or an error code on failure.
     */
    static result<tls_certificate> from_der(const binary& der);
    /**
     * Loads a certificate from a PEM or DER file.
     * The format is detected automatically from the file contents.
     * @param path Path to the certificate file.
     * @return The certificate, or an error code on failure.
     */
    static result<tls_certificate> from_file(const string& path);
    /**
     * Parses a PEM bundle containing one or more certificates.
     * @param pem PEM-encoded string holding one or more certificates.
     * @return A certificate chain (leaf first), or an error code on failure.
     */
    static result<vector<tls_certificate>> chain_from_pem(const string& pem);
    /**
     * Loads a certificate chain from a PEM or DER file.
     * A PEM file may contain multiple concatenated certificates.
     * A DER file holds exactly one certificate (returned as a chain of one).
     * @param path Path to the certificate file.
     * @return A certificate chain (leaf first), or an error code on failure.
     */
    static result<vector<tls_certificate>> chain_from_file(const string& path);
    /**
     * Copy assignment. Increments the OpenSSL reference count.
     */
    tls_certificate& operator=(const tls_certificate& rhs) {
        if (&rhs != this) {
            ::X509_free(cert_);
            cert_ = rhs.cert_;
            if (cert_)
                ::X509_up_ref(cert_);
        }
        return *this;
    }
    /**
     * Move assignment.
     */
    tls_certificate& operator=(tls_certificate&& rhs) noexcept {
        if (&rhs != this) {
            ::X509_free(cert_);
            cert_ = rhs.cert_;
            rhs.cert_ = nullptr;
        }
        return *this;
    }
    /**
     * Checks whether this object holds a valid certificate.
     */
    bool is_valid() const { return cert_ != nullptr; }
    /**
     * Gets the subject name for the certificate.
     * @return The subject name for the certificate.
     */
    string subject_name() const;
    /**
     * Gets the issuer name for the certificate.
     * @return The issuer name for the certificate.
     */
    string issuer_name() const;
    /**
     * Gets the certificate's "not before" date as a string.
     * @return The certificate's "not before" date as a string.
     */
    string not_before_str() const;
    /**
     * Gets the certificate's "not after" date as a string.
     * @return The certificate's "not after" date as a string.
     */
    string not_after_str() const;
    /**
     * Gets the certificate's "not before" date as a time_point.
     * @return The certificate's "not before" date.
     */
    std::chrono::system_clock::time_point not_before() const;
    /**
     * Gets the certificate's "not after" date as a time_point.
     * @return The certificate's "not after" date.
     */
    std::chrono::system_clock::time_point not_after() const;
    /**
     * Gets the DER-encoded serial number value bytes.
     * @return The raw serial number bytes (big-endian, no tag or length).
     */
    binary serial_number() const;
    /**
     * Gets the serial number as a lowercase hexadecimal string.
     * @return The serial number in hex, e.g. "4def09a6...".
     */
    string serial_number_hex() const;
    /**
     * Computes the SHA-256 fingerprint of the certificate.
     * This is the SHA-256 digest of the DER-encoded certificate.
     * @return A 32-byte binary holding the SHA-256 digest.
     */
    binary fingerprint_sha256() const;
    /**
     * Gets all Subject Alternative Name (SAN) entries.
     * @return A vector of parsed SAN entries; empty if no SAN extension.
     */
    vector<subject_alt_name> subject_alt_names() const;
    /**
     * Gets the Key Usage extension bitmask.
     * Bit values follow RFC 5280 §4.2.1.3 and the KU_* constants in OpenSSL.
     * @return The key usage bitmask, or 0 if the extension is absent.
     */
    uint32_t key_usage() const;
    /**
     * Gets the Extended Key Usage OIDs as dotted strings.
     * @return A vector of dotted OID strings, e.g. {"1.3.6.1.5.5.7.3.1"}.
     */
    vector<string> extended_key_usage() const;
    /**
     * Gets the certificate as a DER binary blob.
     * @return The certificate as a DER binary blob.
     */
    binary to_der() const;
    /**
     * Gets the certificate as a PEM string.
     * @return The certificate as a PEM string.
     */
    string to_pem() const;
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#include "sockpp/tls/certificate_chain.h"

#endif  // __sockpp_tls_openssl_certificate_h
