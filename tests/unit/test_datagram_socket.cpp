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

#include <cstring>
#include <string>

#include "catch2_version.h"
#include "sockpp/datagram_socket.h"
#include "sockpp/inet_address.h"
#include "sockpp/udp_socket.h"

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

// --------------------------------------------------------------------------
// open()
// --------------------------------------------------------------------------

TEST_CASE("datagram_socket open", "[datagram_socket]") {
    SECTION("open with valid inet address succeeds") {
        datagram_socket sock;
        REQUIRE(!sock.is_open());

        const auto ADDR = inet_address(INADDR_LOOPBACK, in_port_t(TEST_PORT));
        auto res = sock.open(ADDR);
        REQUIRE(res);
        REQUIRE(sock.is_open());
    }

    SECTION("open with invalid address fails") {
        datagram_socket sock;
        auto res = sock.open(sock_address_any{});
        REQUIRE(!res);
    }
}

// --------------------------------------------------------------------------
// clone()
// --------------------------------------------------------------------------

TEST_CASE("datagram_socket clone", "[datagram_socket]") {
    const auto ADDR = inet_address(INADDR_LOOPBACK, in_port_t(TEST_PORT + 1));
    datagram_socket sock{ADDR};
    REQUIRE(sock.is_open());

    auto res = sock.clone();
    REQUIRE(res);

    auto dup = res.release();
    REQUIRE(dup.is_open());
    REQUIRE(dup.handle() != sock.handle());
}

// --------------------------------------------------------------------------
// UDP send_to / recv_from
// --------------------------------------------------------------------------

TEST_CASE("udp_socket send_to and recv_from", "[datagram_socket][udp]") {
    // Bind two UDP sockets to loopback with OS-assigned ports.
    udp_socket sender{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(sender);

    udp_socket receiver{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(receiver);

    const auto recv_addr = receiver.address();
    REQUIRE(recv_addr.port() != 0);

    // Set a short timeout so the test fails cleanly if no data arrives.
    REQUIRE(receiver.read_timeout(std::chrono::milliseconds{250}));

    const string MSG{"ping"};

    auto sres = sender.send_to(MSG, recv_addr);
    REQUIRE(sres);
    REQUIRE(sres.value() == MSG.size());

    char buf[64]{};
    inet_address src_addr;
    auto rres = receiver.recv_from(buf, sizeof(buf) - 1, &src_addr);
    REQUIRE(rres);
    REQUIRE(rres.value() == MSG.size());
    REQUIRE(string(buf, MSG.size()) == MSG);

    // The source address should be the loopback address.
    REQUIRE(src_addr.address() == INADDR_LOOPBACK);
}

TEST_CASE("udp_socket connect and send/recv", "[datagram_socket][udp]") {
    udp_socket sender{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(sender);

    udp_socket receiver{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(receiver);

    const auto recv_addr = receiver.address();
    REQUIRE(receiver.read_timeout(std::chrono::milliseconds{250}));

    // connect() on a UDP socket sets the default destination.
    REQUIRE(sender.connect(recv_addr));

    const string MSG{"connected udp"};
    // After connect(), send() goes to the connected address.
    auto sres = sender.send(MSG);
    REQUIRE(sres);
    REQUIRE(sres.value() == MSG.size());

    char buf[64]{};
    auto rres = receiver.recv_from(buf, sizeof(buf) - 1, nullptr);
    REQUIRE(rres);
    REQUIRE(rres.value() == MSG.size());
    REQUIRE(string(buf, MSG.size()) == MSG);
}
