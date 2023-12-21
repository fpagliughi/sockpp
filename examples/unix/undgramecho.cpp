// udpecho.cpp
//
// Simple Unix-domain UDP echo client
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

#include <iostream>
#include <string>

#include "sockpp/unix_dgram_socket.h"

using namespace std;

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    sockpp::initialize();

    string cliAddr{"/tmp/undgramecho.sock"}, svrAddr{"/tmp/undgramechosvr.sock"};

    sockpp::unix_dgram_socket sock;

    // A Unix-domain UDP client needs to bind to its own address
    // before it can send or receive packets

    if (auto res = sock.bind(sockpp::unix_address(cliAddr)); !res) {
        cerr << "Error connecting to client address at '" << cliAddr << "'"
             << "\n\t" << res.error_message() << endl;
        return 1;
    }

    // "Connect" to the server address. This is a convenience to set the
    // default 'send_to' address, as there is no real connection.

    if (auto res = sock.connect(sockpp::unix_address(svrAddr)); !res) {
        cerr << "Error connecting to server at '" << svrAddr << "'"
             << "\n\t" << res.error_message() << endl;
        return 1;
    }

    cout << "Created UDP socket at: " << sock.address() << endl;

    string s, sret;
    while (getline(cin, s) && !s.empty()) {
        const size_t N = s.length();

        if (auto res = sock.send(s); res != N) {
            cerr << "Error writing to the UDP socket: " << res.error_message() << endl;
            break;
        }

        sret.resize(N);
        if (auto res = sock.recv(&sret[0], N); res != N) {
            cerr << "Error reading from UDP socket: " << res.error_message() << endl;
            break;
        }
    }

    cout << sret << endl;
    return (!sock) ? 1 : 0;
}
