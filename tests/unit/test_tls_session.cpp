// test_tls_session.cpp
//
// Integration tests for TLS session resumption.
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

#include "catch2_version.h"
#include "sockpp/inet_address.h"
#include "sockpp/tls/acceptor.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"

using namespace std;
using namespace sockpp;

// Self-signed P-256/SHA-256 certificate for localhost / 127.0.0.1.
// Valid for 10 years from 2026-05-09.
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

/** Creates a server context with the test certificate and server-side cache. */
static tls_context make_server_ctx() {
    auto ctx = tls_context::server();
    REQUIRE(ctx.set_identity(TEST_CERT, TEST_KEY));
    ctx.set_session_cache_mode(tls_context::session_cache_mode::SERVER);
    return ctx;
}

/** Creates a client context that trusts only the test certificate. */
static tls_context make_client_ctx() {
    auto ctx = tls_context::client();
    ctx.set_root_certs(TEST_CERT);
    return ctx;
}

/**
 * Runs a single-accept echo server on the given acceptor.
 * Accepts one connection, echoes one message back, then returns.
 */
static void run_echo_server(tls_acceptor& acc) {
    auto res = acc.accept();
    if (!res)
        return;
    auto sock = res.release();

    char buf[256];
    auto rres = sock.read(buf, sizeof(buf));
    if (rres)
        sock.write(buf, rres.value());
}

/**
 * Like run_echo_server() but returns whether the server-side TLS session was
 * resumed.  The server is the authoritative side for session resumption: it
 * sets hit=1 for both session-ID and ticket-based resumption, whereas the
 * client's SSL_session_reused() can return false for ticket-based resumption
 * in OpenSSL 3.x when TLS 1.2 session IDs are zero-length.
 */
static bool echo_server_session_reused(tls_acceptor& acc) {
    auto res = acc.accept();
    if (!res)
        return false;
    auto sock = res.release();
    bool reused = sock.session_reused();

    char buf[256];
    auto rres = sock.read(buf, sizeof(buf));
    if (rres)
        sock.write(buf, rres.value());

    return reused;
}

// ===========================================================================
// Tests
// ===========================================================================

TEST_CASE("tls_session default-constructed is invalid", "[tls][session]") {
    tls_session s;
    REQUIRE_FALSE(s.is_valid());
    REQUIRE(s.to_bytes().empty());
}

TEST_CASE("tls_session get and serialise", "[tls][session]") {
    auto svrCtx = make_server_ctx();
    auto cliCtx = make_client_ctx();

    tls_acceptor acc{svrCtx};
    REQUIRE(acc.open(inet_address(0)));
    in_port_t port = inet_address(acc.address()).port();

    auto svrFut = std::async(std::launch::async, [&] { run_echo_server(acc); });

    // Connect and exchange data so the TLS 1.3 session ticket is delivered.
    error_code ec;
    tls_connector conn{cliCtx, ec};
    REQUIRE_FALSE(ec);

    REQUIRE(conn.connect(inet_address("127.0.0.1", port, ec)));

    const string msg = "hello";
    REQUIRE(conn.write(msg));

    char buf[64]{};
    REQUIRE(conn.read(buf, sizeof(buf)));

    svrFut.get();

    // get_session() should now hold a valid ticket.
    auto sessRes = conn.get_session();
    REQUIRE(sessRes);
    auto& sess = sessRes.value();
    REQUIRE(sess.is_valid());

    SECTION("serialise and restore") {
        binary bytes = sess.to_bytes();
        REQUIRE_FALSE(bytes.empty());

        auto restored = tls_session::from_bytes(bytes);
        REQUIRE(restored);
        REQUIRE(restored.value().is_valid());

        // Round-tripped bytes should be identical.
        binary bytes2 = restored.value().to_bytes();
        REQUIRE(bytes == bytes2);
    }
}

TEST_CASE("tls_session resumption (TLS 1.2)", "[tls][session]") {
    // Pin to TLS 1.2.  In TLS 1.3, NewSessionTicket is delivered as a
    // post-handshake record that may arrive after the first application-data
    // read; capturing it reliably requires a new-session callback.  TLS 1.2
    // delivers the ticket during the handshake, so SSL_get1_session() is
    // immediately usable after SSL_connect() returns.
    //
    // We use session TICKETS (the default) rather than session-ID-based caching:
    // in OpenSSL 3.0, TLS 1.2 session IDs have zero length even with
    // SSL_OP_NO_TICKET, so the internal session-ID cache is never populated.
    // With tickets the server embeds the session data in an encrypted blob
    // sent to the client; the client re-sends the blob on the next connection,
    // and the server decrypts and resumes.  SSL_session_reused() returns true
    // in both cases when the server accepts the offered session.
    auto svrCtx = make_server_ctx();
    auto cliCtx = make_client_ctx();
    svrCtx.set_max_tls_version(tls_context::tls_version::TLS_1_2);
    cliCtx.set_max_tls_version(tls_context::tls_version::TLS_1_2);

    tls_acceptor acc{svrCtx};
    REQUIRE(acc.open(inet_address(0)));
    in_port_t port = inet_address(acc.address()).port();

    // ---- First connection: full handshake, capture the session ----
    auto svrFut = std::async(std::launch::async, [&] { run_echo_server(acc); });

    error_code ec;
    tls_session saved_session;
    {
        tls_connector conn{cliCtx, ec};
        REQUIRE_FALSE(ec);
        REQUIRE(conn.connect(inet_address("127.0.0.1", port, ec)));

        const string msg = "first";
        REQUIRE(conn.write(msg));
        char buf[64]{};
        REQUIRE(conn.read(buf, sizeof(buf)));

        REQUIRE(conn.negotiated_version() == "TLSv1.2");

        // For TLS 1.2 the session is established at the end of the handshake.
        auto sessRes = conn.get_session();
        REQUIRE(sessRes);
        saved_session = std::move(sessRes.value());
    }
    svrFut.get();

    REQUIRE(saved_session.is_valid());

    // ---- Second connection: offer the saved session for resumption ----
    // Check resumption on the SERVER side: in OpenSSL 3.x, TLS 1.2 sessions
    // can have zero-length session IDs, making the client-side
    // SSL_session_reused() unreliable (it guards on session_id_length != 0).
    // The server sets hit=1 authoritatively for both session-ID and
    // ticket-based resumption.
    auto svrResumeFut =
        std::async(std::launch::async, [&] { return echo_server_session_reused(acc); });

    {
        tls_connector conn{cliCtx, ec};
        REQUIRE_FALSE(ec);

        REQUIRE(conn.set_session(saved_session));
        REQUIRE(conn.connect(inet_address("127.0.0.1", port, ec)));

        const string msg = "second";
        REQUIRE(conn.write(msg));
        char buf[64]{};
        REQUIRE(conn.read(buf, sizeof(buf)));
    }
    bool server_saw_resumption = svrResumeFut.get();
    REQUIRE(server_saw_resumption);
}

TEST_CASE("tls_context session cache mode", "[tls][session]") {
    // Just verify the API doesn't throw/crash for all modes.
    tls_context ctx{tls_context::role_t::SERVER};
    ctx.set_session_cache_mode(tls_context::session_cache_mode::OFF);
    ctx.set_session_cache_mode(tls_context::session_cache_mode::CLIENT);
    ctx.set_session_cache_mode(tls_context::session_cache_mode::SERVER);
    ctx.set_session_cache_mode(tls_context::session_cache_mode::BOTH);
    ctx.set_session_cache_size(512);
}

TEST_CASE("tls_context_builder session cache", "[tls][session]") {
    auto ctx = tls_context_builder::server()
                   .session_cache_mode(tls_context::session_cache_mode::SERVER)
                   .session_cache_size(256)
                   .finalize();
    // If we got here without throwing, the builder wired up correctly.
    REQUIRE(true);
}
