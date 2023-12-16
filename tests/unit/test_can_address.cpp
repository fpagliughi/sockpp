// test_can_address.cpp
//
// Unit tests for the sockpp `can_address` class.
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

#include <string>

#include "catch2_version.h"
#include "sockpp/can_address.h"

using namespace sockpp;
using namespace std;

// *** NOTE: The "vcan0:" virtual interface must be present. Set it up:
//   $ ip link add type vcan && ip link set up vcan0

static const string IFACE{"vcan0"};

// --------------------------------------------------------------------------

TEST_CASE("can_address default constructor", "[address]") {
    can_address addr;

    REQUIRE(!addr.is_set());
    REQUIRE(addr.iface().empty());
    REQUIRE(sizeof(sockaddr_can) == addr.size());
}

TEST_CASE("can_address iface constructor", "[address]") {
    SECTION("valid interface") {
        can_address addr(IFACE);

        REQUIRE(addr);
        REQUIRE(addr.is_set());
        REQUIRE(IFACE == addr.iface());
        REQUIRE(sizeof(sockaddr_can) == addr.size());
        REQUIRE(addr.index() > 0);
    }

    SECTION("invalid interface") { REQUIRE_THROWS(can_address("invalid")); }
}
