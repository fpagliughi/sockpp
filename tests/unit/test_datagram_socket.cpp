// test_datagram_socket.cpp
//
// Unit tests for the `datagram_socket` class(es).
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019-2023 Frank Pagliughi
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
#include "sockpp/datagram_socket.h"
#include "sockpp/inet_address.h"

using namespace std;
using namespace sockpp;

TEST_CASE("datagram_socket default constructor", "[datagram_socket]") {
    datagram_socket sock;
    REQUIRE(!sock);
    REQUIRE(!sock.is_open());
}

TEST_CASE("datagram_socket handle constructor", "[datagram_socket]") {
    constexpr auto HANDLE = socket_t(3);

    SECTION("valid handle") {
        datagram_socket sock(HANDLE);
        REQUIRE(sock);
        REQUIRE(sock.is_open());
    }

    SECTION("invalid handle") {
        datagram_socket sock(INVALID_SOCKET);
        REQUIRE(!sock);
        REQUIRE(!sock.is_open());
        // TODO: Should this set an error?
    }
}

TEST_CASE("datagram_socket address constructor", "[datagram_socket]") {
    SECTION("valid address") {
        const auto ADDR = inet_address("localhost", TEST_PORT);

        datagram_socket sock(ADDR);
        REQUIRE(sock);
        REQUIRE(sock.address() == ADDR);
    }

    SECTION("invalid address throws") {
        const auto ADDR = sock_address_any{};

        try {
            datagram_socket sock(ADDR);
            REQUIRE(false);
        }
        catch (const system_error& exc) {
#if defined(_WIN32)
            REQUIRE(exc.code() == errc::invalid_argument);
#else
            REQUIRE(exc.code() == errc::address_family_not_supported);
#endif
        }
    }

    SECTION("invalid address ec") {
        const auto ADDR = sock_address_any{};

        error_code ec;
        datagram_socket sock{ADDR, ec};

        REQUIRE(!sock);

        // Windows returns a different error code than *nix
#if defined(_WIN32)
        REQUIRE(ec == errc::invalid_argument);
#else
        REQUIRE(ec == errc::address_family_not_supported);
#endif
    }
}
