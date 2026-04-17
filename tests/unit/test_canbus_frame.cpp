// test_canbus_frame.cpp
//
// Unit tests for the sockpp `canbus_frame` and `canbusfd_frame` classes.
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
//

#include <cstring>
#include <string>
#include <system_error>

#include "catch2_version.h"
#include "sockpp/canbus_frame.h"

using namespace sockpp;
using namespace std;
using namespace std::string_literals;

// --------------------------------------------------------------------------
// canbus_frame construction
// --------------------------------------------------------------------------

TEST_CASE("canbus_frame default constructor", "[canbus][frame]") {
    canbus_frame frame{};

    REQUIRE(frame.id_value() == 0);
    REQUIRE(!frame.has_extended_id());
    REQUIRE(!frame.is_remote());
    REQUIRE(!frame.is_error());
    REQUIRE(frame.len == 0);
}

TEST_CASE("canbus_frame id-only constructor", "[canbus][frame]") {
    constexpr canid_t ID = 0x123;
    canbus_frame frame{ID};

    REQUIRE(frame.id_value() == ID);
    REQUIRE(!frame.has_extended_id());
    REQUIRE(!frame.is_remote());
    REQUIRE(!frame.is_error());
    REQUIRE(frame.len == 0);
}

TEST_CASE("canbus_frame string data constructor", "[canbus][frame]") {
    constexpr canid_t ID = 0x456;
    const string DATA{"hello"s};

    canbus_frame frame{ID, DATA};

    REQUIRE(frame.id_value() == ID);
    REQUIRE(frame.len == DATA.size());
    REQUIRE(memcmp(frame.data, DATA.data(), DATA.size()) == 0);
}

TEST_CASE("canbus_frame raw data constructor", "[canbus][frame]") {
    constexpr canid_t ID = 0x321;
    const uint8_t BUF[] = {0x01, 0x02, 0x03, 0x04};
    constexpr size_t N = sizeof(BUF);

    canbus_frame frame{ID, BUF, N};

    REQUIRE(frame.id_value() == ID);
    REQUIRE(frame.len == N);
    REQUIRE(memcmp(frame.data, BUF, N) == 0);
}

TEST_CASE("canbus_frame raw data constructor clamps to max length", "[canbus][frame]") {
    constexpr canid_t ID = 0x1;
    // Build a buffer larger than CAN_MAX_DLEN (8 bytes)
    const uint8_t BUF[CAN_MAX_DLEN + 4] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC
    };

    canbus_frame frame{ID, BUF, sizeof(BUF)};

    REQUIRE(frame.len == CAN_MAX_DLEN);
    REQUIRE(memcmp(frame.data, BUF, CAN_MAX_DLEN) == 0);
}

// --------------------------------------------------------------------------
// canbus_frame flags
// --------------------------------------------------------------------------

TEST_CASE("canbus_frame has_extended_id flag", "[canbus][frame]") {
    SECTION("standard frame has no extended ID") {
        canbus_frame frame{0x7FF};
        REQUIRE(!frame.has_extended_id());
    }

    SECTION("frame with EFF flag has extended ID") {
        canbus_frame frame{CAN_EFF_FLAG | 0x1234567};
        REQUIRE(frame.has_extended_id());
    }
}

TEST_CASE("canbus_frame is_remote flag", "[canbus][frame]") {
    SECTION("normal data frame is not remote") {
        canbus_frame frame{0x100, "x"s};
        REQUIRE(!frame.is_remote());
    }

    SECTION("frame with RTR flag is remote") {
        canbus_frame frame{CAN_RTR_FLAG | 0x100};
        REQUIRE(frame.is_remote());
    }
}

TEST_CASE("canbus_frame is_error flag", "[canbus][frame]") {
    SECTION("normal frame is not an error frame") {
        canbus_frame frame{0x100};
        REQUIRE(!frame.is_error());
    }

    SECTION("frame with ERR flag is an error frame") {
        canbus_frame frame{CAN_ERR_FLAG | 0x100};
        REQUIRE(frame.is_error());
    }
}

// --------------------------------------------------------------------------
// canbus_frame id_value
// --------------------------------------------------------------------------

TEST_CASE("canbus_frame id_value standard", "[canbus][frame]") {
    // Maximum 11-bit standard ID; RTR and ERR bits must be masked out
    constexpr canid_t ID = CAN_SFF_MASK;  // 0x7FF
    canbus_frame frame{ID};

    REQUIRE(!frame.has_extended_id());
    REQUIRE(frame.id_value() == ID);
}

TEST_CASE("canbus_frame id_value extended", "[canbus][frame]") {
    // 29-bit extended ID with EFF flag set
    constexpr canid_t ID = 0x1ABCDEF;  // within CAN_EFF_MASK (0x1FFFFFFF)
    canbus_frame frame{CAN_EFF_FLAG | ID};

    REQUIRE(frame.has_extended_id());
    REQUIRE(frame.id_value() == ID);
}

// --------------------------------------------------------------------------
// canbus_frame ID mutation
// --------------------------------------------------------------------------

TEST_CASE("canbus_frame set_standard_id", "[canbus][frame]") {
    // Start with an extended-ID frame, then replace it with a standard one.
    canbus_frame frame{CAN_EFF_FLAG | 0x1234567};
    REQUIRE(frame.has_extended_id());

    constexpr canid_t NEW_ID = 0x55;
    frame.set_standard_id(NEW_ID);

    REQUIRE(!frame.has_extended_id());
    REQUIRE(frame.id_value() == NEW_ID);
}

TEST_CASE("canbus_frame set_extended_id", "[canbus][frame]") {
    // Start with a plain standard-ID frame, then upgrade to extended.
    canbus_frame frame{0x1FF};
    REQUIRE(!frame.has_extended_id());

    constexpr canid_t NEW_ID = 0x1FEDCBA;
    frame.set_extended_id(NEW_ID);

    REQUIRE(frame.has_extended_id());
    REQUIRE(frame.id_value() == NEW_ID);
}

// --------------------------------------------------------------------------
// canbus_remote_frame
// --------------------------------------------------------------------------

TEST_CASE("canbus_remote_frame default constructor", "[canbus][frame]") {
    canbus_remote_frame frame{};

    REQUIRE(frame.is_remote());
    REQUIRE(!frame.has_extended_id());
    REQUIRE(!frame.is_error());
    REQUIRE(frame.id_value() == 0);
    REQUIRE(frame.len == 0);
}

TEST_CASE("canbus_remote_frame id constructor", "[canbus][frame]") {
    constexpr canid_t ID = 0x3FF;
    canbus_remote_frame frame{ID};

    REQUIRE(frame.is_remote());
    REQUIRE(!frame.has_extended_id());
    REQUIRE(frame.id_value() == ID);
}

// --------------------------------------------------------------------------
// canbus_frame conversions
// --------------------------------------------------------------------------

TEST_CASE("canbus_frame conversion classic to FD", "[canbus][frame]") {
    constexpr canid_t ID = 0x42;
    const string DATA{"hello"s};

    canbus_frame classic{ID, DATA};
    canbusfd_frame fdframe{classic};

    REQUIRE(fdframe.id_value() == ID);
    REQUIRE(fdframe.len == DATA.size());
    REQUIRE(memcmp(fdframe.data, DATA.data(), DATA.size()) == 0);
}

TEST_CASE("canbus_frame conversion FD to classic success", "[canbus][frame]") {
    constexpr canid_t ID = 0x77;
    const string DATA{"hi"s};

    canbusfd_frame fdframe{ID, DATA};
    canbus_frame classic{fdframe};

    REQUIRE(classic.id_value() == ID);
    REQUIRE(classic.len == DATA.size());
    REQUIRE(memcmp(classic.data, DATA.data(), DATA.size()) == 0);
}

TEST_CASE("canbus_frame conversion FD to classic throws on overflow", "[canbus][frame]") {
    // Build an FD frame with more than CAN_MAX_DLEN (8) bytes of data.
    constexpr canid_t ID = 0x10;
    const string DATA(CAN_MAX_DLEN + 1, 0xAB);

    canbusfd_frame fdframe{ID, DATA};
    REQUIRE(fdframe.len > CAN_MAX_DLEN);

    REQUIRE_THROWS_AS(canbus_frame{fdframe}, system_error);
}

TEST_CASE("canbus_frame conversion FD to classic error_code on overflow", "[canbus][frame]") {
    constexpr canid_t ID = 0x20;
    const string DATA(CAN_MAX_DLEN + 2, 0xCD);

    canbusfd_frame fdframe{ID, DATA};
    REQUIRE(fdframe.len > CAN_MAX_DLEN);

    error_code ec;
    canbus_frame classic{fdframe, ec};

    REQUIRE(ec);
    REQUIRE(ec == errc::invalid_argument);
}

TEST_CASE("canbus_frame conversion FD to classic error_code success", "[canbus][frame]") {
    constexpr canid_t ID = 0x30;
    const string DATA{"ok"s};

    canbusfd_frame fdframe{ID, DATA};

    error_code ec;
    canbus_frame classic{fdframe, ec};

    REQUIRE(!ec);
    REQUIRE(classic.id_value() == ID);
    REQUIRE(classic.len == DATA.size());
}

// --------------------------------------------------------------------------
// canbusfd_frame construction
// --------------------------------------------------------------------------

TEST_CASE("canbusfd_frame default constructor", "[canbus][fdframe]") {
    canbusfd_frame frame{};

    REQUIRE(frame.id_value() == 0);
    REQUIRE(!frame.has_extended_id());
    REQUIRE(frame.len == 0);
}

TEST_CASE("canbusfd_frame string data constructor", "[canbus][fdframe]") {
    constexpr canid_t ID = 0x111;
    // Use a payload longer than CAN_MAX_DLEN to confirm FD can handle it.
    const string DATA(20, 0x55);

    canbusfd_frame frame{ID, DATA};

    REQUIRE(frame.id_value() == ID);
    REQUIRE(frame.len == DATA.size());
    REQUIRE(memcmp(frame.data, DATA.data(), DATA.size()) == 0);
}

TEST_CASE("canbusfd_frame raw data constructor clamps to max FD length", "[canbus][fdframe]") {
    constexpr canid_t ID = 0x2;
    const size_t OVERSIZED = CANFD_MAX_DLEN + 8;
    vector<uint8_t> buf(OVERSIZED, 0xAA);

    canbusfd_frame frame{ID, buf.data(), buf.size()};

    REQUIRE(frame.len == CANFD_MAX_DLEN);
}

// --------------------------------------------------------------------------
// canbusfd_frame flags and ID
// --------------------------------------------------------------------------

TEST_CASE("canbusfd_frame has_extended_id flag", "[canbus][fdframe]") {
    SECTION("standard ID") {
        canbusfd_frame frame{0x7FF, ""s};
        REQUIRE(!frame.has_extended_id());
    }

    SECTION("extended ID with EFF flag") {
        canbusfd_frame frame{CAN_EFF_FLAG | 0x1ABCDEF, ""s};
        REQUIRE(frame.has_extended_id());
        REQUIRE(frame.id_value() == canid_t{0x1ABCDEF});
    }
}

TEST_CASE("canbusfd_frame set_standard_id", "[canbus][fdframe]") {
    canbusfd_frame frame{CAN_EFF_FLAG | 0x1234567, ""s};
    REQUIRE(frame.has_extended_id());

    constexpr canid_t NEW_ID = 0x7F;
    frame.set_standard_id(NEW_ID);

    REQUIRE(!frame.has_extended_id());
    REQUIRE(frame.id_value() == NEW_ID);
}

TEST_CASE("canbusfd_frame set_extended_id", "[canbus][fdframe]") {
    canbusfd_frame frame{0x100, ""s};
    REQUIRE(!frame.has_extended_id());

    constexpr canid_t NEW_ID = 0x1FEDCBA;
    frame.set_extended_id(NEW_ID);

    REQUIRE(frame.has_extended_id());
    REQUIRE(frame.id_value() == NEW_ID);
}
