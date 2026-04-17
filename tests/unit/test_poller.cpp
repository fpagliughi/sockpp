// test_poller.cpp
//
// Unit tests for the `poller` class.
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

#include "catch2_version.h"
#include "sockpp/poller.h"

#if defined(_WIN32)
    #include "sockpp/tcp_acceptor.h"
    #include "sockpp/tcp_connector.h"
#else
    #include "sockpp/unix_stream_socket.h"
#endif

using namespace sockpp;
using namespace std::chrono;

// Helper: create a connected socket pair and assert success.
// On POSIX, uses a Unix-domain socketpair(); on Windows, uses a TCP
// loopback pair (AF_UNIX is not reliably available on Windows).
static auto make_pair() {
#if defined(_WIN32)
    tcp_acceptor acc{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(acc);
    auto addr = acc.address();
    tcp_connector cli{inet_address(INADDR_LOOPBACK, addr.port())};
    REQUIRE(cli);
    auto srv_res = acc.accept();
    REQUIRE(srv_res);
    return std::pair<stream_socket, stream_socket>{std::move(cli), srv_res.release()};
#else
    auto res = unix_stream_socket::pair();
    REQUIRE(res);
    auto [s1, s2] = res.release();
    return std::pair<stream_socket, stream_socket>{std::move(s1), std::move(s2)};
#endif
}

// --------------------------------------------------------------------------
// Construction
// --------------------------------------------------------------------------

TEST_CASE("poller default constructor", "[poller][construction]") {
    poller p;
    REQUIRE(p.empty());
    REQUIRE(p.size() == 0);
}

TEST_CASE("poller sized constructor", "[poller][construction]") {
    // Reserves capacity but does not register any sockets.
    poller p{4};
    REQUIRE(p.empty());
    REQUIRE(p.size() == 0);
}

TEST_CASE("poller single-socket constructor", "[poller][construction]") {
    auto [s1, s2] = make_pair();

    poller p{s2, poller::POLL_READ};
    REQUIRE(!p.empty());
    REQUIRE(p.size() == 1);
}

// --------------------------------------------------------------------------
// Registration: add / remove
// --------------------------------------------------------------------------

TEST_CASE("poller add and remove lifecycle", "[poller][registration]") {
    auto [s1a, s2a] = make_pair();
    auto [s1b, s2b] = make_pair();

    poller p;
    REQUIRE(p.empty());

    p.add(s2a);
    REQUIRE(p.size() == 1);

    p.add(s2b);
    REQUIRE(p.size() == 2);
    REQUIRE(!p.empty());

    p.remove(s2a);
    REQUIRE(p.size() == 1);

    p.remove(s2b);
    REQUIRE(p.empty());
}

TEST_CASE("poller remove unregistered socket is no-op", "[poller][registration]") {
    auto [s1a, s2a] = make_pair();
    auto [s1b, s2b] = make_pair();

    poller p;
    p.add(s2a);
    REQUIRE(p.size() == 1);

    // s2b was never added — should silently do nothing.
    p.remove(s2b);
    REQUIRE(p.size() == 1);
}

// --------------------------------------------------------------------------
// wait() behaviour
// --------------------------------------------------------------------------

TEST_CASE("poller wait on empty returns empty event list", "[poller][wait]") {
    poller p;

    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    REQUIRE(res.value().empty());
}

TEST_CASE("poller zero-timeout detects readable socket", "[poller][wait]") {
    auto [s1, s2] = make_pair();

    // Make s2 readable by writing from the other end.
    REQUIRE(s1.write("hello"));

    poller p{s2, poller::POLL_READ};
    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 1);
    REQUIRE((evts[0].events & poller::POLL_READ) != 0);
}

TEST_CASE("poller event sock pointer matches registered socket", "[poller][wait]") {
    auto [s1, s2] = make_pair();

    REQUIRE(s1.write("ping"));

    poller p{s2, poller::POLL_READ};
    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 1);
    // The non-owning pointer must refer back to exactly the registered object.
    REQUIRE(evts[0].sock == &s2);
}

TEST_CASE("poller zero-timeout on non-readable socket returns empty", "[poller][wait]") {
    auto [s1, s2] = make_pair();
    // Deliberately write nothing so s2 has no pending data.

    poller p{s2, poller::POLL_READ};
    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    REQUIRE(res.value().empty());
}

TEST_CASE("poller detects writable socket", "[poller][wait]") {
    auto [s1, s2] = make_pair();
    // A freshly connected socket always has kernel send-buffer space available.

    poller p{s1, poller::POLL_WRITE};
    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 1);
    REQUIRE((evts[0].events & poller::POLL_WRITE) != 0);
}

TEST_CASE("poller multiple sockets only readable one fires", "[poller][wait]") {
    auto [s1a, s2a] = make_pair();
    auto [s1b, s2b] = make_pair();

    // Write data through only the first pair.
    REQUIRE(s1a.write("data"));

    poller p;
    p.add(s2a, poller::POLL_READ);
    p.add(s2b, poller::POLL_READ);

    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 1);
    REQUIRE(evts[0].sock == &s2a);
}

TEST_CASE("poller POLL_READ_WRITE catches write readiness", "[poller][wait]") {
    auto [s1, s2] = make_pair();
    // Write nothing — s2 is writable but not readable.

    poller p{s2, poller::POLL_READ_WRITE};
    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 1);
    REQUIRE((evts[0].events & poller::POLL_WRITE) != 0);
}

TEST_CASE("poller duration template overload", "[poller][wait]") {
    auto [s1, s2] = make_pair();

    REQUIRE(s1.write("hi"));

    poller p{s2, poller::POLL_READ};
    // Use seconds (not milliseconds) to exercise the template overload.
    auto res = p.wait(seconds{0});

    REQUIRE(res);
    REQUIRE(!res.value().empty());
}

// --------------------------------------------------------------------------
// remove() index integrity
// --------------------------------------------------------------------------

TEST_CASE("poller remove middle socket preserves index integrity", "[poller][remove]") {
    auto [s1a, s2a] = make_pair();
    auto [s1b, s2b] = make_pair();
    auto [s1c, s2c] = make_pair();

    poller p;
    p.add(s2a, poller::POLL_READ);
    p.add(s2b, poller::POLL_READ);
    p.add(s2c, poller::POLL_READ);
    REQUIRE(p.size() == 3);

    // Write data to the first and third pairs, not the middle one.
    REQUIRE(s1a.write("a"));
    REQUIRE(s1c.write("c"));

    // Removing s2b must keep the socks_[] / pfds_[] arrays in sync.
    p.remove(s2b);
    REQUIRE(p.size() == 2);

    auto res = p.wait(milliseconds{0});

    REQUIRE(res);
    const auto& evts = res.value();
    REQUIRE(evts.size() == 2);

    bool saw_a = false, saw_c = false;
    for (const auto& e : evts) {
        if (e.sock == &s2a)
            saw_a = true;
        if (e.sock == &s2c)
            saw_c = true;
    }
    REQUIRE(saw_a);
    REQUIRE(saw_c);
}
