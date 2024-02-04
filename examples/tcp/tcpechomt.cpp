// tcpechomt.cpp
//
// TCP echo client that uses separate read and write threads.
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
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
#include <string>
#include <thread>

#include "sockpp/tcp_connector.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------
// The read thread will run independently, retrieving packets from the
// server and writing them to the console. When the main (write) thread
// shuts down the socket, we exit.

void read_thr(sockpp::tcp_socket rdSock) {
    char buf[512];

    while (true) {
        if (auto res = rdSock.read(buf, sizeof(buf)); !res || res.value() == 0) {
            cerr << "Read error: " << res.error_message() << endl;
            break;
        }
        else {
            cout.write(buf, res.value());
            cout << endl;
        }
    }

    rdSock.shutdown();
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "Sample multi-threaded TCP echo client for 'sockpp' " << sockpp::SOCKPP_VERSION
         << '\n'
         << endl;

    string host = (argc > 1) ? argv[1] : "localhost";
    in_port_t port = (argc > 2) ? atoi(argv[2]) : sockpp::TEST_PORT;

    sockpp::initialize();

    // Implicitly creates an inet_address from {host,port}
    // and then tries the connection.

    error_code ec;
    sockpp::tcp_connector conn({host, port}, ec);

    if (ec) {
        cerr << "Error connecting to server at " << sockpp::inet_address(host, port) << "\n\t"
             << ec.message() << endl;
        return 1;
    }

    cout << "Created a connection from " << conn.address() << endl;

    // We create a read thread and send it a clone (dup) of the
    // connector socket.

    auto cloneRes = conn.clone();
    if (!cloneRes) {
        cerr << "Error cloning socket: " << cloneRes.error_message() << endl;
        return 1;
    }

    std::thread rdThr(read_thr, std::move(cloneRes.release()));

    // The write loop get user input and writes it to the socket.

    string s, sret;
    while (getline(cin, s) && !s.empty()) {
        if (auto res = conn.write(s); !res || res != s.length()) {
            if (res == errc::broken_pipe)
                cerr << "It appears that the socket was closed." << endl;
            else
                cerr << "Error writing to the TCP stream :" << res.error_message() << endl;
            break;
        }
    }
    int ret = !conn ? 1 : 0;

    // Shutting down the socket will cause the read thread to exit.
    // We wait for it to exit before we leave the app.

    conn.shutdown(SHUT_WR);
    rdThr.join();

    return ret;
}
