/**
 * @file mbedtls_certificate.h
 *
 * X.509 certificate wrapper for the mbedTLS backend.
 *
 * @author Frank Pagliughi
 * @date May 2026
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2026 Frank Pagliughi
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

#ifndef __sockpp_tls_mbedtls_certificate_h
#define __sockpp_tls_mbedtls_certificate_h

#include <mbedtls/x509_crt.h>

#include <chrono>
#include <vector>

#include "sockpp/result.h"
#include "sockpp/tls/certificate.h"
#include "sockpp/tls/mbedtls_error.h"
#include "sockpp/types.h"

namespace sockpp {

// Forward declarations
class mbedtls_context;
class mbedtls_socket;

/////////////////////////////////////////////////////////////////////////////

/**
 * An X.509 certificate implemented with mbedTLS.
 *
 * Each instance owns its own @c mbedtls_x509_crt struct.  Copying is done by
 * a DER round-trip (serialize → parse into a fresh struct) so that every copy
 * is fully independent.  Moving transfers ownership without any allocation.
 */
class tls_certificate
{
    /** Sole owner of the mbedTLS certificate structure. */
    mbedtls_x509_crt* cert_ = nullptr;

    friend class mbedtls_context;
    friend class mbedtls_socket;

    /** Allocates and initialises a new mbedTLS cert struct. */
    static mbedtls_x509_crt* make_cert();

    /**
     * Deep-copies @p src by serialising to DER and parsing into a fresh struct.
     * Returns @c nullptr if @p src is null, empty, or the parse fails.
     */
    static mbedtls_x509_crt* clone_cert(const mbedtls_x509_crt* src);

    /** Takes ownership of an already-initialised certificate. */
    explicit tls_certificate(mbedtls_x509_crt* cert) : cert_{cert} {}

public:
    /**
     * Creates an empty, invalid certificate.
     */
    tls_certificate() = default;
    /**
     * Copy constructor.
     * Performs a deep copy via a DER round-trip.
     */
    tls_certificate(const tls_certificate& other) : cert_{clone_cert(other.cert_)} {}
    /**
     * Move constructor.
     */
    tls_certificate(tls_certificate&& other) noexcept : cert_{other.cert_} {
        other.cert_ = nullptr;
    }
    /**
     * Destructor.
     */
    ~tls_certificate();
    /**
     * Copy assignment.
     * Performs a deep copy via a DER round-trip.
     */
    tls_certificate& operator=(const tls_certificate& rhs);
    /**
     * Move assignment.
     */
    tls_certificate& operator=(tls_certificate&& rhs) noexcept;
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
    static result<std::vector<tls_certificate>> chain_from_pem(const string& pem);
    /**
     * Loads a certificate chain from a PEM or DER file.
     * A PEM file may contain multiple concatenated certificates.
     * A DER file holds exactly one certificate (returned as a chain of one).
     * @param path Path to the certificate file.
     * @return A certificate chain (leaf first), or an error code on failure.
     */
    static result<std::vector<tls_certificate>> chain_from_file(const string& path);
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
     * The format is "YYYYMMDDHHMMSSZ".
     * @return The certificate's "not before" date as a string.
     */
    string not_before_str() const;
    /**
     * Gets the certificate's "not after" date as a string.
     * The format is "YYYYMMDDHHMMSSZ".
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
    std::vector<subject_alt_name> subject_alt_names() const;
    /**
     * Gets the Key Usage extension bitmask.
     * Bit values follow RFC 5280 §4.2.1.3 and the MBEDTLS_X509_KU_* constants.
     * @return The key usage bitmask, or 0 if the extension is absent.
     */
    uint32_t key_usage() const;
    /**
     * Gets the Extended Key Usage OIDs as dotted strings.
     * @return A vector of dotted OID strings, e.g. {"1.3.6.1.5.5.7.3.1"}.
     */
    std::vector<string> extended_key_usage() const;
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

/**
 * An ordered sequence of X.509 certificates forming a certificate chain.
 * The first element is the leaf (end-entity) certificate; subsequent
 * elements are intermediate CAs in order toward the root.
 */
class tls_certificate_chain
{
    /** The certificates in the chain, leaf first. */
    std::vector<tls_certificate> certs_;

public:
    /** Creates an empty chain. */
    tls_certificate_chain() = default;

    /**
     * Constructs a chain from a vector, copying it.
     * @param certs The certificates to copy into this chain.
     */
    explicit tls_certificate_chain(const std::vector<tls_certificate>& certs)
        : certs_{certs} {}

    /**
     * Constructs a chain from a vector, moving it.
     * @param certs The certificates to move into this chain.
     */
    explicit tls_certificate_chain(std::vector<tls_certificate>&& certs) noexcept
        : certs_{std::move(certs)} {}

    /**
     * Parses a PEM bundle containing one or more certificates.
     * @param pem PEM-encoded string holding one or more certificates.
     * @return A chain (leaf first), or an error code on failure.
     */
    static result<tls_certificate_chain> from_pem(const string& pem) {
        auto res = tls_certificate::chain_from_pem(pem);
        if (!res)
            return res.error();
        return tls_certificate_chain{res.release()};
    }

    /**
     * Loads a certificate chain from a PEM or DER file.
     * A PEM file may contain multiple concatenated certificates.
     * A DER file holds exactly one certificate (returned as a chain of one).
     * @param path Path to the certificate file.
     * @return A chain (leaf first), or an error code on failure.
     */
    static result<tls_certificate_chain> from_file(const string& path) {
        auto res = tls_certificate::chain_from_file(path);
        if (!res)
            return res.error();
        return tls_certificate_chain{res.release()};
    }

    // --- Read-only container interface ---

    /** Returns true if the chain contains no certificates. */
    bool empty() const noexcept { return certs_.empty(); }
    /** Returns the number of certificates in the chain. */
    size_t size() const noexcept { return certs_.size(); }

    /** Returns the certificate at index @p i (no bounds check). */
    const tls_certificate& operator[](size_t i) const { return certs_[i]; }
    /** Returns the certificate at index @p i (throws std::out_of_range). */
    const tls_certificate& at(size_t i) const { return certs_.at(i); }
    /** Returns the leaf (end-entity) certificate. Undefined if empty. */
    const tls_certificate& leaf() const { return certs_.front(); }

    auto cbegin() const noexcept { return certs_.cbegin(); }
    auto cend() const noexcept { return certs_.cend(); }
    auto begin() const noexcept { return certs_.cbegin(); }
    auto end() const noexcept { return certs_.cend(); }

    /** Appends a certificate to the chain (copy). */
    void push_back(const tls_certificate& cert) { certs_.push_back(cert); }
    /** Appends a certificate to the chain (move). */
    void push_back(tls_certificate&& cert) { certs_.push_back(std::move(cert)); }

    // --- Certificate chain properties ---

    /**
     * Checks structural validity of the chain.
     *
     * - Empty chain: @em false.
     * - Single certificate: @em true iff the certificate is non-null and
     *   the current time falls within its validity window (not_before …
     *   not_after).
     * - Two or more certificates: @em true iff each certificate's issuer
     *   name matches the subject name of the next certificate in the chain.
     *
     * This is a lightweight structural check only; it does not perform
     * cryptographic signature verification.
     */
    bool is_valid() const {
        if (certs_.empty())
            return false;
        if (certs_.size() == 1) {
            if (!certs_.front().is_valid())
                return false;
            auto now = std::chrono::system_clock::now();
            return now >= certs_.front().not_before() && now <= certs_.front().not_after();
        }
        for (size_t i = 0; i + 1 < certs_.size(); ++i) {
            if (certs_[i].issuer_name() != certs_[i + 1].subject_name())
                return false;
        }
        return true;
    }

    /**
     * Returns true if the leaf certificate's subject and issuer names are
     * identical (i.e. it is self-signed).
     */
    bool is_self_signed() const {
        return !certs_.empty() &&
               certs_.front().subject_name() == certs_.front().issuer_name();
    }

    /**
     * Returns the PEM encoding of all certificates concatenated in order.
     */
    string to_pem() const {
        string result;
        for (const auto& cert : certs_) result += cert.to_pem();
        return result;
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_mbedtls_certificate_h
