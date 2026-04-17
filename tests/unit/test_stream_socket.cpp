// test_stream_socket.cpp
//
// Unit tests for the `stream_socket` class(es).
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

#include <cstring>
#include <string>

#include "catch2_version.h"
#include "sockpp/inet_address.h"
#include "sockpp/stream_socket.h"
#include "sockpp/tcp_acceptor.h"
#include "sockpp/tcp_connector.h"

using namespace std;
using namespace sockpp;

// --------------------------------------------------------------------------
// Helper: create a connected TCP loopback pair.
// The acceptor binds to port 0 so the OS picks a free port.
// --------------------------------------------------------------------------

static auto make_tcp_pair() {
    tcp_acceptor acc{inet_address(INADDR_LOOPBACK, 0)};
    REQUIRE(acc);
    auto addr = acc.address();
    tcp_connector cli{inet_address(INADDR_LOOPBACK, addr.port())};
    REQUIRE(cli);
    auto srv_res = acc.accept();
    REQUIRE(srv_res);
    return std::pair<stream_socket, stream_socket>{std::move(cli), srv_res.release()};
}

// --------------------------------------------------------------------------
// Construction
// --------------------------------------------------------------------------

TEST_CASE("stream_socket default constructor", "[stream_socket]") {
    stream_socket sock;
    REQUIRE(!sock);
    REQUIRE(!sock.is_open());
}

TEST_CASE("stream_socket handle constructor", "[stream_socket]") {
    constexpr auto HANDLE = socket_t(3);

    SECTION("valid handle") {
        stream_socket sock(HANDLE);
        REQUIRE(sock);
        REQUIRE(HANDLE == sock.handle());
    }

    SECTION("invalid handle") {
        stream_socket sock(INVALID_SOCKET);
        REQUIRE(!sock);
    }
}

// --------------------------------------------------------------------------
// I/O on invalid socket
// --------------------------------------------------------------------------

TEST_CASE("stream_socket write on invalid socket fails", "[stream_socket][io]") {
    stream_socket sock;
    REQUIRE(!sock);

    const string MSG{"hello"};
    auto res = sock.write(MSG);

    REQUIRE(!res);
}

TEST_CASE("stream_socket read on invalid socket fails", "[stream_socket][io]") {
    stream_socket sock;
    REQUIRE(!sock);

    char buf[8];
    auto res = sock.read(buf, sizeof(buf));

    REQUIRE(!res);
}

// --------------------------------------------------------------------------
// Connected I/O
// --------------------------------------------------------------------------

TEST_CASE("stream_socket write and read", "[stream_socket][io]") {
    auto [writer, reader] = make_tcp_pair();

    const string MSG{"Hello, world!"};

    auto wres = writer.write(MSG);
    REQUIRE(wres);
    REQUIRE(wres.value() == MSG.size());

    char buf[64]{};
    auto rres = reader.read(buf, MSG.size());
    REQUIRE(rres);
    REQUIRE(rres.value() == MSG.size());
    REQUIRE(string(buf, MSG.size()) == MSG);
}

TEST_CASE("stream_socket write_n and read_n", "[stream_socket][io]") {
    auto [writer, reader] = make_tcp_pair();

    const string MSG{"Guaranteed full write and read"};

    auto wres = writer.write_n(MSG.data(), MSG.size());
    REQUIRE(wres);
    REQUIRE(wres.value() == MSG.size());

    char buf[64]{};
    auto rres = reader.read_n(buf, MSG.size());
    REQUIRE(rres);
    REQUIRE(rres.value() == MSG.size());
    REQUIRE(string(buf, MSG.size()) == MSG);
}

TEST_CASE("stream_socket write string overload", "[stream_socket][io]") {
    auto [writer, reader] = make_tcp_pair();

    const string MSG{"string overload test"};

    // write(const string&) is the convenience overload
    auto wres = writer.write(MSG);
    REQUIRE(wres);
    REQUIRE(wres.value() == MSG.size());

    char buf[64]{};
    auto rres = reader.read(buf, MSG.size());
    REQUIRE(rres);
    REQUIRE(string(buf, MSG.size()) == MSG);
}

TEST_CASE("stream_socket EOF on closed writer", "[stream_socket][io]") {
    auto [writer, reader] = make_tcp_pair();

    // Closing the writer causes the reader to get EOF (0-byte read).
    writer.close();

    char buf[8];
    auto rres = reader.read(buf, sizeof(buf));
    REQUIRE(rres);
    REQUIRE(rres.value() == 0);
}

// --------------------------------------------------------------------------
// clone()
// --------------------------------------------------------------------------

TEST_CASE("stream_socket clone", "[stream_socket]") {
    auto sock = stream_socket::create(AF_INET).release();
    REQUIRE(sock.is_open());

    auto res = sock.clone();
    REQUIRE(res);

    auto dup = res.release();
    REQUIRE(dup.is_open());
    REQUIRE(dup.handle() != sock.handle());
}
