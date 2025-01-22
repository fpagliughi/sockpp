// tlsconn.cpp
//
// Example app showing some options for connecting to a secure server.
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2024 Frank Pagliughi
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

#include <getopt.h>

#include <fstream>
#include <iostream>
#include <string>

#include "sockpp/tcp_connector.h"
#include "sockpp/tls/connector.h"
#include "sockpp/tls/context.h"
#include "sockpp/tls/error.h"
#include "sockpp/version.h"

using namespace std;

int main(int argc, char* argv[]) {
    bool verify = false;
    string trustStore, certFile, keyFile;

    int c, iOpt;
    static option longOpts[] = {
        {"verify", no_argument, 0, 'v'},
        {"trust-store", required_argument, 0, 't'},
        {"cert", required_argument, 0, 'c'},
        {"key", required_argument, 0, 'k'},
        {0, 0, 0, 0}
    };

    cout << "Sample TLS test connector for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n'
         << endl;

    while ((c = getopt_long(argc, argv, "t:c:k:v", longOpts, &iOpt)) != -1) {
        switch (c) {
            case 'v':
                verify = true;
                break;

            case 't':
                trustStore = string{optarg};
                break;

            case 'c':
                certFile = string{optarg};
                break;

            case 'k':
                keyFile = string{optarg};
                break;

            default:
                cerr << "Unknown option: " << optarg << endl;
                return 1;
        }
    }

    int narg = argc - optind;

    string host = (narg > 0) ? argv[optind] : "example.org";
    in_port_t port = (narg > 1) ? atoi(argv[optind + 1]) : 443;

    sockpp::initialize();

    auto ctxBldr = sockpp::tls_context_builder::client();

    if (verify)
        ctxBldr.verify_peer();

    if (trustStore.empty())
        ctxBldr.default_trust_locations();
    else
        ctxBldr.trust_file(trustStore);

    if (!certFile.empty())
        ctxBldr.cert_file(certFile);

    if (!keyFile.empty())
        ctxBldr.key_file(keyFile);

    if (ctxBldr.error()) {
        cerr << "Error creating TLS context: " << ctxBldr.error().message() << endl;
        return 1;
    }

    auto ctx = ctxBldr.finalize();

    // Implicitly creates an inet_address from {host,port}
    // and then tries the connection.

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

    cout << "Successful connection to " << addr << endl;

    if (auto opt_cert = conn.peer_certificate(); !opt_cert) {
        cout << "No peer certificate" << endl;
    }
    else {
        const auto& cert = opt_cert.value();

        cout << "\nCertificate info:\n"
             << "  Subject: " << cert.subject_name() << '\n'
             << "  Issuer: " << cert.issuer_name() << '\n'
             << "  Valid dates: " << cert.not_before_str() << " - " << cert.not_after_str()
             << endl;

        ofstream derfil("peer.cer", ios::binary);
        auto der = cert.to_der();
        derfil.write(reinterpret_cast<const char*>(der.data()), der.size());
        cout << "\nWrote peer certificate to peer.cer" << endl;

        ofstream pemfil("peer.pem");
        auto pem = cert.to_pem();
        pemfil.write(pem.data(), pem.size());
        cout << "\nWrote peer certificate to peer.pem" << endl;
    }

    if (auto res = conn.write("HELO"); !res) {
        cerr << "Error sending request [0x" << hex << res.error().value()
             << "]: " << res.error_message() << endl;
        return 1;
    }

    char buf[512];
    if (auto res = conn.read(buf, sizeof(buf)); !res) {
        cerr << "Error: " << res.error_message() << endl;
        return 1;
    }

    return 0;
}
