// test_sock_address.cpp
//
// Unit tests for the generic address classes.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi
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
#include <vector>

#include "catch2_version.h"
#include "sockpp/inet_address.h"
#include "sockpp/sock_address.h"

using namespace sockpp;

TEST_CASE("sock_address_any ctor", "[address]") {
    sockaddr_in sin;
    const size_t N = sizeof(sockaddr_in);
    memset(&sin, 0, N);
    sin.sin_family = AF_INET;

    error_code ec;
    sock_address_any addr(reinterpret_cast<sockaddr*>(&sin), N, ec);

    REQUIRE(!ec);
    REQUIRE(addr.family() == AF_INET);
    REQUIRE(addr.size() == N);
}

// --------------------------------------------------------------------------
// Default construction
// --------------------------------------------------------------------------

TEST_CASE("sock_address_any default constructor", "[address]") {
    sock_address_any addr;

    // Default-constructed address has no family set.
    REQUIRE(addr.family() == AF_UNSPEC);
    REQUIRE(!addr.is_set());
    REQUIRE(!addr);
}

// --------------------------------------------------------------------------
// Construction from sock_address
// --------------------------------------------------------------------------

TEST_CASE("sock_address_any copy from sock_address", "[address]") {
    const inet_address src{INADDR_LOOPBACK, in_port_t(TEST_PORT)};
    sock_address_any dst{src};

    REQUIRE(dst.family() == AF_INET);
    REQUIRE(dst.size() == src.size());
    // The bytes must match exactly.
    REQUIRE(std::memcmp(dst.sockaddr_ptr(), src.sockaddr_ptr(), src.size()) == 0);
}

// --------------------------------------------------------------------------
// Equality / inequality
// --------------------------------------------------------------------------

TEST_CASE("sock_address_any equality", "[address]") {
    const inet_address a{INADDR_LOOPBACK, in_port_t(TEST_PORT)};
    const inet_address b{INADDR_LOOPBACK, in_port_t(TEST_PORT)};
    const inet_address c{INADDR_LOOPBACK, in_port_t(TEST_PORT + 1)};

    sock_address_any wa{a}, wb{b}, wc{c};

    REQUIRE(wa == wb);
    REQUIRE(!(wa != wb));

    REQUIRE(wa != wc);
    REQUIRE(!(wa == wc));
}

TEST_CASE("sock_address equality cross-type (inet_address vs sock_address_any)", "[address]") {
    const inet_address addr{INADDR_LOOPBACK, in_port_t(TEST_PORT)};
    const sock_address_any any_addr{addr};

    // operator== is defined on sock_address& so cross-type comparison works.
    REQUIRE(addr == any_addr);
    REQUIRE(any_addr == addr);
    REQUIRE(!(addr != any_addr));
}

// --------------------------------------------------------------------------
// Error path: oversized address
// --------------------------------------------------------------------------

TEST_CASE("sock_address_any ec ctor oversized address", "[address]") {
    // Build a fake sockaddr buffer larger than sockaddr_storage.
    const socklen_t OVERSIZED = socklen_t(sizeof(sockaddr_storage)) + 1;
    std::vector<char> buf(OVERSIZED, 0);

    error_code ec;
    sock_address_any addr(reinterpret_cast<const sockaddr*>(buf.data()), OVERSIZED, ec);

    REQUIRE(ec);
    REQUIRE(ec == errc::invalid_argument);
}

TEST_CASE("sock_address_any throwing ctor oversized address", "[address]") {
    const socklen_t OVERSIZED = socklen_t(sizeof(sockaddr_storage)) + 1;
    std::vector<char> buf(OVERSIZED, 0);

    REQUIRE_THROWS_AS(
        sock_address_any(reinterpret_cast<const sockaddr*>(buf.data()), OVERSIZED),
        std::length_error
    );
}
