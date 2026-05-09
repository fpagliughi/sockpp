// test_tls_psk.cpp
//
// Integration tests for TLS Pre-Shared Key (PSK) connections.
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

// A 32-byte PSK shared between client and server.
static const binary TEST_PSK{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
};

static const string TEST_PSK_IDENTITY = "sockpp-test";

// ===========================================================================
// Helpers
// ===========================================================================

/** Creates a server context configured for PSK-only (no certificate). */
static tls_context make_psk_server_ctx() {
    auto ctx = tls_context::server();
    auto res = ctx.set_psk_callback(
        [](const string& identity) -> binary {
            if (identity == TEST_PSK_IDENTITY)
                return TEST_PSK;
            return {};  // unknown identity → reject
        }
    );
    REQUIRE(res);
    return ctx;
}

/** Creates a client context configured with the shared PSK. */
static tls_context make_psk_client_ctx() {
    auto ctx = tls_context::client();
    // Disable certificate verification — PSK connections have no certificate.
    ctx.set_verify(tls_context::verify_t::NONE);
    auto res = ctx.set_psk(TEST_PSK_IDENTITY, TEST_PSK);
    REQUIRE(res);
    return ctx;
}

// ===========================================================================
// Tests
// ===========================================================================

TEST_CASE("tls_context set_psk does not error", "[tls_context][psk]") {
    auto ctx = tls_context::client();
    ctx.set_verify(tls_context::verify_t::NONE);
    auto res = ctx.set_psk(TEST_PSK_IDENTITY, TEST_PSK);
    REQUIRE(res);
}

TEST_CASE("tls_context set_psk_callback does not error", "[tls_context][psk]") {
    auto ctx = tls_context::server();
    auto res = ctx.set_psk_callback(
        [](const string&) -> binary { return TEST_PSK; }
    );
    REQUIRE(res);
}

TEST_CASE("tls_context set_psk_callback nullptr clears callback", "[tls_context][psk]") {
    auto ctx = tls_context::server();
    ctx.set_psk_callback([](const string&) -> binary { return TEST_PSK; });
    auto res = ctx.set_psk_callback(nullptr);
    REQUIRE(res);
}

TEST_CASE("PSK loopback handshake", "[tls_psk][integration]") {
    auto srv_ctx = make_psk_server_ctx();
    auto cli_ctx = make_psk_client_ctx();

    error_code ec;
    tls_acceptor acc{srv_ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    in_port_t port = inet_address(acc.address()).port();
    REQUIRE(port > 0);

    const string MESSAGE = "hello psk";

    // Server: accept one connection, echo message back.
    auto srv_fut = async(launch::async, [&acc, &MESSAGE] {
        auto res = acc.accept();
        if (!res)
            return false;
        auto sock = res.release();

        char buf[64] = {};
        auto rres = sock.read(buf, sizeof(buf));
        if (!rres || rres.value() == 0)
            return false;
        if (string{buf, rres.value()} != MESSAGE)
            return false;

        auto wres = sock.write(string{buf, rres.value()});
        return static_cast<bool>(wres);
    });

    // Client: connect, send, receive, verify.
    tls_connector conn{cli_ctx, inet_address{"127.0.0.1", port, ec}, "", ec};
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

TEST_CASE("PSK wrong key rejects connection", "[tls_psk][integration]") {
    // Server accepts only TEST_PSK; client sends a different key.
    auto srv_ctx = make_psk_server_ctx();

    auto cli_ctx = tls_context::client();
    cli_ctx.set_verify(tls_context::verify_t::NONE);
    binary wrong_key(32, uint8_t{0xff});
    cli_ctx.set_psk(TEST_PSK_IDENTITY, wrong_key);

    error_code ec;
    tls_acceptor acc{srv_ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    in_port_t port = inet_address(acc.address()).port();

    auto srv_fut = async(launch::async, [&acc]() -> bool {
        auto res = acc.accept();
        return !res;  // expect failure
    });

    tls_connector conn{cli_ctx, inet_address{"127.0.0.1", port, ec}, "", ec};
    REQUIRE(ec);  // handshake should fail

    bool srv_failed = srv_fut.get();
    REQUIRE(srv_failed);
}

TEST_CASE("PSK unknown identity rejects connection", "[tls_psk][integration]") {
    auto srv_ctx = make_psk_server_ctx();  // only accepts TEST_PSK_IDENTITY

    auto cli_ctx = tls_context::client();
    cli_ctx.set_verify(tls_context::verify_t::NONE);
    cli_ctx.set_psk("unknown-identity", TEST_PSK);

    error_code ec;
    tls_acceptor acc{srv_ctx, inet_address(0), acceptor::DFLT_QUE_SIZE, ec};
    REQUIRE(!ec);

    in_port_t port = inet_address(acc.address()).port();

    auto srv_fut = async(launch::async, [&acc]() -> bool {
        auto res = acc.accept();
        return !res;  // expect failure
    });

    tls_connector conn{cli_ctx, inet_address{"127.0.0.1", port, ec}, "", ec};
    REQUIRE(ec);  // handshake should fail

    bool srv_failed = srv_fut.get();
    REQUIRE(srv_failed);
}
