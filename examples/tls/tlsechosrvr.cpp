// tlsechosrvr.cpp
//
// A multi-threaded TLS echo server for sockpp.
// This is a thread-per-connection TLS server.
//
// USAGE:
//    tlsechosrvr <cert.pem> <key.pem> [port]
//
// The server loads a certificate and private key, then listens for TLS
// connections.  Each accepted connection is handled in a new thread that
// echoes data until the peer closes.  The TLS version and whether the
// session was resumed are logged for every accepted connection.
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

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>

#include "sockpp/inet_address.h"
#include "sockpp/tls/acceptor.h"
#include "sockpp/tls/context.h"
#include "sockpp/version.h"

using namespace std;

// Reads the entire contents of a file into a string.
static string read_file(const string& path) {
    ifstream f{path};
    return {istreambuf_iterator<char>{f}, istreambuf_iterator<char>{}};
}

// --------------------------------------------------------------------------
// The thread function.
// Ownership of the socket is transferred to the thread; when the function
// exits the socket is automatically closed and a close_notify is sent.

void run_echo(sockpp::tls_socket sock, const string& peer) {
    cout << "Connection from " << peer << " [" << sock.negotiated_version() << "]"
         << (sock.session_reused() ? " (resumed)" : "") << "\n";

    char buf[512];
    sockpp::result<size_t> res;

    while ((res = sock.read(buf, sizeof(buf))) && res.value() > 0)
        sock.write_n(buf, res.value());

    cout << "Connection from " << peer << " closed\n";
}

// --------------------------------------------------------------------------
// Main: bind, listen, and accept connections in a loop.
// Each accepted connection is handed off to a detached thread.

int main(int argc, char* argv[]) {
    cout << "TLS echo server for 'sockpp' " << sockpp::SOCKPP_VERSION << "\n" << endl;

    if (argc < 3) {
        cerr << "Usage: tlsechosrvr <cert.pem> <key.pem> [port]" << endl;
        return 1;
    }

    string cert_path = argv[1];
    string key_path = argv[2];
    in_port_t port = (argc > 3) ? static_cast<in_port_t>(atoi(argv[3])) : 4433;

    sockpp::initialize();

    // Build a server context: load identity and enable session caching so
    // clients can demonstrate session resumption.
    auto ctx = sockpp::tls_context::server();

    if (auto res = ctx.set_identity(read_file(cert_path), read_file(key_path)); !res) {
        cerr << "Failed to load identity: " << res.error_message() << endl;
        return 1;
    }

    ctx.set_session_cache_mode(sockpp::tls_context::session_cache_mode::SERVER);

    error_code ec;
    sockpp::tls_acceptor acc{
        ctx, sockpp::inet_address(port), sockpp::acceptor::DFLT_QUE_SIZE,
        sockpp::acceptor::REUSE, ec
    };
    if (ec) {
        cerr << "Error creating acceptor on port " << port << ": " << ec.message() << endl;
        return 1;
    }

    cout << "Listening on port " << port << " ...\n";

    while (true) {
        sockpp::inet_address peer;
        auto res = acc.accept(&peer);
        if (!res) {
            cerr << "Accept error: " << res.error_message() << endl;
            continue;
        }

        thread thr{run_echo, res.release(), peer.to_string()};
        thr.detach();
    }

    return 0;
}
