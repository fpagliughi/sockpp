// tcp6echo.cpp
//
// Simple TCP echo client
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2023 Frank Pagliughi
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

#include "sockpp/tcp6_connector.h"
#include "sockpp/version.h"

using namespace std;
using namespace std::chrono;

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "Sample IPv6 TCP echo client for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n'
         << endl;

    std::string host = (argc > 1) ? argv[1] : "::1";
    in_port_t port = (argc > 2) ? atoi(argv[2]) : sockpp::TEST_PORT;

    sockpp::initialize();

    // Try to resolve the address

    // Note that this works if the library was compiled with or without exceptions.
    // Applications normally only handles the exception or the return code.

    auto addrRes = sockpp::inet6_address::create(host, port);

    if (!addrRes) {
        cerr << "Error resolving address for '" << host << "':\n\t"
             << addrRes.error().message() << endl;
        return 1;
    }

    auto addr = addrRes.value();

    // TODO: Shouldn't this work?
    // sockpp::tcp6_connector conn(addr);

    sockpp::tcp6_connector conn;

    if (auto res = conn.connect(addr); !res) {
        cerr << "Error connecting to server at " << addr << "\n\t" << res.error_message()
             << endl;
        return 1;
    }

    cout << "Created a connection from " << conn.address() << endl;

    // Set a timeout for the responses
    if (auto res = conn.read_timeout(5s); !res) {
        cerr << "Error setting timeout on TCP stream: " << res.error_message() << endl;
    }

    string s, sret;
    while (getline(cin, s) && !s.empty()) {
        const size_t N = s.length();

        if (auto res = conn.write(s); res != N) {
            cerr << "Error writing to the TCP stream: " << res.error_message() << endl;
            break;
        }

        sret.resize(N);
        if (auto res = conn.read_n(&sret[0], N); res != N) {
            cerr << "Error reading from TCP stream: " << res.error_message() << endl;
            break;
        }

        cout << sret << endl;
    }

    return (!conn) ? 1 : 0;
}
