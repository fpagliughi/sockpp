// mbedtls_context.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2024 Frank Pagliughi
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

#include "sockpp/tls/mbedtls/mbedtls_context.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

#include <cassert>
#include <chrono>
#include <mutex>

#include "sockpp/connector.h"
#include "sockpp/tls/mbedtls/mbedtls_socket.h"

#ifdef __APPLE__
    #include <TargetConditionals.h>
    #include <fcntl.h>
    #ifdef TARGET_OS_OSX
    // For macOS read_system_root_certs():
        #include <Security/Security.h>
    #endif
#elif !defined(_WIN32)
// For Unix read_system_root_certs():
    #include <dirent.h>
    #include <fcntl.h>
    #include <fnmatch.h>
    #include <sys/stat.h>

    #include <fstream>
    #include <iostream>
    #include <sstream>
#else
    #include <wincrypt.h>

    #include <sstream>

    #pragma comment(lib, "crypt32.lib")
    #pragma comment(lib, "cryptui.lib")
#endif

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

static string read_system_root_certs();

// Simple RAII helper for mbedTLS cert struct
struct mbedtls_context::cert : public mbedtls_x509_crt
{
    cert() { mbedtls_x509_crt_init(this); }
    ~cert() { mbedtls_x509_crt_free(this); }
};

// Simple RAII helper for mbedTLS cert struct
struct mbedtls_context::key : public mbedtls_pk_context
{
    key() { mbedtls_pk_init(this); }
    ~key() { mbedtls_pk_free(this); }
};

/////////////////////////////////////////////////////////////////////////////

mbedtls_context::cert* mbedtls_context::s_system_root_certs;

/*
tls_context& tls_context::default_context()
{
    static mbedtls_context dflt_ctx;
    return dflt_ctx;
}
*/

// Returns a shared mbedTLS random-number generator context.
static mbedtls_ctr_drbg_context* get_drbg_context() {
    static const char* k_entropy_personalization = "sockpp";
    static mbedtls_entropy_context s_entropy;
    static mbedtls_ctr_drbg_context s_random_ctx;

    static once_flag once;
    call_once(once, []() {
        mbedtls_entropy_init(&s_entropy);

#if defined(_MSC_VER)
    #if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        auto uwp_entropy_poll = [](void* data, unsigned char* output, size_t len,
                                   size_t* olen) -> int {
            NTSTATUS status =
                BCryptGenRandom(nullptr, output, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
            if (status < 0) {
                return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
            }

            *olen = len;
            return 0;
        };
        mbedtls_entropy_add_source(
            &s_entropy, uwp_entropy_poll, nullptr, 32, MBEDTLS_ENTROPY_SOURCE_STRONG
        );
    #endif
#endif

        mbedtls_ctr_drbg_init(&s_random_ctx);
        int ret = mbedtls_ctr_drbg_seed(
            &s_random_ctx, mbedtls_entropy_func, &s_entropy,
            (const uint8_t*)k_entropy_personalization, strlen(k_entropy_personalization)
        );
        if (ret != 0) {
            // FIXME: Not an errno; use different exception?
            throw std::system_error{result<>::last_error()};
        }
    });
    return &s_random_ctx;
}

unique_ptr<mbedtls_context::cert> mbedtls_context::parse_cert(
    const string& cert_data, bool partialOk
) {
    unique_ptr<cert> c(new cert);
    mbedtls_x509_crt_init(c.get());
    int ret = mbedtls_x509_crt_parse(
        c.get(), (const uint8_t*)cert_data.data(), cert_data.size() + 1
    );
    if (ret != 0) {
        if (ret < 0 || !partialOk) {
            if (ret > 0) {
                ret = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
            }

            throw std::system_error{result<>::last_error()};
        }
    }
    return c;
}

void mbedtls_context::set_root_certs(const string& cert_data) {
    root_certs_ = parse_cert(cert_data, true);
    mbedtls_ssl_conf_ca_chain(ssl_config_.get(), root_certs_.get(), nullptr);
}

// Returns the set of system trusted root CA certs.
mbedtls_x509_crt* mbedtls_context::get_system_root_certs() {
    static once_flag once;
    call_once(once, []() {
        // One-time initialization:
        string certsPEM = read_system_root_certs();
        if (!certsPEM.empty())
            s_system_root_certs = parse_cert(certsPEM, true).release();
    });
    return s_system_root_certs;
}

mbedtls_context::mbedtls_context(role_t r /*=CLIENT*/) : ssl_config_(new mbedtls_ssl_config) {
    mbedtls_ssl_config_init(ssl_config_.get());
    mbedtls_ssl_conf_rng(ssl_config_.get(), mbedtls_ctr_drbg_random, get_drbg_context());
    int endpoint = (r == CLIENT) ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER;
    set_status(mbedtls_ssl_config_defaults(
        ssl_config_.get(), endpoint, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT
    ));
    if (status() != 0)
        return;

    auto roots = get_system_root_certs();
    if (roots)
        mbedtls_ssl_conf_ca_chain(ssl_config_.get(), roots, nullptr);

    // Install a custom verification callback that will call my verify_callback():
    mbedtls_ssl_conf_verify(
        ssl_config_.get(),
        [](void* ctx, mbedtls_x509_crt* crt, int depth, uint32_t* flags) {
            return ((mbedtls_context*)ctx)->verify_callback(crt, depth, flags);
        },
        this
    );
}

mbedtls_context::~mbedtls_context() { mbedtls_ssl_config_free(ssl_config_.get()); }

int mbedtls_context::trusted_cert_callback(
    void* /*context*/, mbedtls_x509_crt const* child, mbedtls_x509_crt** candidates
) {
    if (!root_cert_locator_cb_)
        return -1;
    string certData((const char*)child->raw.p, child->raw.len);
    string rootData;
    if (!root_cert_locator_cb_(certData, rootData))
        return -1;  // TEMP
    if (rootData.empty()) {
        *candidates = nullptr;
    }
    else {
        // (can't use parse_cert() here because its return value uses RAII and will free
        // itself)
        auto root = (mbedtls_x509_crt*)malloc(sizeof(mbedtls_x509_crt));
        mbedtls_x509_crt_init(root);
        int err = mbedtls_x509_crt_parse(
            root, (const uint8_t*)rootData.data(), rootData.size() + 1
        );
        if (err != 0) {
            mbedtls_x509_crt_free(root);
            free(root);
            return err;
        }
        *candidates = root;
    }
    return 0;
}

void mbedtls_context::set_root_cert_locator(root_cert_locator_cb loc) {
    root_cert_locator_cb_ = loc;
    mbedtls_x509_crt_ca_cb_t callback = nullptr;
    if (loc) {
        callback = [](void* ctx, mbedtls_x509_crt const* child, mbedtls_x509_crt** cand) {
            return ((mbedtls_context*)ctx)->trusted_cert_callback(ctx, child, cand);
        };
        mbedtls_ssl_conf_ca_cb(ssl_config_.get(), callback, this);
    }
    else {
        // Resetting this automatically clears the callback
        auto roots = get_system_root_certs();
        if (roots)
            mbedtls_ssl_conf_ca_chain(ssl_config_.get(), roots, nullptr);
    }
}

void mbedtls_context::require_peer_cert(role_t forRole, bool require, bool sendCAList) {
    if (forRole != role())
        return;
    int authMode = (require ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_authmode(ssl_config_.get(), authMode);

    if (role() == SERVER) {
        mbedtls_ssl_conf_cert_req_ca_list(
            ssl_config_.get(), sendCAList ? MBEDTLS_SSL_CERT_REQ_CA_LIST_ENABLED
                                          : MBEDTLS_SSL_CERT_REQ_CA_LIST_DISABLED
        );
    }
}

void mbedtls_context::allow_only_certificate(const string& cert_data) {
    if (cert_data.empty()) {
        pinned_cert_.reset();
    }
    else {
        pinned_cert_ = parse_cert(cert_data, false);
    }
}

void mbedtls_context::allow_only_certificate(mbedtls_x509_crt* certificate) {
    string cert_data;
    if (certificate) {
        cert_data = string((const char*)certificate->raw.p, certificate->raw.len);
    }
    allow_only_certificate(cert_data);
}

// Callback from mbedTLS cert validation (see above)
//
// When a pinned cert is specified, the verify_callback will compare the pinned cert with
// each cert in the chain. If the pinned cert matches one of the certs in the chain, the
// presented cert (leaf cert) is trusted.
//
// The verify_callback is called for each cert in the chain from root to leaf cert. The
// pinned_cert_validation_result_ will store the previous comparison result. If the
// comparison result is already matched, the comparison will be skipped.
//
// The last callback for the leaf cert is where the comparison or verification is set
// to the status flags. The flags of the parent certs are ignored (clear).
//
int mbedtls_context::verify_callback(mbedtls_x509_crt* crt, int depth, uint32_t* flags) {
    if (pinned_cert_ && !pinned_cert_validation_result_) {
        pinned_cert_validation_result_ =
            (crt->raw.len == pinned_cert_->raw.len &&
             0 == memcmp(crt->raw.p, pinned_cert_->raw.p, crt->raw.len));
    }

    if (depth == 0) {  // leaf cert
        received_cert_data_ = string((const char*)crt->raw.p, crt->raw.len);

        int status = -1;
        if (pinned_cert_) {
            status = pinned_cert_validation_result_;
        }
        else if (auto& callback = get_auth_callback(); callback) {
            string certData((const char*)crt->raw.p, crt->raw.len);
            status = callback(certData);
        }

        if (status > 0) {
            *flags &= ~(MBEDTLS_X509_BADCERT_NOT_TRUSTED | MBEDTLS_X509_BADCERT_CN_MISMATCH);
        }
        else if (status == 0) {
            *flags |= MBEDTLS_X509_BADCERT_OTHER;
        }
    }
    else {
        if (pinned_cert_) {
            // We only care the result when last callback is called, clear all other errors
            *flags = 0;
        }
    }
    return 0;
}

void mbedtls_context::set_identity(
    const string& certificate_data, const string& private_key_data
) {
    auto ident_cert = parse_cert(certificate_data, false);

    unique_ptr<key> ident_key(new key);
    int err = mbedtls_pk_parse_key(
        ident_key.get(), reinterpret_cast<const unsigned char*>(private_key_data.data()),
        private_key_data.size(),
        // TODO: Is this right?
        nullptr, 0, nullptr, 0
    );
    if (err != 0) {
        throw std::system_error{result<>::last_error()};
    }

    set_identity(ident_cert.get(), ident_key.get());
    identity_cert_ = move(ident_cert);
    identity_key_ = move(ident_key);
}

void mbedtls_context::set_identity(
    mbedtls_x509_crt* certificate, mbedtls_pk_context* private_key
) {
    mbedtls_ssl_conf_own_cert(ssl_config_.get(), certificate, private_key);
}

mbedtls_context::role_t mbedtls_context::role() {
    return (ssl_config_->private_endpoint == MBEDTLS_SSL_IS_CLIENT) ? CLIENT : SERVER;
}

unique_ptr<tls_socket> mbedtls_context::wrap_socket(
    stream_socket&& sock, role_t /*r*/, const string& peer_name
) {
    // TODO: Verify supported role at runtime?
    // assert(role == role());
    return make_unique<mbedtls_socket>(std::move(sock), *this, peer_name);
}

// ----- platform specific

// mbedTLS does not have built-in support for reading the OS's trusted root certs.

#ifdef __APPLE__

// Read system root CA certs on macOS.
// (Sadly, SecTrustCopyAnchorCertificates() is not available on iOS)
static string read_system_root_certs() {
    #if TARGET_OS_OSX
    CFArrayRef roots;
    OSStatus err = SecTrustCopyAnchorCertificates(&roots);
    if (err)
        return string{};

    CFDataRef pemData = nullptr;
    err = SecItemExport(roots, kSecFormatPEMSequence, kSecItemPemArmour, nullptr, &pemData);
    CFRelease(roots);
    if (err)
        return string{};

    string pem((const char*)CFDataGetBytePtr(pemData), CFDataGetLength(pemData));
    CFRelease(pemData);
    return pem;
    #else
    // fallback -- no certs
    return string{};
    #endif
}

#elif defined(_WIN32)

// Windows:
static string read_system_root_certs() {
    PCCERT_CONTEXT pContext = nullptr;
    HCERTSTORE hStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_A, 0, nullptr, CERT_SYSTEM_STORE_CURRENT_USER, "ROOT"
    );
    if (hStore == nullptr) {
        return "";
    }

    stringstream certs;
    while ((pContext = CertEnumCertificatesInStore(hStore, pContext))) {
        DWORD pCertPEMSize = 0;
        if (!CryptBinaryToStringA(
                pContext->pbCertEncoded, pContext->cbCertEncoded, CRYPT_STRING_BASE64HEADER,
                nullptr, &pCertPEMSize
            )) {
            return "";
        }
        LPSTR pCertPEM = (LPSTR)malloc(pCertPEMSize);
        if (!CryptBinaryToStringA(
                pContext->pbCertEncoded, pContext->cbCertEncoded, CRYPT_STRING_BASE64HEADER,
                pCertPEM, &pCertPEMSize
            )) {
            return "";
        }
        certs.write(pCertPEM, pCertPEMSize);
        free(pCertPEM);
    }

    CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    return certs.str();
}

#else

// Read system root CA certs on Linux using OpenSSL's cert directory
static string read_system_root_certs() {
    #ifdef __ANDROID__
    static constexpr const char* CERTS_DIR = "/system/etc/security/cacerts/";
    #else
    static constexpr const char* CERTS_DIR = "/etc/ssl/certs/";
    static constexpr const char* CERTS_FILE = "ca-certificates.crt";
    #endif

    stringstream certs;
    char buf[1024];
    // Subroutine to append a file to the `certs` stream:
    auto read_file = [&](const string& file) {
        ifstream in(file);
        char last_char = '\n';
        while (in) {
            in.read(buf, sizeof(buf));
            auto n = in.gcount();
            if (n > 0) {
                certs.write(buf, n);
                last_char = buf[n - 1];
            }
        }
        if (last_char != '\n')
            certs << '\n';
    };

    struct stat s;
    if (stat(CERTS_DIR, &s) == 0 && S_ISDIR(s.st_mode)) {
    #ifndef __ANDROID__
        string certs_file = string(CERTS_DIR) + CERTS_FILE;
        if (stat(certs_file.c_str(), &s) == 0) {
            // If there is a file containing all the certs, just read it:
            read_file(certs_file);
        }
        else
    #endif
        {
            // Otherwise concatenate all the certs found in the dir:
            auto dir = opendir(CERTS_DIR);
            if (dir) {
                struct dirent* ent;
                while (nullptr != (ent = readdir(dir))) {
    #ifndef __ANDROID__
                    if (fnmatch("?*.pem", ent->d_name, FNM_PERIOD) == 0 ||
                        fnmatch("?*.crt", ent->d_name, FNM_PERIOD) == 0)
    #endif
                        read_file(string(CERTS_DIR) + ent->d_name);
                }
                closedir(dir);
            }
        }
    }
    return certs.str();
}

#endif

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}  // namespace sockpp
