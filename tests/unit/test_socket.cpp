// test_inet_address.cpp
//
// Unit tests for the `inet_address` class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2018 Frank Pagliughi
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
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace sockpp;

// Test that the "last error" call to a socket gives the proper result
// for the current thread.
// Here we share a socket across two threads, force an error in one
// thread, and then check to make sure that the error did not propagate
// to the other thread.
//
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

