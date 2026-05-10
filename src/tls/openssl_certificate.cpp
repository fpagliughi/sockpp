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

#include <arpa/inet.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <sstream>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

result<tls_certificate> tls_certificate::from_pem(const string& pem) {
    auto bio_deleter = [](BIO* b) { BIO_free(b); };
    unique_ptr<BIO, decltype(bio_deleter)> bio{
        BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())), bio_deleter
    };
    if (!bio)
        return tls_last_error();

    ERR_clear_error();
    X509* cert = PEM_read_bio_X509_AUX(bio.get(), nullptr, nullptr, nullptr);
    if (!cert)
        return tls_last_error();

    return tls_certificate{cert};
}

result<tls_certificate> tls_certificate::from_der(const binary& der) {
    const uint8_t* p = der.data();
    X509* cert = d2i_X509(nullptr, &p, static_cast<long>(der.size()));
    if (!cert)
        return tls_last_error();

    return tls_certificate{cert};
}

result<tls_certificate> tls_certificate::from_file(const string& path) {
    std::ifstream f{path, std::ios::binary};
    if (!f.is_open())
        return error_code{errno, std::generic_category()};

    string content{std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{}};
    if (f.bad())
        return error_code{errno, std::generic_category()};

    if (content.size() >= 5 && content.compare(0, 5, "-----") == 0)
        return from_pem(content);

    return from_der(binary{content.begin(), content.end()});
}

result<std::vector<tls_certificate>> tls_certificate::chain_from_pem(const string& pem) {
    auto bio_deleter = [](BIO* b) { BIO_free(b); };
    unique_ptr<BIO, decltype(bio_deleter)> bio{
        BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())), bio_deleter
    };
    if (!bio)
        return tls_last_error();

    std::vector<tls_certificate> chain;
    ERR_clear_error();
    for (;;) {
        X509* cert = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
        if (!cert) {
            unsigned long err = ERR_peek_last_error();
            if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
                ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
                ERR_clear_error();
                break;
            }
            return tls_last_error();
        }
        chain.push_back(tls_certificate{cert});
    }
    if (chain.empty())
        return make_error_code(std::errc::invalid_argument);
    return chain;
}

result<std::vector<tls_certificate>> tls_certificate::chain_from_file(const string& path) {
    std::ifstream f{path, std::ios::binary};
    if (!f.is_open())
        return error_code{errno, std::generic_category()};

    string content{std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{}};
    if (f.bad())
        return error_code{errno, std::generic_category()};

    if (content.size() >= 5 && content.compare(0, 5, "-----") == 0)
        return chain_from_pem(content);

    // DER holds exactly one certificate.
    if (auto res = from_der(binary{content.begin(), content.end()}); res) {
        std::vector<tls_certificate> chain;
        chain.push_back(res.release());
        return chain;
    }
    else
        return res.error();
}

string tls_certificate::subject_name() const {
    auto name = X509_get_subject_name(cert_);
    if (!name)
        return string{};
    char* name_str = X509_NAME_oneline(name, nullptr, 0);
    if (!name_str)
        return string{};
    string result{name_str};
    OPENSSL_free(name_str);
    return result;
}

// int X509_set_subject_name(X509 *x, const X509_NAME *name);

string tls_certificate::issuer_name() const {
    auto name = X509_get_issuer_name(cert_);
    if (!name)
        return string{};
    char* name_str = X509_NAME_oneline(name, nullptr, 0);
    if (!name_str)
        return string{};
    string result{name_str};
    OPENSSL_free(name_str);
    return result;
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

// Helper: convert bytes to lowercase hex string
static string bytes_to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) oss << std::setw(2) << static_cast<unsigned>(data[i]);
    return oss.str();
}

// Helper: convert an ASN1_TIME to a UTC time_point (epoch on error)
static std::chrono::system_clock::time_point asn1_time_to_tp(const ASN1_TIME* tm) {
    if (!tm)
        return {};
    struct tm t = {};
    if (ASN1_TIME_to_tm(tm, &t) != 1)
        return {};
    time_t ts = timegm(&t);
    if (ts == (time_t)-1)
        return {};
    return std::chrono::system_clock::from_time_t(ts);
}

std::chrono::system_clock::time_point tls_certificate::not_before() const {
    return cert_ ? asn1_time_to_tp(X509_get0_notBefore(cert_))
                 : std::chrono::system_clock::time_point{};
}

std::chrono::system_clock::time_point tls_certificate::not_after() const {
    return cert_ ? asn1_time_to_tp(X509_get0_notAfter(cert_))
                 : std::chrono::system_clock::time_point{};
}

binary tls_certificate::serial_number() const {
    if (!cert_)
        return {};
    const ASN1_INTEGER* s = X509_get0_serialNumber(cert_);
    if (!s || s->length <= 0)
        return {};
    return binary{s->data, s->data + s->length};
}

string tls_certificate::serial_number_hex() const {
    const auto sn = serial_number();
    return bytes_to_hex(sn.data(), sn.size());
}

binary tls_certificate::fingerprint_sha256() const {
    if (!cert_)
        return {};
    const auto der = to_der();
    if (der.empty())
        return {};
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdlen = 0;
    if (!EVP_Digest(der.data(), der.size(), md, &mdlen, EVP_sha256(), nullptr))
        return {};
    return binary{md, md + mdlen};
}

std::vector<subject_alt_name> tls_certificate::subject_alt_names() const {
    std::vector<subject_alt_name> result;
    if (!cert_)
        return result;

    auto* sans = static_cast<GENERAL_NAMES*>(
        X509_get_ext_d2i(cert_, NID_subject_alt_name, nullptr, nullptr)
    );
    if (!sans)
        return result;

    int n = sk_GENERAL_NAME_num(sans);
    result.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        const GENERAL_NAME* gn = sk_GENERAL_NAME_value(sans, i);
        if (!gn)
            continue;

        subject_alt_name san;
        switch (gn->type) {
            case GEN_DNS:
                san.kind = subject_alt_name::type::DNS;
                san.value = string{
                    reinterpret_cast<const char*>(ASN1_STRING_get0_data(gn->d.dNSName)),
                    static_cast<size_t>(ASN1_STRING_length(gn->d.dNSName))
                };
                break;
            case GEN_IPADD: {
                san.kind = subject_alt_name::type::IP;
                const auto* addr = ASN1_STRING_get0_data(gn->d.iPAddress);
                int addr_len = ASN1_STRING_length(gn->d.iPAddress);
                char ipbuf[INET6_ADDRSTRLEN] = {};
                int af = (addr_len == 4) ? AF_INET : AF_INET6;
                if (inet_ntop(af, addr, ipbuf, sizeof(ipbuf)))
                    san.value = ipbuf;
                break;
            }
            case GEN_URI:
                san.kind = subject_alt_name::type::URI;
                san.value = string{
                    reinterpret_cast<const char*>(
                        ASN1_STRING_get0_data(gn->d.uniformResourceIdentifier)
                    ),
                    static_cast<size_t>(ASN1_STRING_length(gn->d.uniformResourceIdentifier))
                };
                break;
            case GEN_EMAIL:
                san.kind = subject_alt_name::type::EMAIL;
                san.value = string{
                    reinterpret_cast<const char*>(ASN1_STRING_get0_data(gn->d.rfc822Name)),
                    static_cast<size_t>(ASN1_STRING_length(gn->d.rfc822Name))
                };
                break;
            default:
                san.kind = subject_alt_name::type::OTHER;
                break;
        }
        result.push_back(std::move(san));
    }

    GENERAL_NAMES_free(sans);
    return result;
}

uint32_t tls_certificate::key_usage() const {
    if (!cert_)
        return 0;
    // X509_get_key_usage returns UINT32_MAX when the extension is absent.
    uint32_t ku = static_cast<uint32_t>(X509_get_key_usage(cert_));
    return (ku == UINT32_MAX) ? 0 : ku;
}

std::vector<string> tls_certificate::extended_key_usage() const {
    std::vector<string> result;
    if (!cert_)
        return result;

    auto* eku = static_cast<EXTENDED_KEY_USAGE*>(
        X509_get_ext_d2i(cert_, NID_ext_key_usage, nullptr, nullptr)
    );
    if (!eku)
        return result;

    int n = sk_ASN1_OBJECT_num(eku);
    result.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        const ASN1_OBJECT* obj = sk_ASN1_OBJECT_value(eku, i);
        if (!obj)
            continue;
        char buf[128];
        if (OBJ_obj2txt(buf, sizeof(buf), obj, 1) > 0)
            result.emplace_back(buf);
    }

    EXTENDED_KEY_USAGE_free(eku);
    return result;
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
    OPENSSL_free(buf);

    return certBin;
}

string tls_certificate::to_pem() const {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio || !PEM_write_bio_X509(bio, cert_)) {
        BIO_vfree(bio);
        return string{};
    }

    size_t keylen = BIO_pending(bio);
    unique_ptr<char[]> key(new char[keylen]);

    int len = BIO_read(bio, key.get(), (int)keylen);
    BIO_vfree(bio);

    return (len > 0) ? string{key.get(), (size_t)len} : string{};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
