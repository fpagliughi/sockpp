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

// --------------------------------------------------------------------------
// result<> (none / void) specialization
// --------------------------------------------------------------------------

TEST_CASE("result<> default constructor is success", "[result][none]") {
    result<> res;

    REQUIRE(res);
    REQUIRE(res.is_ok());
    REQUIRE(!res.is_error());
    REQUIRE(res.error() == error_code{});
}

TEST_CASE("result<> constructed from errc is error", "[result][none]") {
    const auto ERR = errc::permission_denied;
    result<> res{ERR};

    REQUIRE(!res);
    REQUIRE(res.is_error());
    REQUIRE(res.error() == ERR);
}

TEST_CASE("result<> from_error", "[result][none]") {
    const auto ERR = errc::timed_out;
    auto res = result<>::from_error(ERR);

    REQUIRE(!res);
    REQUIRE(res.error() == ERR);
}

// --------------------------------------------------------------------------
// Direct errc and error_code constructors
// --------------------------------------------------------------------------

TEST_CASE("result errc constructor", "[result]") {
    // Constructing result<int> directly from errc (not via from_error)
    const auto ERR = errc::no_such_file_or_directory;
    result<int> res{ERR};

    REQUIRE(!res);
    REQUIRE(res.is_error());
    REQUIRE(res.error() == ERR);
    REQUIRE(res.value() == int{});
}

TEST_CASE("result error_code constructor", "[result]") {
    const auto EC = std::make_error_code(errc::connection_refused);
    result<int> res{EC};

    REQUIRE(!res);
    REQUIRE(res.error() == EC);
    REQUIRE(res.error() == errc::connection_refused);
}

TEST_CASE("result from_error integer overload", "[result]") {
    // from_error(int, error_category) — platform-specific integer code
    const int CODE = ENOENT;
    auto res = result<int>::from_error(CODE, std::system_category());

    REQUIRE(!res);
    REQUIRE(res.error().value() == CODE);
    REQUIRE(&res.error().category() == &std::system_category());
}

// --------------------------------------------------------------------------
// value_or_throw / release_or_throw
// --------------------------------------------------------------------------

TEST_CASE("value_or_throw on success returns value", "[result]") {
    const int VAL = 99;
    result<int> res{VAL};

    REQUIRE(res.value_or_throw() == VAL);
}

TEST_CASE("value_or_throw on error throws system_error", "[result]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);

    REQUIRE_THROWS_AS(res.value_or_throw(), std::system_error);

    try {
        res.value_or_throw();
    }
    catch (const std::system_error& ex) {
        REQUIRE(ex.code() == ERR);
    }
}

TEST_CASE("release_or_throw on success moves value out", "[result]") {
    const int VAL = 42;
    result<moveable> res{moveable{VAL}};

    REQUIRE(res);
    auto val = res.release_or_throw();

    REQUIRE(val.val() == VAL);
    // The moved-from slot holds the default (zeroed) state
    REQUIRE(res.value().val() == 0);
}

TEST_CASE("release_or_throw on error throws system_error", "[result]") {
    const auto ERR = errc::bad_address;
    auto res = result<int>::from_error(ERR);

    REQUIRE_THROWS_AS(res.release_or_throw(), std::system_error);

    try {
        res.release_or_throw();
    }
    catch (const std::system_error& ex) {
        REQUIRE(ex.code() == ERR);
    }
}

// --------------------------------------------------------------------------
// error_message
// --------------------------------------------------------------------------

TEST_CASE("error_message on error returns non-empty string matching error", "[result]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);

    REQUIRE(!res.error_message().empty());
    REQUIRE(res.error_message() == res.error().message());
}

TEST_CASE("error_message on success matches error_code default message", "[result]") {
    result<int> res{7};

    REQUIRE(res.error_message() == error_code{}.message());
}

// --------------------------------------------------------------------------
// success() / error() free functions
// --------------------------------------------------------------------------

TEST_CASE("success() lvalue free function", "[result][free_functions]") {
    const int VAL = 55;
    auto res = success(VAL);

    REQUIRE(res);
    REQUIRE(res.value() == VAL);
}

TEST_CASE("success() rvalue free function", "[result][free_functions]") {
    auto res = success(moveable{77});

    REQUIRE(res);
    REQUIRE(res.value().val() == 77);
}

TEST_CASE("error() free function with errc", "[result][free_functions]") {
    const auto ERR = errc::resource_unavailable_try_again;
    auto res = error<int>(ERR);

    REQUIRE(!res);
    REQUIRE(res.error() == ERR);
}

TEST_CASE("error() free function with error_code", "[result][free_functions]") {
    const auto EC = std::make_error_code(errc::broken_pipe);
    auto res = error<int>(EC);

    REQUIRE(!res);
    REQUIRE(res.error() == EC);
}

TEST_CASE("error() free function with integer code", "[result][free_functions]") {
    const int CODE = EACCES;
    auto res = error<int>(CODE, std::system_category());

    REQUIRE(!res);
    REQUIRE(res.error().value() == CODE);
}

// --------------------------------------------------------------------------
// Reverse (commutative) comparison operators
// --------------------------------------------------------------------------

TEST_CASE("reverse value comparisons (val == result)", "[result][cmp]") {
    const int VAL = 42;
    result<int> res{VAL};

    // value on the left-hand side
    REQUIRE(42 == res);
    REQUIRE(!(42 != res));
    REQUIRE(29 != res);
    REQUIRE(!(29 == res));
}

TEST_CASE("reverse value comparisons fail for error result", "[result][cmp]") {
    auto res = result<int>::from_error(errc::interrupted);

    // An error result should never equal any value
    REQUIRE(!(0 == res));
    REQUIRE(0 != res);
    REQUIRE(!(42 == res));
    REQUIRE(42 != res);
}

TEST_CASE("reverse error_code comparisons (ec == result)", "[result][cmp]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);
    const auto EC = std::make_error_code(ERR);

    REQUIRE(EC == res);
    REQUIRE(!(EC != res));

    const auto OTHER_EC = std::make_error_code(errc::bad_address);
    REQUIRE(OTHER_EC != res);
    REQUIRE(!(OTHER_EC == res));
}

TEST_CASE("reverse errc comparisons (errc == result)", "[result][cmp]") {
    const auto ERR = errc::interrupted;
    auto res = result<int>::from_error(ERR);

    REQUIRE(ERR == res);
    REQUIRE(!(ERR != res));

    REQUIRE(errc::bad_address != res);
    REQUIRE(!(errc::bad_address == res));
}
