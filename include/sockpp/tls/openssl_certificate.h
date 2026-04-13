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
// Copyright (c) 2025 Frank Pagliughi
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

#include "sockpp/types.h"

namespace sockpp {

// Forward declaration
class tls_socket;

/////////////////////////////////////////////////////////////////////////////

/**
 * An X509 certificate implemented with OpenSSL.
 */
class tls_certificate
{
    /** The certificate library struct */
    X509* cert_;

    friend class tls_socket;

    /** Object takes ownership of the pointer */
    tls_certificate(X509* cert) : cert_{cert} {}

public:
    /**
     * Destructor.
     */
    ~tls_certificate() { ::X509_free(cert_); }
    /**
     * Gets the subject name for the certificate.
     * @return The subject name for the certificate.
     */
    string subject_name() const;
    /**
     * Gets the subject name for the certificate.
     * @return The subject name for the certificate.
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

#endif  // __sockpp_tls_openssl_certificate_h
