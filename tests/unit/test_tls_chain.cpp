// test_tls_chain.cpp
//
// Unit and integration tests for §2: certificate chain support.
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

#include <future>
#include <string>
#include <vector>

#include "catch2_version.h"
#include "sockpp/inet_address.h"
#include "sockpp/tls/acceptor.h"
#include "sockpp/tls/certificate.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"

using namespace std;
using namespace sockpp;

// Self-signed ECDSA-P256 certificate for localhost / 127.0.0.1.
// Shared with test_tls_acceptor.cpp tests.
static const string TEST_CERT =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBmDCCAT+gAwIBAgIUNj3qqk1eir8KZ6acscAGphKwCBMwCgYIKoZIzj0EAwIw\n"
    "FDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI2MDUwOTE0NTIyNFoXDTM2MDUwNjE0\n"
    "NTIyNFowFDESMBAGA1UEAwwJbG9jYWxob3N0MFkwEwYHKoZIzj0CAQYIKoZIzj0D\n"
    "AQcDQgAEF13kmqjn6Kw6q7cwDW3tujz/5R5aJniJb88UEuWfZNDG8XSxu1iaAx6U\n"
    "oG1HyyHGa2H6RIgV7/gDm6mrkzFpyKNvMG0wHQYDVR0OBBYEFIsmifMvOCfErGG2\n"
    "QTWCGrqqxEPPMB8GA1UdIwQYMBaAFIsmifMvOCfErGG2QTWCGrqqxEPPMA8GA1Ud\n"
    "EwEB/wQFMAMBAf8wGgYDVR0RBBMwEYcEfwAAAYIJbG9jYWxob3N0MAoGCCqGSM49\n"
    "BAMCA0cAMEQCIG/jEqTzCSAvDbK5VtDRgx6PkzmO+w3xcvdjZ94WegNuAiAgCP2p\n"
    "NYDQjecjml2ijvX/68Nt6RLyJsMlVDS+deGafQ==\n"
    "-----END CERTIFICATE-----\n";

static const string TEST_KEY =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQga2k/03faVeWICjom\n"
    "xy1v3HRbYtKrOELa+ugdZFKqRquhRANCAAQXXeSaqOforDqrtzANbe26PP/lHlom\n"
    "eIlvzxQS5Z9k0MbxdLG7WJoDHpSgbUfLIcZrYfpEiBXv+AObqauTMWnI\n"
    "-----END PRIVATE KEY-----\n";

// ===========================================================================
// chain_from_pem tests
// ===========================================================================

TEST_CASE("chain_from_pem with one certificate", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem(TEST_CERT);
    REQUIRE(res);
    REQUIRE(res.value().size() == 1);
    REQUIRE(res.value()[0].is_valid());
}

TEST_CASE("chain_from_pem with two concatenated certificates", "[tls_certificate][chain]") {
    // Two copies of the same cert — just to test multi-cert parsing.
    string bundle = TEST_CERT + TEST_CERT;
    auto res = tls_certificate::chain_from_pem(bundle);
    REQUIRE(res);
    REQUIRE(res.value().size() == 2);
}

TEST_CASE("chain_from_pem with empty string returns error", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem("");
    REQUIRE(!res);
}

TEST_CASE("chain_from_pem with junk returns error", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem("not a certificate");
    REQUIRE(!res);
}

TEST_CASE("chain_from_pem round-trips through to_pem", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem(TEST_CERT);
    REQUIRE(res);
    const auto& chain = res.value();

    string reconstructed = to_pem(chain);
    REQUIRE(!reconstructed.empty());

    // Re-parsing the reconstructed PEM should yield the same number of certs.
    auto res2 = tls_certificate::chain_from_pem(reconstructed);
    REQUIRE(res2);
    REQUIRE(res2.value().size() == chain.size());
}

TEST_CASE("chain_from_pem produces valid certificates", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem(TEST_CERT);
    REQUIRE(res);
    for (const auto& cert : res.value()) {
        REQUIRE(cert.is_valid());
        REQUIRE(!cert.subject_name().empty());
    }
}

// ===========================================================================
// tls_certificate_chain alias and to_pem() free function
// ===========================================================================

TEST_CASE(
    "tls_certificate_chain is a vector of tls_certificate", "[tls_certificate][chain]"
) {
    tls_certificate_chain chain;
    auto res = tls_certificate::from_pem(TEST_CERT);
    REQUIRE(res);
    chain.push_back(res.release());
    REQUIRE(chain.size() == 1);
}

TEST_CASE("to_pem of empty chain is empty string", "[tls_certificate][chain]") {
    tls_certificate_chain chain;
    REQUIRE(to_pem(chain).empty());
}

TEST_CASE("to_pem of one-cert chain is non-empty PEM", "[tls_certificate][chain]") {
    auto res = tls_certificate::chain_from_pem(TEST_CERT);
    REQUIRE(res);
    string pem = to_pem(res.value());
    REQUIRE(pem.size() >= 5);
    REQUIRE(pem.substr(0, 5) == "-----");
}

// ===========================================================================
// set_identity(chain, key) on the context
// ===========================================================================

TEST_CASE("set_identity with chain loads identity without error", "[tls_context][chain]") {
    auto chain_res = tls_certificate::chain_from_pem(TEST_CERT);
    REQUIRE(chain_res);

    auto ctx = tls_context::server();
    auto res = ctx.set_identity(chain_res.value(), TEST_KEY);
    REQUIRE(res);
}

// ===========================================================================
// peer_certificate_chain integration test
// ===========================================================================

TEST_CASE(
    "peer_certificate_chain returns at least the leaf after handshake",
    "[tls_context][chain][integration]"
) {
    auto srv_ctx = tls_context::server();
    REQUIRE(srv_ctx.set_identity(TEST_CERT, TEST_KEY));

    auto cli_ctx = tls_context::client();
    cli_ctx.set_root_certs(TEST_CERT);

    error_code ec;
    tls_acceptor acc{srv_ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    in_port_t port = inet_address(acc.address()).port();

    auto srv_fut = async(launch::async, [&acc] {
        auto res = acc.accept();
        return static_cast<bool>(res);
    });

    tls_connector conn{cli_ctx, inet_address{"127.0.0.1", port, ec}, "localhost", ec};
    REQUIRE(!ec);

    auto chain = conn.peer_certificate_chain();
    REQUIRE(!chain.empty());
    REQUIRE(chain[0].is_valid());
    // The leaf should be the test cert.
    REQUIRE(chain[0].subject_name().find("localhost") != string::npos);

    conn.close();
    REQUIRE(srv_fut.get());
}
