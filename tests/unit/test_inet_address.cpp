// test_inet_address.cpp
//
// Unit tests for the `inet_address` class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2018-2023 Frank Pagliughi
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
#include "sockpp/inet_address.h"

using namespace sockpp;

const uint32_t ANY_ADDR{INADDR_ANY};             // Any iface 0x00000000
const uint32_t LOCALHOST_ADDR{INADDR_LOOPBACK};  // Localhost 0x7F000001
const std::string LOCALHOST_STR{"localhost"};
const in_port_t PORT{sockpp::TEST_PORT};

TEST_CASE("inet_address default constructor", "[address]") {
    SECTION("default address") {
        inet_address addr;

        REQUIRE(!addr);
        REQUIRE(!addr.is_set());
        REQUIRE(0 == addr.address());
        REQUIRE(0 == addr.port());
        REQUIRE(sizeof(sockaddr_in) == addr.size());
    }

    SECTION("creating address from int32") {
        inet_address addr{LOCALHOST_ADDR, PORT};

        REQUIRE(addr);
        REQUIRE(addr.is_set());
        REQUIRE(LOCALHOST_ADDR == addr.address());
        REQUIRE(PORT == addr.port());

        REQUIRE(uint8_t((LOCALHOST_ADDR >> 0) & 0xFF) == addr[0]);
        REQUIRE(uint8_t((LOCALHOST_ADDR >> 8) & 0xFF) == addr[1]);
        REQUIRE(uint8_t((LOCALHOST_ADDR >> 16) & 0xFF) == addr[2]);
        REQUIRE(uint8_t((LOCALHOST_ADDR >> 24) & 0xFF) == addr[3]);

        // Check the low-level struct
        REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
        REQUIRE(LOCALHOST_ADDR == ntohl(addr.sockaddr_in_ptr()->sin_addr.s_addr));
        REQUIRE(PORT == ntohs(addr.sockaddr_in_ptr()->sin_port));
    }

    SECTION("creating address from name") {
        inet_address addr(LOCALHOST_STR, PORT);

        REQUIRE(addr);
        REQUIRE(addr.is_set());
        REQUIRE(LOCALHOST_ADDR == addr.address());
        REQUIRE(PORT == addr.port());

        // Check the low-level struct
        REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
        REQUIRE(LOCALHOST_ADDR == ntohl(addr.sockaddr_in_ptr()->sin_addr.s_addr));
        REQUIRE(PORT == ntohs(addr.sockaddr_in_ptr()->sin_port));
    }
}

// When created using only a port number this should use the
// "any" address to bind to all interfaces (typ for server)
TEST_CASE("inet_address port-only constructor", "[address]") {
    inet_address addr(PORT);

    REQUIRE(addr.is_set());
    REQUIRE(ANY_ADDR == addr.address());
    REQUIRE(PORT == addr.port());

    // Check the low-level struct
    REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
}

TEST_CASE("inet_address int32_t constructor", "[address]") {
    inet_address addr(LOCALHOST_ADDR, PORT);

    REQUIRE(addr);
    REQUIRE(addr.is_set());
    REQUIRE(LOCALHOST_ADDR == addr.address());
    REQUIRE(PORT == addr.port());

    REQUIRE(uint8_t((LOCALHOST_ADDR >> 0) & 0xFF) == addr[0]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 8) & 0xFF) == addr[1]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 16) & 0xFF) == addr[2]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 24) & 0xFF) == addr[3]);

    // Check the low-level struct
    REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
    REQUIRE(LOCALHOST_ADDR == ntohl(addr.sockaddr_in_ptr()->sin_addr.s_addr));
    REQUIRE(PORT == ntohs(addr.sockaddr_in_ptr()->sin_port));
}

TEST_CASE("inet_address name constructor", "[address]") {
    inet_address addr(LOCALHOST_STR, PORT);

    REQUIRE(addr);
    REQUIRE(addr.is_set());
    REQUIRE(LOCALHOST_ADDR == addr.address());
    REQUIRE(PORT == addr.port());

    REQUIRE(uint8_t((LOCALHOST_ADDR >> 0) & 0xFF) == addr[0]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 8) & 0xFF) == addr[1]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 16) & 0xFF) == addr[2]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 24) & 0xFF) == addr[3]);

    // Check the low-level struct
    REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
    REQUIRE(LOCALHOST_ADDR == ntohl(addr.sockaddr_in_ptr()->sin_addr.s_addr));
    REQUIRE(PORT == ntohs(addr.sockaddr_in_ptr()->sin_port));
}

TEST_CASE("IPv4 resolve address", "[address]") {
    SECTION("localhost numeric") {
        auto res = inet_address::resolve_name("127.0.0.1");
        REQUIRE(res.is_ok());

        auto addr = res.value();
        REQUIRE(addr == htonl(LOCALHOST_ADDR));
    }

    SECTION("localhost name") {
        auto res = inet_address::resolve_name(LOCALHOST_STR);
        REQUIRE(res.is_ok());

        auto addr = res.value();
        REQUIRE(addr == htonl(LOCALHOST_ADDR));
    }

// According to RFC6761, "invalid." domain should not resolve
#if !defined(_WIN32)
    SECTION("resolve failure", "[address]") {
        auto res = inet_address::resolve_name("invalid");
        REQUIRE(!res);
        REQUIRE(res.is_error());
    }
#endif
}

TEST_CASE("IPv4 create address", "[address]") {
    SECTION("localhost numeric") {
        auto res = inet_address::create("127.0.0.1", PORT);
        REQUIRE(res.is_ok());

        auto addr = res.value();

        REQUIRE(PORT == addr.port());
        REQUIRE(LOCALHOST_ADDR == addr.address());
        REQUIRE(inet_address{LOCALHOST_ADDR, PORT} == addr);
    }

    SECTION("localhost name") {
        auto res = inet_address::create(LOCALHOST_STR, PORT);
        REQUIRE(res.is_ok());

        auto addr = res.value();
        REQUIRE(PORT == addr.port());
        REQUIRE(LOCALHOST_ADDR == addr.address());
        REQUIRE(inet_address{LOCALHOST_ADDR, PORT} == addr);
    }

// According to RFC6761, "invalid." domain should not resolve
#if !defined(_WIN32)
    SECTION("create failure", "[address]") {
        auto res = inet_address::create("invalid", PORT);
        REQUIRE(!res);
        REQUIRE(res.is_error());
    }
#endif
}
