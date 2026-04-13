// test_unix_dgram_socket.cpp
//
// Unit tests for the `unix_dgram_socket` class.
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
#if __cplusplus >= 202002L
    #include <array>
    #include <span>
#endif

#include "catch2_version.h"
#include "sockpp/unix_dgram_socket.h"

using namespace sockpp;

// Test that we can create a Unix-domain datagram socket pair and send data
// from one of the sockets to the other.
TEST_CASE("unix dgram socket pair", "[unix_dgram_socket]") {
    auto res = unix_dgram_socket::pair();
    REQUIRE(res);

    auto [sock1, sock2] = res.release();

    REQUIRE(sock1);
    REQUIRE(sock2);

    REQUIRE(sock1.is_open());
    REQUIRE(sock2.is_open());

    const std::string MSG{"Hello there!"};
    const size_t N = MSG.length();

    char buf[512];

    REQUIRE(sock1.send(MSG).value() == N);
    REQUIRE(sock2.recv(buf, N).value() == N);

    std::string msg{buf, buf + N};
    REQUIRE(msg == MSG);
}

#if __cplusplus >= 202002L

// Test span-based send/recv overloads on a connected datagram socket pair.
// These exercise the socket::send(span) and socket::recv(span) base methods.
TEST_CASE("unix dgram socket pair span send/recv", "[unix_dgram_socket]") {
    auto res = unix_dgram_socket::pair();
    REQUIRE(res);

    auto [sock1, sock2] = res.release();

    const std::string MSG{"Hello span!"};
    const size_t N = MSG.length();

    auto wbuf = std::as_bytes(std::span{MSG.data(), N});
    std::array<std::byte, 512> rbuf{};

    REQUIRE(sock1.send(wbuf).value() == N);
    REQUIRE(sock2.recv(std::span{rbuf}.first(N)).value() == N);
    REQUIRE(std::string(reinterpret_cast<const char*>(rbuf.data()), N) == MSG);
}

// Test span-based send_to/recv_from overloads on datagram_socket_tmpl<unix_address>.
// These exercise the typed template overloads with actual bound socket paths.
TEST_CASE("unix dgram socket span send_to/recv_from", "[unix_dgram_socket]") {
    const unix_address ADDR1{"/tmp/sockpp_test_span1.sock"};
    const unix_address ADDR2{"/tmp/sockpp_test_span2.sock"};

    // Remove any leftover socket files from a prior run
    ::unlink(ADDR1.path().c_str());
    ::unlink(ADDR2.path().c_str());

    unix_dgram_socket sock1{ADDR1};
    unix_dgram_socket sock2{ADDR2};
    REQUIRE(sock1);
    REQUIRE(sock2);

    const std::string MSG{"Hello span!"};
    const size_t N = MSG.length();

    auto wbuf = std::as_bytes(std::span{MSG.data(), N});
    std::array<std::byte, 512> rbuf{};

    SECTION("send_to(span, addr) / recv_from(span)") {
        REQUIRE(sock1.send_to(wbuf, ADDR2).value() == N);
        REQUIRE(sock2.recv_from(std::span{rbuf}.first(N)).value() == N);
        REQUIRE(std::string(reinterpret_cast<const char*>(rbuf.data()), N) == MSG);
    }

    SECTION("send_to(span, flags, addr) / recv_from(span, flags, src)") {
        unix_address srcAddr;
        REQUIRE(sock1.send_to(wbuf, 0, ADDR2).value() == N);
        REQUIRE(sock2.recv_from(std::span{rbuf}.first(N), 0, &srcAddr).value() == N);
        REQUIRE(std::string(reinterpret_cast<const char*>(rbuf.data()), N) == MSG);
        REQUIRE(srcAddr.path() == ADDR1.path());
    }

    ::unlink(ADDR1.path().c_str());
    ::unlink(ADDR2.path().c_str());
}

#endif  // __cplusplus >= 202002L
