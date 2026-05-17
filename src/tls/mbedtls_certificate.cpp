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

#include <arpa/inet.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/pem.h>
#include <mbedtls/x509.h>
#include <psa/crypto.h>

// mbedtls/oid.h is missing extern "C" guards in mbedTLS 4.x
extern "C" {
#include <mbedtls/oid.h>
}

#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>

using namespace std;
using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// Ensures psa_crypto_init() has been called before any certificate parsing.
// Certificate parsing uses the PSA algorithm table; without this call, OID
// lookups fail with MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG even for modern certs.
static void ensure_psa_init() {
    static once_flag once;
    call_once(once, [] {
        psa_status_t status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            throw std::system_error{
                std::error_code{static_cast<int>(status), std::system_category()},
                "psa_crypto_init failed"
            };
        }
    });
}

// File-local helper: release and delete a cert struct (null-safe).
static void free_cert(mbedtls_x509_crt* p) {
    if (p) {
        mbedtls_x509_crt_free(p);
        delete p;
    }
}

// Static helper: allocate and initialise a fresh mbedtls_x509_crt on the heap.
// Also ensures PSA Crypto is initialised, which is required before any
// certificate parsing in mbedTLS 4.x.
mbedtls_x509_crt* tls_certificate::make_cert() {
    ensure_psa_init();
    auto* p = new mbedtls_x509_crt;
    mbedtls_x509_crt_init(p);
    return p;
}

// Deep-copy src by serialising its raw DER bytes into a fresh struct.
mbedtls_x509_crt* tls_certificate::clone_cert(const mbedtls_x509_crt* src) {
    if (!src || src->raw.len == 0)
        return nullptr;

    auto* p = make_cert();
    int ret = mbedtls_x509_crt_parse_der(p, src->raw.p, src->raw.len);
    if (ret != 0) {
        free_cert(p);
        return nullptr;
    }
    return p;
}

// --------------------------------------------------------------------------
// Special members

tls_certificate::~tls_certificate() { free_cert(cert_); }

tls_certificate& tls_certificate::operator=(const tls_certificate& rhs) {
    if (this != &rhs) {
        free_cert(cert_);
        cert_ = clone_cert(rhs.cert_);
    }
    return *this;
}

tls_certificate& tls_certificate::operator=(tls_certificate&& rhs) noexcept {
    if (this != &rhs) {
        free_cert(cert_);
        cert_ = rhs.cert_;
        rhs.cert_ = nullptr;
    }
    return *this;
}

// --------------------------------------------------------------------------

result<tls_certificate> tls_certificate::from_pem(const string& pem) {
    auto* cert = make_cert();

    // mbedtls_x509_crt_parse() requires the buffer to include the NUL
    // terminator when the input is PEM-encoded.
    int ret = mbedtls_x509_crt_parse(
        cert, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1
    );
    if (ret != 0) {
        free_cert(cert);
        return make_tls_error_code(ret);
    }

    return tls_certificate{cert};
}

result<tls_certificate> tls_certificate::from_der(const binary& der) {
    auto* cert = make_cert();

    int ret = mbedtls_x509_crt_parse_der(
        cert, reinterpret_cast<const unsigned char*>(der.data()), der.size()
    );
    if (ret != 0) {
        free_cert(cert);
        return make_tls_error_code(ret);
    }

    return tls_certificate{cert};
}

result<tls_certificate> tls_certificate::from_file(const string& path) {
    std::ifstream fil{path, std::ios::binary};
    if (!fil.is_open())
        return error_code{errno, std::generic_category()};

    string content{std::istreambuf_iterator<char>{fil}, std::istreambuf_iterator<char>{}};
    if (fil.bad())
        return error_code{errno, std::generic_category()};

    if (content.size() >= 5 && content.compare(0, 5, "-----") == 0)
        return from_pem(content);

    return from_der(binary{content.begin(), content.end()});
}

result<vector<tls_certificate>> tls_certificate::chain_from_pem(const string& pem) {
    auto* crt = make_cert();

    // Parse the full PEM bundle into a linked list.
    // A positive return means some certs failed (partial success);
    // negative means total failure.
    int ret = mbedtls_x509_crt_parse(
        crt, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1
    );
    if (ret < 0) {
        free_cert(crt);
        return make_tls_error_code(ret);
    }

    vector<tls_certificate> chain;
    for (const mbedtls_x509_crt* link = crt; link != nullptr; link = link->next) {
        binary der{link->raw.p, link->raw.p + link->raw.len};
        if (auto res = from_der(der); res)
            chain.push_back(res.release());
    }

    free_cert(crt);

    if (chain.empty())
        return make_error_code(std::errc::invalid_argument);
    return chain;
}

result<vector<tls_certificate>> tls_certificate::chain_from_file(const string& path) {
    std::ifstream fil{path, std::ios::binary};
    if (!fil.is_open())
        return error_code{errno, std::generic_category()};

    string content{std::istreambuf_iterator<char>{fil}, std::istreambuf_iterator<char>{}};
    if (fil.bad())
        return error_code{errno, std::generic_category()};

    if (content.size() >= 5 && content.compare(0, 5, "-----") == 0)
        return chain_from_pem(content);

    // DER holds exactly one certificate.
    if (auto res = from_der(binary{content.begin(), content.end()}); res) {
        vector<tls_certificate> chain;
        chain.push_back(res.release());
        return chain;
    }
    else
        return res.error();
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
        buf, sizeof(buf), "%04d%02d%02d%02d%02d%02dZ", t.year, t.mon, t.day, t.hour, t.min,
        t.sec
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

// Helper: bytes to lowercase hex string
static string bytes_to_hex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) oss << std::setw(2) << static_cast<unsigned>(data[i]);
    return oss.str();
}

// Helper: convert mbedtls_x509_time to a UTC time_point
static system_clock::time_point x509_time_to_tp(const mbedtls_x509_time& t) {
    struct tm tm = {};
    tm.tm_year = t.year - 1900;
    tm.tm_mon = t.mon - 1;
    tm.tm_mday = t.day;
    tm.tm_hour = t.hour;
    tm.tm_min = t.min;
    tm.tm_sec = t.sec;
    time_t ts = timegm(&tm);
    if (ts == (time_t)-1)
        return {};
    return system_clock::from_time_t(ts);
}

system_clock::time_point tls_certificate::not_before() const {
    return cert_ ? x509_time_to_tp(cert_->valid_from)
                 : system_clock::time_point{};
}

system_clock::time_point tls_certificate::not_after() const {
    return cert_ ? x509_time_to_tp(cert_->valid_to) : system_clock::time_point{};
}

binary tls_certificate::serial_number() const {
    if (!cert_ || cert_->serial.len == 0)
        return {};
    return binary{cert_->serial.p, cert_->serial.p + cert_->serial.len};
}

string tls_certificate::serial_number_hex() const {
    if (!cert_ || cert_->serial.len == 0)
        return {};
    return bytes_to_hex(cert_->serial.p, cert_->serial.len);
}

binary tls_certificate::fingerprint_sha256() const {
    if (!cert_ || cert_->raw.len == 0)
        return {};

    binary digest(32, uint8_t{0});
    int ret = mbedtls_md(
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), cert_->raw.p, cert_->raw.len,
        reinterpret_cast<unsigned char*>(digest.data())
    );
    if (ret != 0)
        return {};
    return digest;
}

vector<subject_alt_name> tls_certificate::subject_alt_names() const {
    vector<subject_alt_name> result;
    if (!cert_)
        return result;
    if (!mbedtls_x509_crt_has_ext_type(cert_, MBEDTLS_X509_EXT_SUBJECT_ALT_NAME))
        return result;

    const mbedtls_x509_sequence* seq = &cert_->subject_alt_names;
    for (; seq != nullptr; seq = seq->next) {
        if (seq->buf.len == 0)
            continue;

        mbedtls_x509_subject_alternative_name san_parsed;
        int ret = mbedtls_x509_parse_subject_alt_name(&seq->buf, &san_parsed);
        if (ret != 0)
            continue;

        subject_alt_name san;
        switch (san_parsed.type) {
            case MBEDTLS_X509_SAN_DNS_NAME:
                san.kind = subject_alt_name::type::DNS;
                san.value = string{
                    reinterpret_cast<const char*>(san_parsed.san.unstructured_name.p),
                    san_parsed.san.unstructured_name.len
                };
                break;
            case MBEDTLS_X509_SAN_IP_ADDRESS: {
                san.kind = subject_alt_name::type::IP;
                const auto* addr = san_parsed.san.unstructured_name.p;
                size_t addr_len = san_parsed.san.unstructured_name.len;
                char ipbuf[INET6_ADDRSTRLEN] = {};
                int af = (addr_len == 4) ? AF_INET : AF_INET6;
                if (inet_ntop(af, addr, ipbuf, sizeof(ipbuf)))
                    san.value = ipbuf;
                break;
            }
            case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
                san.kind = subject_alt_name::type::URI;
                san.value = string{
                    reinterpret_cast<const char*>(san_parsed.san.unstructured_name.p),
                    san_parsed.san.unstructured_name.len
                };
                break;
            case MBEDTLS_X509_SAN_RFC822_NAME:
                san.kind = subject_alt_name::type::EMAIL;
                san.value = string{
                    reinterpret_cast<const char*>(san_parsed.san.unstructured_name.p),
                    san_parsed.san.unstructured_name.len
                };
                break;
            default:
                san.kind = subject_alt_name::type::OTHER;
                break;
        }
        result.push_back(std::move(san));
        mbedtls_x509_free_subject_alt_name(&san_parsed);
    }
    return result;
}

uint32_t tls_certificate::key_usage() const {
    if (!cert_)
        return 0;
    if (!mbedtls_x509_crt_has_ext_type(cert_, MBEDTLS_X509_EXT_KEY_USAGE))
        return 0;

    // Build the bitmask by probing each known flag.
    static const uint32_t all_flags[] = {
        MBEDTLS_X509_KU_DIGITAL_SIGNATURE, MBEDTLS_X509_KU_NON_REPUDIATION,
        MBEDTLS_X509_KU_KEY_ENCIPHERMENT,  MBEDTLS_X509_KU_DATA_ENCIPHERMENT,
        MBEDTLS_X509_KU_KEY_AGREEMENT,     MBEDTLS_X509_KU_KEY_CERT_SIGN,
        MBEDTLS_X509_KU_CRL_SIGN,          MBEDTLS_X509_KU_ENCIPHER_ONLY,
        MBEDTLS_X509_KU_DECIPHER_ONLY,
    };
    uint32_t flags = 0;
    for (auto f : all_flags) {
        if (mbedtls_x509_crt_check_key_usage(cert_, f) == 0)
            flags |= f;
    }
    return flags;
}

vector<string> tls_certificate::extended_key_usage() const {
    vector<string> result;
    if (!cert_)
        return result;
    if (!mbedtls_x509_crt_has_ext_type(cert_, MBEDTLS_X509_EXT_EXTENDED_KEY_USAGE))
        return result;

    const mbedtls_x509_sequence* seq = &cert_->ext_key_usage;
    for (; seq != nullptr; seq = seq->next) {
        if (seq->buf.len == 0)
            continue;
        char oidbuf[64];
        int ret = mbedtls_oid_get_numeric_string(oidbuf, sizeof(oidbuf), &seq->buf);
        if (ret > 0)
            result.emplace_back(oidbuf);
    }
    return result;
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
    mbedtls_pem_write_buffer(header, footer, cert_->raw.p, cert_->raw.len, nullptr, 0, &olen);

    if (olen == 0)
        return {};

    unique_ptr<unsigned char[]> buf{new unsigned char[olen]};
    int ret = mbedtls_pem_write_buffer(
        header, footer, cert_->raw.p, cert_->raw.len, buf.get(), olen, &olen
    );
    if (ret != 0)
        return {};

    // olen includes the NUL terminator; exclude it from the string.
    return string{reinterpret_cast<const char*>(buf.get()), olen > 0 ? olen - 1 : 0};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
