// test_result.cpp
//
// Unit tests for the result class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi All rights reserved.
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

#include "catch2_version.h"
#include "sockpp/result.h"

using namespace std;
using namespace sockpp;

class moveable
{
    int val_;
    moveable(const moveable&) = delete;
    moveable& operator=(const moveable&) = delete;

public:
    moveable(int val) : val_{val} {}
    moveable(moveable&& other) : val_{other.val_} { other.val_ = 0; }
    int val() const { return val_; }
};

ostream& operator<<(ostream& os, const moveable& v) {
    os << v.val();
    return os;
}

// --------------------------------------------------------------------------

TEST_CASE("test result success", "[result]") {
    const int VAL = 42;
    result<int> res{VAL};

    REQUIRE(res);
    REQUIRE(res.is_ok());
    REQUIRE(!res.is_error());
    REQUIRE(res.value() == VAL);
    REQUIRE(res.error() == error_code{});
    REQUIRE(res.error().value() == 0);
}

TEST_CASE("test result error", "[result]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);

    REQUIRE(!res);
    REQUIRE(!res.is_ok());
    REQUIRE(res.is_error());
    REQUIRE(res.error() == ERR);
    REQUIRE(res == ERR);
    REQUIRE(res == std::make_error_code(ERR));
}

TEST_CASE("test result release", "[result]") {
    const int VAL = 42;
    result<moveable> res{moveable{42}};

    REQUIRE(res);
    REQUIRE(VAL == res.value().val());

    auto val = res.release();
    REQUIRE(VAL == val.val());
    REQUIRE(0 == res.value().val());
}

TEST_CASE("test result cmp error", "[result]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);

    REQUIRE(!res);

    // Should compare to different forms of the error

    REQUIRE(res == ERR);
    REQUIRE(res == std::make_error_code(ERR));

    REQUIRE(!(res != ERR));

    REQUIRE(res != errc::bad_address);
    REQUIRE(!(res == errc::bad_address));

    // Errors should never equal _any_ value type.

    REQUIRE(res != 42);
    REQUIRE(!(res == 42));

    REQUIRE(res != int{});
    REQUIRE(!(res == int{}));
}

TEST_CASE("test result cmp value", "[result]") {
    const int VAL = 42;
    auto res = result<int>{VAL};

    REQUIRE(res);
    REQUIRE(!(!res));

    REQUIRE(res == 42);
    REQUIRE(!(res != 42));

    REQUIRE(res != 29);
    REQUIRE(!(res == 29));
    REQUIRE(res != 0);
    REQUIRE(!(res == 0));

    REQUIRE(res != errc::interrupted);
}

TEST_CASE("test result no error", "[result]") {
    // Zero error means success
    auto res = result<int>::from_error(0);

    REQUIRE(res);
    REQUIRE(res.error().value() == 0);
    REQUIRE(res.value() == int{});
    REQUIRE(res.value() != 42);
}
