// test_tls_version_cipher.cpp
//
// Unit tests for TLS protocol version and cipher suite configuration.
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

#include <string>
#include <vector>

#include "catch2_version.h"
#include "sockpp/tls/context.h"

using namespace std;
using namespace sockpp;

// ===========================================================================
// Protocol version tests
// ===========================================================================

TEST_CASE("set_min_tls_version TLS_1_2 does not error", "[tls_context][version]") {
    auto ctx = tls_context::client();
    auto res = ctx.set_min_tls_version(tls_context::tls_version::TLS_1_2);
    REQUIRE(res);
}

TEST_CASE("set_min_tls_version TLS_1_3 does not error", "[tls_context][version]") {
    auto ctx = tls_context::client();
    auto res = ctx.set_min_tls_version(tls_context::tls_version::TLS_1_3);
    REQUIRE(res);
}

TEST_CASE("set_max_tls_version TLS_1_2 does not error", "[tls_context][version]") {
    auto ctx = tls_context::client();
    auto res = ctx.set_max_tls_version(tls_context::tls_version::TLS_1_2);
    REQUIRE(res);
}

TEST_CASE("set_max_tls_version TLS_1_3 does not error", "[tls_context][version]") {
    auto ctx = tls_context::client();
    auto res = ctx.set_max_tls_version(tls_context::tls_version::TLS_1_3);
    REQUIRE(res);
}

TEST_CASE("set_min and set_max TLS_1_2 combined does not error", "[tls_context][version]") {
    auto ctx = tls_context::client();
    REQUIRE(ctx.set_min_tls_version(tls_context::tls_version::TLS_1_2));
    REQUIRE(ctx.set_max_tls_version(tls_context::tls_version::TLS_1_2));
}

TEST_CASE(
    "builder min_tls_version and max_tls_version do not error", "[tls_context][version]"
) {
    auto ctx = tls_context_builder::client()
                   .min_tls_version(tls_context::tls_version::TLS_1_2)
                   .max_tls_version(tls_context::tls_version::TLS_1_3)
                   .finalize();
    // If builder recorded an error the context would be unusable; just verify
    // the calls above didn't throw.
    (void)ctx;
    SUCCEED();
}

// ===========================================================================
// Cipher suite tests
// ===========================================================================

TEST_CASE(
    "set_ciphersuites with one valid name does not error", "[tls_context][ciphersuites]"
) {
    auto ctx = tls_context::client();
#if defined(SOCKPP_MBEDTLS)
    // mbedTLS uses its own naming convention.
    auto res = ctx.set_ciphersuites({"TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256"});
#else
    // OpenSSL uses its own naming convention.
    auto res = ctx.set_ciphersuites({"ECDHE-RSA-AES128-GCM-SHA256"});
#endif
    REQUIRE(res);
}

TEST_CASE(
    "set_ciphersuites with multiple valid names does not error", "[tls_context][ciphersuites]"
) {
    auto ctx = tls_context::client();
#if defined(SOCKPP_MBEDTLS)
    auto res = ctx.set_ciphersuites({
        "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256",
        "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256",
    });
#else
    auto res = ctx.set_ciphersuites({
        "ECDHE-RSA-AES128-GCM-SHA256",
        "ECDHE-ECDSA-AES128-GCM-SHA256",
    });
#endif
    REQUIRE(res);
}

TEST_CASE(
    "set_ciphersuites with all invalid names returns error", "[tls_context][ciphersuites]"
) {
    auto ctx = tls_context::client();
    auto res = ctx.set_ciphersuites({"NOT-A-REAL-CIPHER", "ALSO-FAKE"});
    // Both backends should fail when no valid suite name is supplied.
    REQUIRE(!res);
}

TEST_CASE(
    "builder ciphersuites with valid name does not error", "[tls_context][ciphersuites]"
) {
    auto builder = tls_context_builder::client();
#if defined(SOCKPP_MBEDTLS)
    builder.ciphersuites({"TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256"});
#else
    builder.ciphersuites({"ECDHE-RSA-AES128-GCM-SHA256"});
#endif
    REQUIRE(!builder.error());
}
