// test_unix_stream_socket.cpp
//
// Unit tests for the `unix_stream_socket` class.
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
#if __cplusplus >= 202002L
    #include <array>
    #include <span>
#endif

#include "catch2_version.h"
#include "sockpp/unix_stream_socket.h"

using namespace sockpp;

// Test that we can create a Unix-domain stream socket pair and send
// data from one of the sockets to the other.
TEST_CASE("unix stream socket pair write/read", "[unix_stream_socket]") {
    auto res = unix_stream_socket::pair();
    REQUIRE(res);

    auto [sock1, sock2] = res.release();

    REQUIRE(sock1);
    REQUIRE(sock2);

    REQUIRE(sock1.is_open());
    REQUIRE(sock2.is_open());

    const std::string MSG{"Hello there!"};
    const size_t N = MSG.length();

    char buf[512];

    REQUIRE(sock1.write(MSG).value() == N);
    REQUIRE(sock2.read_n(buf, N).value() == N);

    std::string msg{buf, buf + N};
    REQUIRE(msg == MSG);
}

#if __cplusplus >= 202002L

// Test span-based read/write overloads on stream sockets.
TEST_CASE("unix stream socket pair span write/read", "[unix_stream_socket]") {
    auto res = unix_stream_socket::pair();
    REQUIRE(res);

    auto [sock1, sock2] = res.release();

    const std::string MSG{"Hello span!"};
    const size_t N = MSG.length();

    // View the string data as a read-only byte span for writing
    auto wbuf = std::as_bytes(std::span{MSG.data(), N});

    SECTION("write(span) / read(span)") {
        std::array<std::byte, 512> rbuf{};
        REQUIRE(sock1.write(wbuf).value() == N);
        REQUIRE(sock2.read(std::span{rbuf}.first(N)).value() == N);
        REQUIRE(std::string(reinterpret_cast<const char*>(rbuf.data()), N) == MSG);
    }

    SECTION("write_n(span) / read_n(span)") {
        std::array<std::byte, 512> rbuf{};
        REQUIRE(sock1.write_n(wbuf).value() == N);
        REQUIRE(sock2.read_n(std::span{rbuf}.first(N)).value() == N);
        REQUIRE(std::string(reinterpret_cast<const char*>(rbuf.data()), N) == MSG);
    }
}

#endif  // __cplusplus >= 202002L
