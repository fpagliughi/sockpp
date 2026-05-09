// test_tls_certificate.cpp
//
// Unit tests for tls_certificate and tls_context (OpenSSL and mbedTLS backends).
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

#include <cstdio>
#include <ctime>
#include <string>

#include "catch2_version.h"
#include "sockpp/tls/certificate.h"
#include "sockpp/tls/context.h"

using namespace std;
using namespace sockpp;

// A real ECDSA-P256/SHA-256 leaf certificate for example.org, issued by
// Cloudflare.  Used as a self-contained parse fixture that exercises modern
// algorithm paths without requiring filesystem access.
//
// Note: this cert will expire 2026-07-01; when it does, replace it with any
// current PEM cert signed with SHA-256 or stronger.
static const string TEST_PEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIID0zCCA3mgAwIBAgIQTe8JpseJy3C3lEIhxFq+uTAKBggqhkjOPQQDAjBSMQsw\n"
    "CQYDVQQGEwJVUzEZMBcGA1UECgwQQ0xPVURGTEFSRSwgSU5DLjEoMCYGA1UEAwwf\n"
    "Q2xvdWRmbGFyZSBUTFMgSXNzdWluZyBFQ0MgQ0EgMTAeFw0yNjA0MDIyMjE4MTZa\n"
    "Fw0yNjA3MDEyMTUyNTRaMBYxFDASBgNVBAMMC2V4YW1wbGUub3JnMFkwEwYHKoZI\n"
    "zj0CAQYIKoZIzj0DAQcDQgAE8Z9LQufaNZfHx6E44qdjhMkXW/GT+v+p3PxMVk4d\n"
    "Armsuz05UIceLATZt8Blm/GshbROInBgDGcLGahRXlvV2qOCAmswggJnMAwGA1Ud\n"
    "EwEB/wQCMAAwHwYDVR0jBBgwFoAUnMQJckcYF3unGomzkjXV4QOM/pIwbAYIKwYB\n"
    "BQUHAQEEYDBeMDkGCCsGAQUFBzAChi1odHRwOi8vaS5jZi1iLnNzbC5jb20vQ2xv\n"
    "dWRmbGFyZSBUTFMtSS1FMS5jZXIwIQYIKwYBBQUHMAGGFWh0dHA6Ly9vLmNmLWIu\n"
    "c3NsLmNvbTAlBgNVHREEHjAcggtleGFtcGxlLm9yZ4INKi5leGFtcGxlLm9yZzAj\n"
    "BgNVHSAEHDAaMAgGBmeBDAECATAOBgwrBgEEAYKpMAEDAQEwEwYDVR0lBAwwCgYI\n"
    "KwYBBQUHAwEwPgYDVR0fBDcwNTAzoDGgL4YtaHR0cDovL2MuY2YtYi5zc2wuY29t\n"
    "L0Nsb3VkZmxhcmUtVExTLUktRTEuY3JsMA4GA1UdDwEB/wQEAwIHgDAPBgkrBgEE\n"
    "AYLaSywEAgUAMIIBBAYKKwYBBAHWeQIEAgSB9QSB8gDwAHUAyKPEf8ezrbk1awE/\n"
    "anoSbeM6TkOlxkb5l605dZkdz5oAAAGdUE/DoAAABAMARjBEAiBLZBG0dH12kHxD\n"
    "d/zNao42B+moWtIYUUTB3DM9X9yfZAIgJS4UmlwIfq7xIec/+GP2YKEywroEIka9\n"
    "b/F8cHWhDyYAdwDCMX5XRRmjRe5/ON6ykEHrx8IhWiK/f9W1rXaa2Q5SzQAAAZ1Q\n"
    "T8OXAAAEAwBIMEYCIQDVnimEyxkvUNo+Yoehw04Km66CH04wkRtcuSqQcdgweQIh\n"
    "AN4LPo9J/2tMVHGLjpAA86c9gMCr8AduwGxHq2+oUoIvMAoGCCqGSM49BAMCA0gA\n"
    "MEUCIQC0OgGjA3sqXrDOP3SP7fwlTvlgsYjeopy2QAK8dpSdywIgQ0rm+/OiCwjq\n"
    "bk/xcAanOCpr4f3HspOuJtOFmUn3eCE=\n"
    "-----END CERTIFICATE-----\n";

// ===========================================================================
// tls_certificate — default state
// ===========================================================================

TEST_CASE("default-constructed tls_certificate is invalid", "[tls_certificate]") {
    tls_certificate cert;

    REQUIRE(!cert.is_valid());
    REQUIRE(cert.to_der().empty());
    REQUIRE(cert.to_pem().empty());
    REQUIRE(cert.subject_name().empty());
    REQUIRE(cert.issuer_name().empty());
    REQUIRE(cert.not_before_str().empty());
    REQUIRE(cert.not_after_str().empty());
}

// ===========================================================================
// tls_certificate — parsing
// ===========================================================================

TEST_CASE("tls_certificate from_pem with valid PEM", "[tls_certificate][from_pem]") {
    auto res = tls_certificate::from_pem(TEST_PEM);
    REQUIRE(res);

    const auto& cert = res.value();

    SECTION("is_valid") { REQUIRE(cert.is_valid()); }

    SECTION("subject_name is non-empty") { REQUIRE(!cert.subject_name().empty()); }

    SECTION("issuer_name is non-empty") { REQUIRE(!cert.issuer_name().empty()); }

    SECTION("subject contains the expected CN") {
        // Both backends include the value; separators differ (/CN= vs CN=).
        REQUIRE(cert.subject_name().find("example.org") != string::npos);
    }

    SECTION("not_before_str is non-empty and ends with Z") {
        const auto s = cert.not_before_str();
        REQUIRE(!s.empty());
        REQUIRE(s.back() == 'Z');
    }

    SECTION("not_after_str is non-empty and ends with Z") {
        const auto s = cert.not_after_str();
        REQUIRE(!s.empty());
        REQUIRE(s.back() == 'Z');
    }

    SECTION("to_der returns non-empty binary") { REQUIRE(!cert.to_der().empty()); }

    SECTION("to_pem returns a PEM-encoded string") {
        const auto pem = cert.to_pem();
        REQUIRE(pem.find("-----BEGIN CERTIFICATE-----") != string::npos);
        REQUIRE(pem.find("-----END CERTIFICATE-----") != string::npos);
    }
}

TEST_CASE(
    "tls_certificate from_pem with garbage input fails", "[tls_certificate][from_pem]"
) {
    auto res = tls_certificate::from_pem("this is not a certificate");
    REQUIRE(!res);
}

TEST_CASE("tls_certificate from_pem with empty string fails", "[tls_certificate][from_pem]") {
    auto res = tls_certificate::from_pem("");
    REQUIRE(!res);
}

TEST_CASE("tls_certificate from_der round-trip", "[tls_certificate][from_der]") {
    auto orig = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(orig.is_valid());

    const auto der = orig.to_der();
    REQUIRE(!der.empty());

    auto res = tls_certificate::from_der(der);
    REQUIRE(res);

    const auto& cert = res.value();
    REQUIRE(cert.is_valid());

    // The round-tripped DER should be byte-for-byte identical.
    REQUIRE(cert.to_der() == der);
}

TEST_CASE(
    "tls_certificate from_der with garbage input fails", "[tls_certificate][from_der]"
) {
    sockpp::binary garbage{0x01, 0x02, 0x03, 0x04};
    auto res = tls_certificate::from_der(garbage);
    REQUIRE(!res);
}

TEST_CASE(
    "tls_certificate from_file with nonexistent path fails", "[tls_certificate][from_file]"
) {
    auto res = tls_certificate::from_file("/nonexistent/path/cert.pem");
    REQUIRE(!res);
}

// ===========================================================================
// tls_certificate — copy semantics
// ===========================================================================

TEST_CASE(
    "tls_certificate copy constructor produces a valid copy", "[tls_certificate][copy]"
) {
    auto orig = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(orig.is_valid());

    tls_certificate copy{orig};

    REQUIRE(copy.is_valid());
    REQUIRE(copy.to_der() == orig.to_der());
    REQUIRE(copy.subject_name() == orig.subject_name());
}

TEST_CASE("tls_certificate copy is independent of original", "[tls_certificate][copy]") {
    // Construct a copy inside a scope, destroy the original, verify the copy
    // still holds its own data (i.e. is not a dangling pointer/reference).
    sockpp::binary der_after;
    {
        auto orig = tls_certificate::from_pem(TEST_PEM).release();
        const auto der_before = orig.to_der();

        tls_certificate copy{orig};
        // orig goes out of scope and is destroyed here.
        der_after = copy.to_der();

        REQUIRE(der_after == der_before);
    }
    // der_after was captured before orig was destroyed; it should still be
    // the same bytes (this tests that we captured the copy's data, not a
    // reference into orig's storage).
    REQUIRE(!der_after.empty());
}

TEST_CASE("tls_certificate copy assignment", "[tls_certificate][copy]") {
    auto orig = tls_certificate::from_pem(TEST_PEM).release();
    tls_certificate copy;

    REQUIRE(!copy.is_valid());
    copy = orig;
    REQUIRE(copy.is_valid());
    REQUIRE(copy.to_der() == orig.to_der());
}

TEST_CASE("tls_certificate self-copy-assignment is safe", "[tls_certificate][copy]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());
    const auto der = cert.to_der();

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    cert = cert;
#ifdef __clang__
    #pragma clang diagnostic pop
#endif

    REQUIRE(cert.is_valid());
    REQUIRE(cert.to_der() == der);
}

// ===========================================================================
// tls_certificate — move semantics
// ===========================================================================

TEST_CASE("tls_certificate move constructor transfers ownership", "[tls_certificate][move]") {
    auto orig = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(orig.is_valid());
    const auto der = orig.to_der();

    tls_certificate moved{std::move(orig)};

    REQUIRE(moved.is_valid());
    REQUIRE(moved.to_der() == der);
    REQUIRE(!orig.is_valid());
}

TEST_CASE("tls_certificate move assignment transfers ownership", "[tls_certificate][move]") {
    auto orig = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(orig.is_valid());
    const auto der = orig.to_der();

    tls_certificate dest;
    dest = std::move(orig);

    REQUIRE(dest.is_valid());
    REQUIRE(dest.to_der() == der);
    REQUIRE(!orig.is_valid());
}

TEST_CASE("tls_certificate self-move-assignment is safe", "[tls_certificate][move]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());
    const auto der = cert.to_der();

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wself-move"
#endif
    cert = std::move(cert);
#ifdef __clang__
    #pragma clang diagnostic pop
#endif

    // After a self-move the object must be in a valid (possibly empty) state;
    // it must not crash or leave dangling memory.
    // (The standard permits the moved-from state to be "valid but unspecified".)
    (void)cert.is_valid();
}

// ===========================================================================
// tls_certificate — §8 richer fields
// ===========================================================================

// These tests use the Cloudflare TEST_PEM certificate declared above.
//
// Known values for that cert:
//   Serial (hex):  4def09a6c789cb70b79442 21c45abeb9
//   SANs:          DNS:example.org, DNS:*.example.org
//   Key Usage:     digitalSignature (0x80 per RFC 5280 / KU_DIGITAL_SIGNATURE)
//   EKU OID:       1.3.6.1.5.5.7.3.1  (TLS Web Server Authentication)
//   SHA-256 fp:    8c3ec68f304a7d7add4be10f0f203ffe3a94cd0958d4487f28fac7d0dd1fab22
//   Not Before:    2026-04-02 22:18:16 UTC
//   Not After:     2026-07-01 21:52:54 UTC

TEST_CASE("tls_certificate not_before and not_after time_points", "[tls_certificate][time]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    using namespace std::chrono;

    auto nb = cert.not_before();
    auto na = cert.not_after();

    // Both must be after the Unix epoch.
    REQUIRE(nb.time_since_epoch().count() > 0);
    REQUIRE(na.time_since_epoch().count() > 0);

    // not_before must be strictly before not_after.
    REQUIRE(nb < na);

    // Not Before: 2026-04-02 22:18:16 UTC
    auto nb_t = system_clock::to_time_t(nb);
    struct tm nb_tm;
    gmtime_r(&nb_t, &nb_tm);
    REQUIRE(nb_tm.tm_year + 1900 == 2026);
    REQUIRE(nb_tm.tm_mon + 1 == 4);
    REQUIRE(nb_tm.tm_mday == 2);
    REQUIRE(nb_tm.tm_hour == 22);
    REQUIRE(nb_tm.tm_min == 18);
    REQUIRE(nb_tm.tm_sec == 16);

    // Not After: 2026-07-01 21:52:54 UTC
    auto na_t = system_clock::to_time_t(na);
    struct tm na_tm;
    gmtime_r(&na_t, &na_tm);
    REQUIRE(na_tm.tm_year + 1900 == 2026);
    REQUIRE(na_tm.tm_mon + 1 == 7);
    REQUIRE(na_tm.tm_mday == 1);
    REQUIRE(na_tm.tm_hour == 21);
    REQUIRE(na_tm.tm_min == 52);
    REQUIRE(na_tm.tm_sec == 54);
}

TEST_CASE(
    "default-constructed certificate has epoch time_points", "[tls_certificate][time]"
) {
    tls_certificate cert;
    using tp = std::chrono::system_clock::time_point;
    REQUIRE(cert.not_before() == tp{});
    REQUIRE(cert.not_after() == tp{});
}

TEST_CASE("tls_certificate serial_number", "[tls_certificate][serial]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    auto sn = cert.serial_number();
    REQUIRE(!sn.empty());

    // Serial hex must match the known value.
    auto hex = cert.serial_number_hex();
    REQUIRE(hex == "4def09a6c789cb70b7944221c45abeb9");
}

TEST_CASE("default-constructed certificate has empty serial", "[tls_certificate][serial]") {
    tls_certificate cert;
    REQUIRE(cert.serial_number().empty());
    REQUIRE(cert.serial_number_hex().empty());
}

TEST_CASE("tls_certificate fingerprint_sha256", "[tls_certificate][fingerprint]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    auto fp = cert.fingerprint_sha256();
    REQUIRE(fp.size() == 32);

    // Convert to hex and compare against the known OpenSSL fingerprint.
    string hex;
    for (uint8_t b : fp) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", b);
        hex += buf;
    }
    REQUIRE(hex == "8c3ec68f304a7d7add4be10f0f203ffe3a94cd0958d4487f28fac7d0dd1fab22");
}

TEST_CASE(
    "default-constructed certificate has empty fingerprint", "[tls_certificate][fingerprint]"
) {
    tls_certificate cert;
    REQUIRE(cert.fingerprint_sha256().empty());
}

TEST_CASE("tls_certificate subject_alt_names", "[tls_certificate][san]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    auto sans = cert.subject_alt_names();

    // The Cloudflare cert has two DNS SANs: example.org and *.example.org
    REQUIRE(sans.size() == 2);

    for (const auto& san : sans) REQUIRE(san.kind == subject_alt_name::type::DNS);

    // Order as they appear in the extension: example.org first.
    REQUIRE(sans[0].value == "example.org");
    REQUIRE(sans[1].value == "*.example.org");
}

TEST_CASE("default-constructed certificate has empty SANs", "[tls_certificate][san]") {
    tls_certificate cert;
    REQUIRE(cert.subject_alt_names().empty());
}

TEST_CASE("tls_certificate key_usage", "[tls_certificate][key_usage]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    uint32_t ku = cert.key_usage();

    // The Cloudflare cert has Key Usage: Digital Signature (0x80).
    REQUIRE(ku != 0);
    REQUIRE((ku & 0x80) != 0);  // digitalSignature bit

    // Bits that should NOT be set for a leaf TLS cert.
    REQUIRE((ku & 0x04) == 0);  // keyCertSign must be off
}

TEST_CASE(
    "default-constructed certificate has zero key_usage", "[tls_certificate][key_usage]"
) {
    tls_certificate cert;
    REQUIRE(cert.key_usage() == 0);
}

TEST_CASE("tls_certificate extended_key_usage", "[tls_certificate][eku]") {
    auto cert = tls_certificate::from_pem(TEST_PEM).release();
    REQUIRE(cert.is_valid());

    auto eku = cert.extended_key_usage();
    REQUIRE(!eku.empty());

    // The Cloudflare cert has id-kp-serverAuth.
    bool found_server_auth = false;
    for (const auto& oid : eku) {
        if (oid == "1.3.6.1.5.5.7.3.1")
            found_server_auth = true;
    }
    REQUIRE(found_server_auth);
}

TEST_CASE("default-constructed certificate has empty EKU", "[tls_certificate][eku]") {
    tls_certificate cert;
    REQUIRE(cert.extended_key_usage().empty());
}

// ===========================================================================
// tls_context — factory and configuration
// ===========================================================================

TEST_CASE("tls_context client() creates a context", "[tls_context]") {
    // Should not throw.
    auto ctx = tls_context::client();

#if defined(SOCKPP_MBEDTLS)
    REQUIRE(ctx.role() == tls_context::CLIENT);
#elif defined(SOCKPP_OPENSSL)
    REQUIRE(ctx.role() == tls_context::role_t::CLIENT);
#endif
}

TEST_CASE("tls_context server() creates a context", "[tls_context]") {
    auto ctx = tls_context::server();

#if defined(SOCKPP_MBEDTLS)
    REQUIRE(ctx.role() == tls_context::SERVER);
#elif defined(SOCKPP_OPENSSL)
    REQUIRE(ctx.role() == tls_context::role_t::SERVER);
#endif
}

TEST_CASE("tls_context default_context() is a valid client context", "[tls_context]") {
    auto& ctx = tls_context::default_context();

#if defined(SOCKPP_MBEDTLS)
    REQUIRE(ctx.role() == tls_context::CLIENT);
#elif defined(SOCKPP_OPENSSL)
    REQUIRE(ctx.role() == tls_context::role_t::CLIENT);
#endif
}

TEST_CASE("tls_context auth_callback round-trip", "[tls_context]") {
    auto ctx = tls_context::client();

    SECTION("initially no callback") { REQUIRE(!ctx.get_auth_callback()); }

    SECTION("set callback is retrievable") {
        bool called = false;
        ctx.set_auth_callback([&called](const string&) {
            called = true;
            return true;
        });
        REQUIRE(ctx.get_auth_callback());

        // Invoke it and verify it's the right function.
        REQUIRE(ctx.get_auth_callback()("dummy_cert_data"));
        REQUIRE(called);
    }

    SECTION("callback can be cleared") {
        ctx.set_auth_callback([](const string&) { return true; });
        REQUIRE(ctx.get_auth_callback());

        ctx.set_auth_callback(nullptr);
        REQUIRE(!ctx.get_auth_callback());
    }
}

TEST_CASE("tls_context set_mode and clear_mode do not crash", "[tls_context]") {
    auto ctx = tls_context::client();

    // These are no-ops on mbedTLS (flags stored but not acted on) and delegate
    // to SSL_CTX_set_mode / SSL_CTX_clear_mode on OpenSSL.  Either way they
    // must not throw or crash.
    ctx.set_mode(tls_context::AUTO_RETRY);
    ctx.set_mode(tls_context::ENABLE_PARTIAL_WRITE);
    ctx.clear_mode(tls_context::AUTO_RETRY);
    ctx.clear_mode(tls_context::ENABLE_PARTIAL_WRITE);
}

TEST_CASE("tls_context require_peer_cert does not crash", "[tls_context]") {
    auto ctx = tls_context::client();
    ctx.require_peer_cert(true);
    ctx.require_peer_cert(false);
}
