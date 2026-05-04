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

#include <memory>

#include "sockpp/result.h"
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
 * The underlying mbedTLS certificate structure is reference-counted via a
 * @c shared_ptr, so copies are cheap and the struct is freed only when the
 * last @c tls_certificate referencing it is destroyed.
 */
class tls_certificate
{
    /** Shared ownership of the mbedTLS certificate structure. */
    std::shared_ptr<mbedtls_x509_crt> cert_;

    friend class mbedtls_context;
    friend class mbedtls_socket;

    /** Allocates and initialises a new mbedTLS cert struct. */
    static std::shared_ptr<mbedtls_x509_crt> make_cert();

    /** Takes shared ownership of an already-initialised certificate. */
    explicit tls_certificate(std::shared_ptr<mbedtls_x509_crt> cert)
        : cert_{std::move(cert)} {}

public:
    /**
     * Creates an empty, invalid certificate.
     */
    tls_certificate() = default;
    /**
     * Copy constructor. Both objects share ownership of the underlying struct.
     */
    tls_certificate(const tls_certificate&) = default;
    /**
     * Move constructor.
     */
    tls_certificate(tls_certificate&&) noexcept = default;
    /**
     * Destructor. Frees the certificate when the last owner is destroyed.
     */
    ~tls_certificate() = default;
    /**
     * Copy assignment.
     */
    tls_certificate& operator=(const tls_certificate&) = default;
    /**
     * Move assignment.
     */
    tls_certificate& operator=(tls_certificate&&) noexcept = default;
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

#endif  // __sockpp_tls_mbedtls_certificate_h
