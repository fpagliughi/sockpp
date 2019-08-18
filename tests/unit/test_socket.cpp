// test_socket.cpp
//
// Unit tests for the base `socket` class.
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

#include "catch2/catch.hpp"
#include "sockpp/socket.h"
#include <string>
/*
#include <thread>
#include <mutex>
#include <condition_variable>
*/

using namespace sockpp;

TEST_CASE("socket constructors", "[socket]") {
	SECTION("default constructor") {
		sockpp::socket sock;

		REQUIRE(!sock);
		REQUIRE(!sock.is_open());
		REQUIRE(sock.handle() == INVALID_SOCKET);
		REQUIRE(sock.last_error() == 0);
	}

	SECTION("handle constructor") {
		constexpr auto HANDLE = socket_t(3);
		sockpp::socket sock(HANDLE);

		REQUIRE(sock);
		REQUIRE(sock.is_open());
		REQUIRE(sock.handle() == HANDLE);
		REQUIRE(sock.last_error() == 0);
	}


	SECTION("move constructor") {
		constexpr auto HANDLE = socket_t(3);
		sockpp::socket org_sock(HANDLE);

		sockpp::socket sock(std::move(org_sock));

		// Make sure the new socket got the handle
		REQUIRE(sock);
		REQUIRE(sock.handle() == HANDLE);
		REQUIRE(sock.last_error() == 0);

		// Make sure the handle was moved out of the org_sock
		REQUIRE(!org_sock);
		REQUIRE(org_sock.handle() == INVALID_SOCKET);
	}
}

// Test the socket error behavior
TEST_CASE("socket errors", "[socket]") {
	sockpp::socket sock;

	// Operations on an unopened socket should give an error
	int reuse = 1;
	socklen_t len = sizeof(int);
	bool ok = sock.get_option(SOL_SOCKET, SO_REUSEADDR, &reuse, &len);

	// Socket should be in error state
	REQUIRE(!ok);
	REQUIRE(!sock);

	int err = sock.last_error();
	REQUIRE(err != 0);

	// last_error() is sticky, unlike `errno`
	REQUIRE(sock.last_error() == err);

	// We can clear the error
	sock.clear();
	REQUIRE(sock.last_error() == 0);

	// Test arbitrary clear value
	sock.clear(42);
	REQUIRE(sock.last_error() == 42);
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

		sock.reset();	// Default reset acts like release w/o return

		// Make sure the handle was moved out of the sock
		REQUIRE(!sock);
		REQUIRE(sock.handle() == INVALID_SOCKET);

		// Now reset with a "valid" handle
		sock.reset(HANDLE);
		REQUIRE(sock);
		REQUIRE(sock.handle() == HANDLE);
	}
}

// Socket pair shouldn't work for TCP sockets on any known platform.
// So this should fail, but fail gracefully and retain the error
// in both sockets.
TEST_CASE("failed socket pair", "[socket]") {
	sockpp::socket sock1, sock2;
	std::tie(sock1, sock2) = std::move(socket::pair(AF_INET, SOCK_STREAM));

	REQUIRE(!sock1);
	REQUIRE(!sock2);

	REQUIRE(sock1.last_error() != 0);
	REQUIRE(sock1.last_error() == sock2.last_error());
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
		REQUIRE(sock.last_error() == 0);
		{
			// Wait for Test #2
			std::unique_lock<std::mutex> lk(m);
			state = 1;
			cv.notify_one();
			cv.wait(lk, [&state]{return state >= 2;});
		}

		// Test #3
		REQUIRE(sock.last_error() == 0);
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
	REQUIRE(sock.last_error() != 0);

	{
		std::unique_lock<std::mutex> lk(m);
		state = 2;
		cv.notify_one();
	}
	thr.join();
}
#endif

