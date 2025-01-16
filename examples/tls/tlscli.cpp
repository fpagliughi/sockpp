// tlscli.cpp
//
// Simple secure TLS client
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi
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

#include "sockpp/inet_address.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"
#include "sockpp/tls/error.h"
#include "sockpp/version.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "Sample TLS client for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n' << endl;

    string host = (argc > 1) ? argv[1] : "example.org";
    in_port_t port = (argc > 2) ? atoi(argv[2]) : 443;
    string trustStore = (argc > 3) ? argv[3] : "";

    const string REQUEST = string{"GET / HTTP/1.0\r\nHost: "} + host + "\r\n\r\n";

    sockpp::initialize();

    // Different ways to make a client context:
    //   sockpp::tls_context ctx(sockpp::tls_context::role_t::CLIENT);
    //   sockpp::tls_client_context ctx;
    //   auto ctx = sockpp::tls_context::client();

    auto ctx = sockpp::tls_context_builder::client().verify_peer().finalize();

    if (trustStore.empty())
        ctx.set_default_trust_locations();
    else
        ctx.set_trust_file(trustStore);

    // Creates an inet_address from {host,port} and then try the connection.

    error_code ec;

    sockpp::inet_address addr{host, port, ec};
    if (ec) {
        cerr << "Error resolving address: " << host << ":" << port << "\n\t" << ec.message()
             << endl;
        return 1;
    }

    sockpp::tls_connector conn{ctx, addr, ec};

    if (ec) {
        cerr << "Error connecting to server: " << ec.message() << endl;
        return 1;
    }

    // Send the request

    if (auto res = conn.write(REQUEST); !res) {
        cerr << "Error sending request [0x" << hex << res.error().value()
             << "]: " << res.error_message() << endl;
        return 1;
    }

    cout << "Wrote the request..." << endl;

    // Read the result

    while (!conn.received_shutdown()) {
        char buf[512];

        if (auto res = conn.read(buf, sizeof(buf)); !res) {
            cerr << "Error: " << res.error_message() << endl;
            break;
        }
        else if (res.value() > 0) {
            string s(buf, res.value());
            cout << s << flush;
        }
        else {
            break;
        }
    }
    cout << endl;

    return 0;
}
