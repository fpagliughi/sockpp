// tcp6echosvr.cpp
//
// A multi-threaded TCP v6 echo server for sockpp library.
// This is a simple thread-per-connection TCP server for IPv6.
//
// USAGE:
//  	tcp6echosvr [port]
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019-2023 Frank Pagliughi
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

#include <iostream>
#include <thread>

#include "sockpp/tcp6_acceptor.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------
// The thread function. This is run in a separate thread for each socket.
// Ownership of the socket object is transferred to the thread, so when this
// function exits, the socket is automatically closed.

void run_echo(sockpp::tcp6_socket sock) {
    char buf[512];
    sockpp::result<size_t> res;

    while ((res = sock.read(buf, sizeof(buf))) && res.value() > 0)
        sock.write_n(buf, res.value());

    cout << "Connection closed from " << sock.peer_address() << endl;
}

// --------------------------------------------------------------------------
// The main thread runs the TCP port acceptor. Each time a connection is
// made a new thread is spawned to handle it leaving this main thread to
// immediately wait for the next connection.

int main(int argc, char* argv[]) {
    cout << "Sample IPv6 TCP echo server for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n'
         << endl;

    in_port_t port = (argc > 1) ? atoi(argv[1]) : sockpp::TEST_PORT;

    sockpp::initialize();

    error_code ec;
    sockpp::tcp6_acceptor acc(port, 4, ec);

    if (ec) {
        cerr << "Error creating the acceptor: " << ec.message() << endl;
        return 1;
    }
    cout << "Awaiting connections on port " << port << "..." << endl;

    while (true) {
        sockpp::inet6_address peer;

        // Accept a new client connection
        if (auto res = acc.accept(&peer); !res) {
            cerr << "Error accepting incoming connection: " << res.error_message() << endl;
        }
        else {
            cout << "Received a connection request from " << peer << endl;
            sockpp::tcp6_socket sock = res.release();

            // Create a thread and transfer the new stream to it.
            thread thr(run_echo, std::move(sock));
            thr.detach();
        }
    }

    return 0;
}
