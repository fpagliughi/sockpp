// openssl_certificate.cpp
//
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

#include "sockpp/tls/openssl_certificate.h"

#include <openssl/bio.h>
#include <openssl/pem.h>

#include <memory>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

string tls_certificate::subject_name() const {
    auto name = X509_get_subject_name(cert_);
    if (!name)
        return string{};
    const char* name_str = X509_NAME_oneline(name, NULL, 0);
    return (name_str) ? string{name_str} : string{};
}

// int X509_set_subject_name(X509 *x, const X509_NAME *name);

string tls_certificate::issuer_name() const {
    auto name = X509_get_issuer_name(cert_);
    if (!name)
        return string{};
    const char* name_str = X509_NAME_oneline(name, NULL, 0);
    return (name_str) ? string{name_str} : string{};
}

// int X509_set_issuer_name(X509 *x, const X509_NAME *name);

string tls_certificate::not_before_str() const {
    auto tm = X509_get0_notBefore(cert_);
    return (tm && tm->data) ? string{(const char*)tm->data} : string{};
}

string tls_certificate::not_after_str() const {
    auto tm = X509_get0_notAfter(cert_);
    return (tm && tm->data) ? string{(const char*)tm->data} : string{};
}

binary tls_certificate::to_der() const {
    if (!cert_)
        return binary{};

    uint8_t* buf = nullptr;
    int len = i2d_X509(cert_, &buf);

    // TODO: Return an error result on <0?
    if (len <= 0)
        return binary{};

    binary certBin{buf, size_t(len)};
    ::OPENSSL_free(buf);

    return certBin;
}

string tls_certificate::to_pem() const {
    BIO* bio = ::BIO_new(BIO_s_mem());
    if (!bio || !::PEM_write_bio_X509(bio, cert_)) {
        ::BIO_vfree(bio);
        return string{};
    }

    size_t keylen = BIO_pending(bio);
    std::unique_ptr<char[]> key(new char[keylen]);

    int len = ::BIO_read(bio, key.get(), (int)keylen);
    ::BIO_vfree(bio);

    return (len > 0) ? string{key.get(), (size_t)len} : string{};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
