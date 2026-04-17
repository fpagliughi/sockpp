// test_socket.cpp
//
// Unit tests for the base `socket` class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019-2026 Frank Pagliughi
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
#include "sockpp/socket.h"

#if !defined(_WIN32)
    #include "sockpp/unix_stream_socket.h"
#endif

using namespace sockpp;
using namespace std::chrono;

/////////////////////////////////////////////////////////////////////////////
// Test aux functions

TEST_CASE("test to_timeval", "aux") {
    SECTION("concrete function") {
        timeval tv = to_timeval(microseconds(500));
        REQUIRE(tv.tv_sec == 0);
        REQUIRE(tv.tv_usec == 500);

        tv = to_timeval(microseconds(2500000));
        REQUIRE(tv.tv_sec == 2);
        REQUIRE(tv.tv_usec == 500000);
    }

    SECTION("template") {
        timeval tv = to_timeval(milliseconds(1));
        REQUIRE(tv.tv_sec == 0);
        REQUIRE(tv.tv_usec == 1000);

        tv = to_timeval(milliseconds(2500));
        REQUIRE(tv.tv_sec == 2);
        REQUIRE(tv.tv_usec == 500000);

        tv = to_timeval(seconds(5));
        REQUIRE(tv.tv_sec == 5);
        REQUIRE(tv.tv_usec == 0);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Test socket class

constexpr in_port_t INET_TEST_PORT = 12346;

TEST_CASE("socket constructors", "[socket]") {
    SECTION("default constructor") {
        sockpp::socket sock;

        REQUIRE(!sock);
        REQUIRE(!sock.is_open());
        REQUIRE(sock.handle() == INVALID_SOCKET);
    }

    SECTION("handle constructor") {
        constexpr auto HANDLE = socket_t(3);
        sockpp::socket sock(HANDLE);

        REQUIRE(sock);
        REQUIRE(sock.is_open());
        REQUIRE(sock.handle() == HANDLE);
    }

    SECTION("move constructor") {
        constexpr auto HANDLE = socket_t(3);
        sockpp::socket org_sock(HANDLE);

        sockpp::socket sock(std::move(org_sock));

        // Make sure the new socket got the handle
        REQUIRE(sock);
        REQUIRE(sock.handle() == HANDLE);

        // Make sure the handle was moved out of the org_sock
        REQUIRE(!org_sock);
        REQUIRE(org_sock.handle() == INVALID_SOCKET);
    }
}

// Test the socket error behavior
TEST_CASE("socket errors", "[socket]") {
    SECTION("basic errors") {
        sockpp::socket sock;
        REQUIRE(!sock);

        // Operations on an unopened socket should give an error
        int reuse = 1;
        socklen_t len = sizeof(int);
        auto res = sock.get_option(SOL_SOCKET, SO_REUSEADDR, &reuse, &len);
        REQUIRE(!res);
#if defined(_WIN32)
        REQUIRE(errc::not_a_socket == res);
#else
        REQUIRE(errc::bad_file_descriptor == res);
#endif
    }
}

TEST_CASE("socket handles", "[socket]") {
    constexpr auto HANDLE = socket_t(3);

    SECTION("test release") {
        sockpp::socket sock(HANDLE);

        REQUIRE(sock.handle() == HANDLE);
        REQUIRE(sock.release() == HANDLE);

        // Make sure the handle was moved out of the sock
        REQUIRE(!sock);
        REQUIRE(sock.handle() == INVALID_SOCKET);
    }

    SECTION("test reset") {
        sockpp::socket sock(HANDLE);
        REQUIRE(sock.handle() == HANDLE);

        sock.reset();  // Default reset acts like release w/o return

        // Make sure the handle was moved out of the sock
        REQUIRE(!sock);
        REQUIRE(sock.handle() == INVALID_SOCKET);

        // Now reset with a "valid" handle
        sock.reset(HANDLE);
        REQUIRE(sock);
        REQUIRE(sock.handle() == HANDLE);
    }
}

TEST_CASE("socket family", "[socket]") {
    SECTION("uninitialized socket") {
        // Uninitialized socket should have unspecified family
        sockpp::socket sock;
        REQUIRE(sock.family() == AF_UNSPEC);
    }

    SECTION("unbound socket") {
        // Unbound socket should have creation family
        auto res = socket::create(AF_INET, SOCK_STREAM);
        REQUIRE(res);
        auto sock = res.release();

// Windows and *nix behave differently
#if defined(_WIN32)
        REQUIRE(sock.family() == AF_UNSPEC);
#else
        REQUIRE(sock.family() == AF_INET);
#endif
    }

    SECTION("bound socket") {
        // Bound socket should have same family as
        // address to which it's bound
        auto res = socket::create(AF_INET, SOCK_STREAM);
        REQUIRE(res);
        auto sock = res.release();

        inet_address addr(INET_TEST_PORT);

        int reuse = 1;
        REQUIRE(sock.set_option(SOL_SOCKET, SO_REUSEADDR, reuse));
        REQUIRE(sock.bind(addr));
        REQUIRE(sock.family() == addr.family());
    }
}

TEST_CASE("socket address", "[socket]") {
    SECTION("uninitialized socket") {
        // Uninitialized socket should have empty address
        sockpp::socket sock;
        REQUIRE(sock.address() == sock_address_any{});
    }

    // The address has the specified family but all zeros
    SECTION("unbound socket") {
        auto sock = socket::create(AF_INET, SOCK_STREAM).release();
        auto addr = inet_address(sock.address());

// Windows and *nix behave differently for family
#if defined(_WIN32)
        REQUIRE(sock.family() == AF_UNSPEC);
#else
        REQUIRE(sock.family() == AF_INET);
#endif

        REQUIRE(addr.address() == 0);
        REQUIRE(addr.port() == 0);
    }

    SECTION("bound socket") {
        // Bound socket should have same family as
        // address to which it's bound
        auto sock = socket::create(AF_INET, SOCK_STREAM).release();
        const inet_address ADDR(INET_TEST_PORT);

        int reuse = 1;
        REQUIRE(sock.set_option(SOL_SOCKET, SO_REUSEADDR, reuse));

        REQUIRE(sock.bind(ADDR));
        REQUIRE(sock.address() == ADDR);
    }
}

// Socket pair shouldn't work for TCP sockets on any known platform.
// So this should fail, but fail gracefully and retain the error
// in both sockets.
TEST_CASE("failed socket pair", "[socket]") {
    auto res = socket::pair(AF_INET, SOCK_STREAM);
    REQUIRE(!res);
}

// Test putting the socket into and out of non-blocking mode
TEST_CASE("socket non-blocking mode", "[socket]") {
    auto res = socket::create(AF_INET, SOCK_STREAM);
    REQUIRE(res);
    auto sock = res.release();

#if !defined(_WIN32)
    REQUIRE(!sock.is_non_blocking());
#endif

    REQUIRE(sock.set_non_blocking());
#if !defined(_WIN32)
    REQUIRE(sock.is_non_blocking());
#endif

    REQUIRE(sock.set_non_blocking(false));
#if !defined(_WIN32)
    REQUIRE(!sock.is_non_blocking());
#endif
}

// --------------------------------------------------------------------------
// Aux function: to_duration / to_timepoint
// --------------------------------------------------------------------------

TEST_CASE("test to_duration", "[socket][aux]") {
    SECTION("zero timeval") {
        timeval tv{0, 0};
        REQUIRE(to_duration(tv) == microseconds(0));
    }

    SECTION("seconds only") {
        timeval tv{3, 0};
        REQUIRE(to_duration(tv) == seconds(3));
    }

    SECTION("microseconds only") {
        timeval tv{0, 750000};
        REQUIRE(to_duration(tv) == microseconds(750000));
    }

    SECTION("seconds and microseconds") {
        timeval tv{2, 500000};
        REQUIRE(to_duration(tv) == microseconds(2500000));
    }

    SECTION("round-trip with to_timeval") {
        const auto orig = microseconds(1234567);
        REQUIRE(to_duration(to_timeval(orig)) == orig);
    }
}

TEST_CASE("test to_timepoint", "[socket][aux]") {
    SECTION("epoch") {
        timeval tv{0, 0};
        auto tp = to_timepoint(tv);
        REQUIRE(tp.time_since_epoch().count() == 0);
    }

    SECTION("one second past epoch") {
        timeval tv{1, 0};
        auto tp = to_timepoint(tv);
        auto secs = duration_cast<seconds>(tp.time_since_epoch());
        REQUIRE(secs.count() == 1);
    }
}

// --------------------------------------------------------------------------
// Domain constructors
// --------------------------------------------------------------------------

TEST_CASE("socket domain constructors", "[socket]") {
    SECTION("throwing constructor valid domain") {
        sockpp::socket sock(AF_INET, SOCK_STREAM);
        REQUIRE(sock.is_open());
    }

    SECTION("throwing constructor invalid domain") {
        REQUIRE_THROWS_AS(sockpp::socket(AF_UNSPEC, SOCK_STREAM), std::system_error);
    }

    SECTION("error_code constructor valid domain") {
        error_code ec;
        sockpp::socket sock(AF_INET, SOCK_STREAM, ec);
        REQUIRE(!ec);
        REQUIRE(sock.is_open());
    }

    SECTION("error_code constructor invalid domain") {
        error_code ec;
        sockpp::socket sock(AF_UNSPEC, SOCK_STREAM, ec);
        REQUIRE(ec);
        REQUIRE(!sock.is_open());
    }
}

// --------------------------------------------------------------------------
// Move assignment
// --------------------------------------------------------------------------

TEST_CASE("socket move assignment", "[socket]") {
    auto res = socket::create(AF_INET, SOCK_STREAM);
    REQUIRE(res);
    auto src = res.release();
    const auto h = src.handle();

    sockpp::socket dst;
    REQUIRE(!dst.is_open());

    dst = std::move(src);

    REQUIRE(dst.is_open());
    REQUIRE(dst.handle() == h);
    REQUIRE(!src.is_open());
    REQUIRE(src.handle() == INVALID_SOCKET);
}

// --------------------------------------------------------------------------
// close()
// --------------------------------------------------------------------------

TEST_CASE("socket close", "[socket]") {
    SECTION("close open socket succeeds") {
        auto sock = socket::create(AF_INET, SOCK_STREAM).release();
        REQUIRE(sock.is_open());
        REQUIRE(sock.close());
        REQUIRE(!sock.is_open());
    }

    SECTION("close already-closed socket is a no-op") {
        sockpp::socket sock;
        REQUIRE(!sock.is_open());
        // Should succeed silently (nothing to close)
        REQUIRE(sock.close());
    }
}

// --------------------------------------------------------------------------
// clone()
// --------------------------------------------------------------------------

TEST_CASE("socket clone", "[socket]") {
    auto sock = socket::create(AF_INET, SOCK_STREAM).release();
    REQUIRE(sock.is_open());

    auto res = sock.clone();
    REQUIRE(res);

    auto dup = res.release();

    REQUIRE(dup.is_open());
    // The duplicate must be a distinct file descriptor.
    REQUIRE(dup.handle() != sock.handle());

    // Closing one does not invalidate the other.
    REQUIRE(sock.close());
    REQUIRE(!sock.is_open());
    REQUIRE(dup.is_open());

    // The duplicate should still be usable.
    // On Windows, getsockname() on an unbound socket returns AF_UNSPEC.
#if defined(_WIN32)
    REQUIRE(dup.family() == AF_UNSPEC);
#else
    REQUIRE(dup.family() == AF_INET);
#endif
}

// --------------------------------------------------------------------------
// peer_address()
// --------------------------------------------------------------------------

TEST_CASE("socket peer_address", "[socket]") {
    SECTION("unconnected socket returns empty address") {
        auto sock = socket::create(AF_INET, SOCK_STREAM).release();
        REQUIRE(sock.peer_address() == sock_address_any{});
    }

#if !defined(_WIN32)
    SECTION("connected socket returns non-empty address") {
        // socket::pair gives a connected pair of unnamed Unix sockets.
        auto res = socket::pair(AF_UNIX, SOCK_STREAM);
        REQUIRE(res);
        auto [s1, s2] = res.release();

        // An unnamed socket peer has AF_UNIX family even though the path is empty.
        REQUIRE(s1.peer_address().family() == AF_UNIX);
        REQUIRE(s2.peer_address().family() == AF_UNIX);
    }
#endif
}

// --------------------------------------------------------------------------
// Socket options
// --------------------------------------------------------------------------

TEST_CASE("socket reuse_address", "[socket][options]") {
    auto sock = socket::create(AF_INET, SOCK_STREAM).release();

    REQUIRE(sock.reuse_address(true));
    auto res = sock.reuse_address();
    REQUIRE(res);
    REQUIRE(res.value() == true);

// macOS treats SO_REUSEADDR as a one-way latch: once set it cannot be cleared.
#if !defined(__APPLE__)
    REQUIRE(sock.reuse_address(false));
    res = sock.reuse_address();
    REQUIRE(res);
    REQUIRE(res.value() == false);
#endif
}

// SO_REUSEPORT round-trip is only meaningful on Linux.  On macOS/BSD,
// setsockopt(SO_REUSEPORT) on an unbound SOCK_STREAM socket may succeed but
// getsockopt reads back 0 — the option is silently ignored, making the
// round-trip unreliable.  Windows/Cygwin don't expose the option at all.
#if defined(__linux__)
TEST_CASE("socket reuse_port", "[socket][options]") {
    auto sock = socket::create(AF_INET, SOCK_STREAM).release();

    REQUIRE(sock.reuse_port(true));
    auto res = sock.reuse_port();
    REQUIRE(res);
    REQUIRE(res.value() == true);
}
#endif

TEST_CASE("socket buffer sizes", "[socket][options]") {
    auto sock = socket::create(AF_INET, SOCK_STREAM).release();

    SECTION("recv_buffer_size") {
        // Default size is system-defined but must be positive.
        auto res = sock.recv_buffer_size();
        REQUIRE(res);
        REQUIRE(res.value() > 0);

        // Setting a size succeeds (kernel may round up, so just check no error).
        REQUIRE(sock.recv_buffer_size(16384));
        REQUIRE(sock.recv_buffer_size());
    }

    SECTION("send_buffer_size") {
        auto res = sock.send_buffer_size();
        REQUIRE(res);
        REQUIRE(res.value() > 0);

        REQUIRE(sock.send_buffer_size(16384));
        REQUIRE(sock.send_buffer_size());
    }
}

TEST_CASE("socket timeouts", "[socket][options]") {
    auto sock = socket::create(AF_INET, SOCK_STREAM).release();

    SECTION("read_timeout succeeds") {
        REQUIRE(sock.read_timeout(milliseconds(100)));
    }

    SECTION("write_timeout succeeds") {
        REQUIRE(sock.write_timeout(milliseconds(200)));
    }

#if !defined(_WIN32)
    SECTION("read_timeout causes recv to return an error") {
        auto res = socket::pair(AF_UNIX, SOCK_STREAM);
        REQUIRE(res);
        auto [s1, s2] = res.release();

        // Set a very short timeout; no data will arrive.
        REQUIRE(s2.read_timeout(milliseconds(1)));

        char buf[1];
        auto recv_res = s2.recv(buf, sizeof(buf));

        REQUIRE(!recv_res);
        // Linux reports EAGAIN; some platforms report ETIMEDOUT.
        REQUIRE((recv_res == errc::resource_unavailable_try_again ||
                 recv_res == errc::timed_out ||
                 recv_res == errc::operation_would_block));
    }
#endif
}

// --------------------------------------------------------------------------
// shutdown()
// --------------------------------------------------------------------------

TEST_CASE("socket shutdown", "[socket]") {
    SECTION("shutdown on invalid socket fails") {
        sockpp::socket sock;
        REQUIRE(!sock.shutdown(SHUT_RDWR));
    }

#if !defined(_WIN32)
    SECTION("shutdown SHUT_WR stops sends, peer reads EOF") {
        auto res = socket::pair(AF_UNIX, SOCK_STREAM);
        REQUIRE(res);
        auto [s1, s2] = res.release();

        REQUIRE(s1.shutdown(SHUT_WR));

        // After the writer shuts down, the reader gets EOF (0 bytes).
        char buf[4];
        auto recv_res = s2.recv(buf, sizeof(buf));
        REQUIRE(recv_res);
        REQUIRE(recv_res.value() == 0);
    }

    SECTION("shutdown SHUT_RDWR succeeds on valid socket") {
        auto res = socket::pair(AF_UNIX, SOCK_STREAM);
        REQUIRE(res);
        auto [s1, s2] = res.release();

        REQUIRE(s1.shutdown(SHUT_RDWR));
    }
#endif
}

// --------------------------------------------------------------------------

// Test that the "last error" call to a socket gives the proper result
// for the current thread.
// Here we share a socket across two threads, force an error in one
// thread, and then check to make sure that the error did not propagate
// to the other thread.
//
#if 0
TEST_CASE("thread-safe last error", "[socket]") {
	sockpp::socket sock;

	int state = 0;
	std::mutex m;
	std::condition_variable cv;

	std::thread thr([&] {
		// Test #1

		{
			// Wait for Test #2
			std::unique_lock<std::mutex> lk(m);
			state = 1;
			cv.notify_one();
			cv.wait(lk, [&state]{return state >= 2;});
		}

		// Test #3

	});

	{
		// Wait for Test #1
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [&state]{return state >= 1;});
	}

	// Test #2
	// Setting options on an un-opened socket should generate an error
	int reuse = 1;
	socklen_t len = sizeof(int);
	bool ok = sock.get_option(SOL_SOCKET, SO_REUSEADDR, &reuse, &len);

	REQUIRE(!ok);
	REQUIRE(sock.last_error());

	{
		std::unique_lock<std::mutex> lk(m);
		state = 2;
		cv.notify_one();
	}
	thr.join();
}
#endif
