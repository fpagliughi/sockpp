// tlsecho.cpp
//
// A TLS echo client that demonstrates session resumption.
//
// USAGE:
//    tlsecho [[host] port [ca.pem]]
//
// The host defaults to "localhost".  If the first argument is a bare
// integer it is taken as the port number (host keeps its default), so
// both of the following work:
//
//    tlsecho 4433 ca.pem
//    tlsecho myserver 4433 ca.pem
//
// Reads lines from stdin and sends them to the echo server, printing the
// echoed response.  An empty line closes the connection.
//
// When built with OpenSSL, the client saves the session from the first
// connection and offers it on the second, demonstrating TLS session
// resumption.  The handshake type (full or resumed) is printed for every
// connection so the speed-up is easy to observe.
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

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <system_error>

#include "sockpp/inet_address.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"
#include "sockpp/version.h"

#if defined(SOCKPP_OPENSSL)
    #include "sockpp/tls/openssl_session.h"
#endif

using namespace std;

// Runs the echo loop on an already-connected TLS socket.
// Returns when the user types an empty line or stdin reaches EOF.
static bool run_echo_loop(sockpp::tls_connector& conn) {
    string s, sret;

    while (getline(cin, s) && !s.empty()) {
        if (auto res = conn.write(s); !res) {
            cerr << "Write error: " << res.error_message() << "\n";
            return false;
        }

        sret.resize(s.size());
        if (auto res = conn.read_n(sret.data(), s.size()); !res || res.value() != s.size()) {
            cerr << "Read error: " << (!res ? res.error_message() : "connection closed")
                 << "\n";
            return false;
        }

        cout << sret << "\n";
    }

    return true;
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "TLS echo client for 'sockpp' " << sockpp::SOCKPP_VERSION << "\n" << endl;

    // Argument parsing: if argv[1] is a bare integer treat it as the port
    // (host defaults to "localhost"), otherwise argv[1] is the host.
    // This lets both "tlsecho 4433 ca.pem" and "tlsecho myhost 4433 ca.pem" work.
    string host = "localhost";
    in_port_t port = 4433;
    string ca_file;

    int next = 1;
    if (argc > next) {
        const char* a = argv[next];
        // Purely numeric first arg → port number, host stays "localhost"
        bool is_port = (*a != '\0' && strspn(a, "0123456789") == strlen(a));
        if (is_port) {
            port = static_cast<in_port_t>(atoi(a));
        }
        else {
            host = a;
            ++next;
            if (argc > next)
                port = static_cast<in_port_t>(atoi(argv[next]));
        }
        ++next;
    }
    if (argc > next)
        ca_file = argv[next];

    sockpp::initialize();

    // Build a client context.
    auto ctx = sockpp::tls_context::client();
    if (ca_file.empty())
        ctx.set_default_trust_locations();
    else if (auto res = ctx.set_trust_file(ca_file); !res) {
        cerr << "Failed to load CA file: " << res.error_message() << endl;
        return 1;
    }

    error_code ec;
    sockpp::inet_address addr{host, port, ec};
    if (ec) {
        cerr << "Error resolving '" << host << "': " << ec.message() << endl;
        return 1;
    }

    // ---- First connection ----

    sockpp::tls_connector conn{ctx, ec};
    if (ec) {
        cerr << "Error creating connector: " << ec.message() << endl;
        return 1;
    }

    if (auto res = conn.connect(addr); !res) {
        cerr << "Error connecting to " << addr << ": " << res.error_message() << endl;
        return 1;
    }

    cout << "Connected to " << addr << " [" << conn.negotiated_version() << "]"
         << " — full handshake" << endl;

    /*
    #if defined(SOCKPP_OPENSSL)
        // during SSL_connect(). Do a brief read with a short timeout so the ticket
        // lands in the session before we call get_session(), regardless of whether
        // the user types anything in the echo loop.
        if (conn.negotiated_version() == "TLSv1.3") {
            conn.read_timeout(std::chrono::milliseconds(500));
            char drain[1];
            conn.read(drain, 1);  // processes NewSessionTicket, then times out
            conn.read_timeout(std::chrono::seconds(0));  // restore blocking
        }
    #endif
    */
    run_echo_loop(conn);

#if defined(SOCKPP_OPENSSL)
    cout << "\nAttempting reconnect..." << endl;

    // Save the session before the connector goes out of scope.
    // The close_notify sent by the destructor marks the session as
    // resumable on the server side.
    auto sess_res = conn.get_session();
    if (!sess_res) {
        cerr << "Warning: could not capture session: " << sess_res.error_message() << endl;
        return 0;
    }
    sockpp::tls_session saved = std::move(sess_res.value());

    // Close the first connection explicitly so the destructor has already
    // run (and sent close_notify) before we attempt the second connect.
    conn = sockpp::tls_connector{ctx, ec};
    if (ec) {
        cerr << "Error resetting connector: " << ec.message() << endl;
        return 1;
    }

    // ---- Second connection: offer the saved session ----

    // Note that in TLS 1.3, the server delivers the session ticket in a
    // post-handshake record that OpenSSL only processes on the first
    // SSL_read() call. So we needed at least one successful read from the
    // first connection to resume the session.

    if (auto res = conn.set_session(saved); !res) {
        cerr << "Warning: set_session failed: " << res.error_message() << endl;
    }

    if (auto res = conn.connect(addr); !res) {
        cerr << "Error on second connect: " << res.error_message() << endl;
        return 1;
    }

    bool reused = conn.session_reused();
    cout << "\nReconnected to " << addr << " [" << conn.negotiated_version() << "]"
         << " — " << (reused ? "session RESUMED" : "full handshake (not resumed)") << endl;

    run_echo_loop(conn);
#endif

    return 0;
}
