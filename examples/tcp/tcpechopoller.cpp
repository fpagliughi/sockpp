// tcpechopoller.cpp
//
// A single-threaded TCP echo server using a poller.
//
// A poller watches the acceptor and all active connections. When the
// acceptor becomes readable a new connection is accepted and added to
// the poller. When a connection becomes readable the data is echoed back.
// A zero-length read means the remote end closed, so the connection is
// removed from the poller and destroyed.
//
// USAGE:
//      tcpechopoller [port]
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

#include <algorithm>
#include <iostream>
#include <list>

#include "sockpp/poller.h"
#include "sockpp/tcp_acceptor.h"
#include "sockpp/version.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "Single-threaded TCP echo server (poller) for 'sockpp' " << sockpp::SOCKPP_VERSION
         << '\n'
         << endl;

    in_port_t port = (argc > 1) ? in_port_t(atoi(argv[1])) : sockpp::TEST_PORT;

    sockpp::initialize();

    error_code ec;
    sockpp::tcp_acceptor acc{
        port, sockpp::tcp_acceptor::DFLT_QUE_SIZE, sockpp::tcp_acceptor::REUSE, ec
    };

    if (ec) {
        cerr << "Error creating the acceptor: " << ec.message() << endl;
        return 1;
    }
    cout << "Awaiting connections on port " << port << "..." << endl;

    // The list owns the active connections. std::list gives stable addresses
    // so the poller's non-owning pointers remain valid across insertions and
    // removals.
    list<sockpp::tcp_socket> connections;

    // Start the poller watching the acceptor for incoming connections.
    sockpp::poller poller(acc, sockpp::poller::POLL_IN);

    while (true) {
        auto res = poller.wait();
        if (!res) {
            cerr << "Poll error: " << res.error_message() << endl;
            break;
        }

        for (auto& ev : res.value()) {
            if (ev.sock == &acc) {
                // The acceptor is readable — a new connection is waiting.
                sockpp::inet_address peer;
                if (auto connRes = acc.accept(&peer); connRes) {
                    cout << "Incoming connection from " << peer << endl;
                    auto& sock = connections.emplace_back(connRes.release());
                    poller.add(sock, sockpp::poller::POLL_IN);
                }
                else {
                    cerr << "Error accepting connection: " << connRes.error_message() << endl;
                }
            }
            else {
                // An existing connection is readable.
                auto* sock = static_cast<sockpp::stream_socket*>(ev.sock);
                char buf[512];

                if (auto n = sock->read(buf, sizeof(buf)); !n || n.value() == 0) {
                    // Zero bytes = EOF; error also means the connection is gone.
                    auto it = find_if(
                        connections.begin(), connections.end(),
                        [sock](const auto& s) { return &s == sock; }
                    );
                    if (it != connections.end()) {
                        cout << "Connection closed from " << it->peer_address() << endl;
                        poller.remove(*sock);
                        connections.erase(it);
                    }
                }
                else {
                    sock->write_n(buf, n.value());
                }
            }
        }
    }

    return 0;
}
