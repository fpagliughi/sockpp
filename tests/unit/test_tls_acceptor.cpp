// test_tls_acceptor.cpp
//
// Integration tests for tls_acceptor: loopback server + client handshake.
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

#include <atomic>
#include <future>
#include <string>
#include <thread>

#include "catch2_version.h"
#include "sockpp/inet_address.h"
#include "sockpp/tls/acceptor.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"

using namespace std;
using namespace sockpp;

// Self-signed P-256/SHA-256 certificate for localhost / 127.0.0.1.
// Valid for 10 years from 2026-05-09.  Regenerate with:
//   openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 \
//     -keyout key.pem -out cert.pem -days 3650 -nodes \
//     -subj "/CN=localhost" \
//     -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
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
// Helpers
// ===========================================================================

/** Creates a server context loaded with the test certificate. */
static tls_context make_server_ctx() {
    auto ctx = tls_context::server();
    auto res = ctx.set_identity(TEST_CERT, TEST_KEY);
    REQUIRE(res);
    return ctx;
}

/** Creates a client context that trusts only the test certificate. */
static tls_context make_client_ctx() {
    auto ctx = tls_context::client();
    ctx.set_root_certs(TEST_CERT);
    return ctx;
}

// ===========================================================================
// Tests
// ===========================================================================

TEST_CASE("tls_acceptor constructs without binding", "[tls_acceptor]") {
    auto ctx = make_server_ctx();
    tls_acceptor acc{ctx};
    // Not bound yet; the object is valid but has no file descriptor.
    REQUIRE(!acc.is_open());
}

TEST_CASE("tls_acceptor open binds to port 0", "[tls_acceptor]") {
    auto ctx = make_server_ctx();
    tls_acceptor acc{ctx};

    auto ores = acc.open(inet_address(0), acceptor::DFLT_QUE_SIZE);
    REQUIRE(ores);
    REQUIRE(acc.is_open());

    // OS chose a non-zero ephemeral port.
    in_port_t port = inet_address(acc.address()).port();
    REQUIRE(port > 0);
}

TEST_CASE("tls_acceptor address constructor binds and listens", "[tls_acceptor]") {
    auto ctx = make_server_ctx();
    error_code ec;
    tls_acceptor acc{ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);
    REQUIRE(acc.is_open());
}

TEST_CASE("tls_acceptor loopback handshake", "[tls_acceptor][integration]") {
    auto srv_ctx = make_server_ctx();
    auto cli_ctx = make_client_ctx();

    // Bind to an ephemeral port.
    error_code ec;
    tls_acceptor acc{srv_ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    in_port_t port = inet_address(acc.address()).port();
    REQUIRE(port > 0);

    const string MESSAGE = "hello tls";

    // Server runs in a background thread: accept one connection, echo the
    // message back, then close.
    auto srv_fut = async(launch::async, [&acc, &MESSAGE] {
        auto res = acc.accept();
        if (!res)
            return false;
        auto sock = res.release();

        char buf[64] = {};
        auto rres = sock.read(buf, sizeof(buf));
        if (!rres || rres.value() == 0)
            return false;

        string got{buf, rres.value()};
        if (got != MESSAGE)
            return false;

        auto wres = sock.write(got);
        return static_cast<bool>(wres);
    });

    // Client: connect, send, receive, verify.
    tls_connector conn{cli_ctx, inet_address{"127.0.0.1", port, ec}, "localhost", ec};
    REQUIRE(!ec);

    auto wres = conn.write(MESSAGE);
    REQUIRE(wres);
    REQUIRE(wres.value() == MESSAGE.size());

    char buf[64] = {};
    auto rres = conn.read(buf, sizeof(buf));
    REQUIRE(rres);
    REQUIRE(string(buf, rres.value()) == MESSAGE);

    conn.close();

    bool srv_ok = srv_fut.get();
    REQUIRE(srv_ok);
}

TEST_CASE(
    "tls_acceptor accept timeout returns timed_out when no client connects", "[tls_acceptor]"
) {
    auto ctx = make_server_ctx();
    error_code ec;
    tls_acceptor acc{ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    using namespace std::chrono_literals;
    auto res = acc.accept(50ms);
    REQUIRE(!res);
    REQUIRE(res.error() == errc::timed_out);
}
