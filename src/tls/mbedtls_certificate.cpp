// mbedtls_certificate.cpp
//
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

#include "sockpp/tls/mbedtls_certificate.h"

#include <mbedtls/error.h>
#include <mbedtls/oid.h>
#include <mbedtls/pem.h>

#include <cstdio>
#include <memory>

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// Static helper: allocate and initialise a fresh mbedtls_x509_crt on the
// heap, wrapped in a shared_ptr with a custom deleter that calls
// mbedtls_x509_crt_free() before releasing the memory.
std::shared_ptr<mbedtls_x509_crt> tls_certificate::make_cert() {
    auto* p = new mbedtls_x509_crt;
    mbedtls_x509_crt_init(p);
    return {p, [](mbedtls_x509_crt* c) {
                mbedtls_x509_crt_free(c);
                delete c;
            }};
}

// --------------------------------------------------------------------------

result<tls_certificate> tls_certificate::from_pem(const string& pem) {
    auto cert = make_cert();

    // mbedtls_x509_crt_parse() requires the buffer to include the NUL
    // terminator when the input is PEM-encoded.
    int ret = mbedtls_x509_crt_parse(
        cert.get(),
        reinterpret_cast<const unsigned char*>(pem.c_str()),
        pem.size() + 1
    );
    if (ret != 0)
        return make_tls_error_code(ret);

    return tls_certificate{std::move(cert)};
}

result<tls_certificate> tls_certificate::from_der(const binary& der) {
    auto cert = make_cert();

    int ret = mbedtls_x509_crt_parse_der(
        cert.get(),
        reinterpret_cast<const unsigned char*>(der.data()),
        der.size()
    );
    if (ret != 0)
        return make_tls_error_code(ret);

    return tls_certificate{std::move(cert)};
}

// --------------------------------------------------------------------------

string tls_certificate::subject_name() const {
    if (!cert_)
        return {};

    char buf[512];
    int ret = mbedtls_x509_dn_gets(buf, sizeof(buf), &cert_->subject);
    return (ret > 0) ? string{buf, size_t(ret)} : string{};
}

string tls_certificate::issuer_name() const {
    if (!cert_)
        return {};

    char buf[512];
    int ret = mbedtls_x509_dn_gets(buf, sizeof(buf), &cert_->issuer);
    return (ret > 0) ? string{buf, size_t(ret)} : string{};
}

// Format an mbedtls_x509_time as "YYYYMMDDHHMMSSZ".
static string format_time(const mbedtls_x509_time& t) {
    char buf[16];
    std::snprintf(
        buf, sizeof(buf),
        "%04d%02d%02d%02d%02d%02dZ",
        t.year, t.mon, t.day,
        t.hour, t.min, t.sec
    );
    return buf;
}

string tls_certificate::not_before_str() const {
    return cert_ ? format_time(cert_->valid_from) : string{};
}

string tls_certificate::not_after_str() const {
    return cert_ ? format_time(cert_->valid_to) : string{};
}

// --------------------------------------------------------------------------

binary tls_certificate::to_der() const {
    if (!cert_ || cert_->raw.len == 0)
        return {};

    return binary{cert_->raw.p, cert_->raw.p + cert_->raw.len};
}

string tls_certificate::to_pem() const {
    if (!cert_ || cert_->raw.len == 0)
        return {};

    // PEM output is at most ~1.4× the DER size plus headers.
    // mbedtls_pem_write_buffer() tells us the required size on the first call
    // (returns MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL and sets olen).
    const char* header = "-----BEGIN CERTIFICATE-----\n";
    const char* footer = "-----END CERTIFICATE-----\n";

    size_t olen = 0;
    mbedtls_pem_write_buffer(
        header, footer,
        cert_->raw.p, cert_->raw.len,
        nullptr, 0, &olen
    );

    if (olen == 0)
        return {};

    std::unique_ptr<unsigned char[]> buf{new unsigned char[olen]};
    int ret = mbedtls_pem_write_buffer(
        header, footer,
        cert_->raw.p, cert_->raw.len,
        buf.get(), olen, &olen
    );
    if (ret != 0)
        return {};

    // olen includes the NUL terminator; exclude it from the string.
    return string{reinterpret_cast<const char*>(buf.get()), olen > 0 ? olen - 1 : 0};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
