// tlssvr.cpp
//
// Simple secure TLS echo server
//
// Usage:
//   tlssvr <cert.pem> <key.pem> [port]
//
// Accepts one TLS connection at a time, echoes each line back to the client,
// and loops until the client closes the connection.
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

// Handles one accepted TLS connection: echoes data until the peer closes.
static void handle_client(sockpp::tls_socket sock, const string& peer) {
    cout << "Connection from " << peer << endl;

    char buf[4096];
    while (true) {
        auto res = sock.read(buf, sizeof(buf));
        if (!res || res.value() == 0)
            break;
        if (auto wres = sock.write(buf, res.value()); !wres) {
            cerr << "Write error: " << wres.error_message() << endl;
            break;
        }
    }

    sock.send_close_notify();
    cout << "Connection from " << peer << " closed" << endl;
}

int main(int argc, char* argv[]) {
    cout << "Sample TLS server for 'sockpp' " << sockpp::SOCKPP_VERSION << "\n\n";

    if (argc < 3) {
        cerr << "Usage: tlssvr <cert.pem> <key.pem> [port]\n";
        return 1;
    }

    string cert_path = argv[1];
    string key_path = argv[2];
    in_port_t port = (argc > 3) ? static_cast<in_port_t>(atoi(argv[3])) : 4433;

    sockpp::initialize();

    // Build a server TLS context with the provided certificate and key.
    auto ctx = sockpp::tls_context::server();

    string cert = read_file(cert_path);
    string key = read_file(key_path);

    if (auto res = ctx.set_identity(cert, key); !res) {
        cerr << "Failed to load identity: " << res.error_message() << "\n";
        return 1;
    }

    // Bind and listen.
    error_code ec;
    sockpp::tls_acceptor acc{
        ctx, sockpp::inet_address(port), sockpp::acceptor::DFLT_QUE_SIZE, ec
    };
    if (ec) {
        cerr << "Failed to bind on port " << port << ": " << ec.message() << "\n";
        return 1;
    }

    cout << "Listening on port " << port << " ...\n";

    // Accept loop: one client at a time (no threading for simplicity).
    while (true) {
        sockpp::inet_address peer;
        auto res = acc.accept(&peer);
        if (!res) {
            cerr << "Accept error: " << res.error_message() << "\n";
            continue;
        }
        handle_client(res.release(), peer.to_string());
    }

    return 0;
}
