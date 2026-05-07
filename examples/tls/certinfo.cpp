// certinfo.cpp
//
// Reads X.509 certificate files and prints information about each one.
//
// Usage:
//   certinfo <file> [<file> ...]
//
// Each file may be PEM- or DER-encoded; the format is detected automatically.
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

#include <iostream>
#include <string>

#include "sockpp/tls/certificate.h"
#include "sockpp/version.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "Certificate Info - sockpp " << sockpp::SOCKPP_VERSION << "\n";

    if (argc < 2) {
        cerr << "Usage: certinfo <file> [<file> ...]" << endl;
        return 1;
    }

    int exit_code = 0;
    bool multi = (argc > 2);

    for (int i = 1; i < argc; ++i) {
        const string path{argv[i]};

        if (multi)
            cout << "\n--- " << path << " ---\n";

        auto res = sockpp::tls_certificate::from_file(path);
        if (!res) {
            cerr << (multi ? "  " : "") << "Error: " << res.error_message() << "\n";
            exit_code = 1;
            continue;
        }

        const auto& cert = res.value();

        cout << "  Subject : " << cert.subject_name() << "\n";
        cout << "  Issuer  : " << cert.issuer_name() << "\n";
        cout << "  Valid from : " << cert.not_before_str() << "\n";
        cout << "  Valid to   : " << cert.not_after_str() << "\n";
        cout << "  DER size   : " << cert.to_der().size() << " bytes\n";
    }

    cout << endl;
    return exit_code;
}
