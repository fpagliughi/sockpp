// test_connector.cpp
//
// Unit tests for the `connector` class(es).
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019 Frank Pagliughi
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
//

#include <string>

#include "catch2_version.h"
#include "sockpp/connector.h"
#include "sockpp/inet_address.h"
#include "sockpp/sock_address.h"
#include "sockpp/tcp_acceptor.h"
#include "sockpp/tcp_connector.h"

using namespace sockpp;

// Test that connector errors properly when given an empty address.
TEST_CASE("connector unspecified address", "[connector]") {
    connector conn;
    REQUIRE(!conn);

    sock_address_any addr;

    auto res = conn.connect(addr);
    REQUIRE(!res);

// Windows returns a different error code than *nix
#if defined(_WIN32)
    REQUIRE(errc::invalid_argument == res);
#else
    REQUIRE(errc::address_family_not_supported == res);
#endif
}

// --------------------------------------------------------------------------
// Successful connect via TCP loopback
// --------------------------------------------------------------------------

TEST_CASE("tcp_connector constructor connects successfully", "[connector]") {
    tcp_acceptor acc{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(acc);

    const auto port = acc.address().port();

    tcp_connector cli{inet_address(INADDR_LOOPBACK, port)};
    REQUIRE(cli);
    REQUIRE(cli.is_open());
}

TEST_CASE("tcp_connector ec constructor connects successfully", "[connector]") {
    tcp_acceptor acc{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(acc);

    const auto port = acc.address().port();

    error_code ec;
    tcp_connector cli{inet_address(INADDR_LOOPBACK, port), ec};
    REQUIRE(!ec);
    REQUIRE(cli.is_open());
}

TEST_CASE("tcp_connector connect() method succeeds", "[connector]") {
    tcp_acceptor acc{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(acc);

    const auto port = acc.address().port();

    tcp_connector cli;
    REQUIRE(!cli.is_open());

    auto res = cli.connect(inet_address(INADDR_LOOPBACK, port));
    REQUIRE(res);
    REQUIRE(cli.is_open());
}

TEST_CASE("tcp_connector connection refused", "[connector]") {
    // Create a TCP socket bound to port 0 but never listening.
    // The connector should fail and set ec.
    // Note: the exact error code is platform-dependent — Linux gives
    // ECONNREFUSED, macOS/BSD may give ECONNRESET or a related code.
    // The connector also keeps its OS socket open even on failure, so
    // is_open() is not a reliable indicator of success.
    auto bound = socket::create(AF_INET, SOCK_STREAM).release();
    REQUIRE(bound.is_open());
    inet_address bind_addr{INADDR_LOOPBACK, 0};
    REQUIRE(bound.bind(bind_addr));
    const auto port = inet_address(bound.address()).port();

    error_code ec;
    tcp_connector cli{inet_address(INADDR_LOOPBACK, port), ec};

    REQUIRE(ec);
}
