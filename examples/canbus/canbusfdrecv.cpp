// canbusfdrecv.cpp
//
// Linux SocketCAN FD reader example.
//
// Receives both classic CAN and CAN FD frames using a canbusfd_socket.
// Usage: canbusfdrecv [interface [can_id]]
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

#include <iomanip>
#include <iostream>
#include <string>
#include <variant>

#include "sockpp/canbus/canbus_socket.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "Sample SocketCAN FD reader for 'sockpp' " << sockpp::SOCKPP_VERSION << endl;

    string canIface = (argc > 1) ? argv[1] : "can0";
    bool hasFilter = (argc > 2);
    canid_t canID = hasFilter ? canid_t(strtoul(argv[2], nullptr, 0)) : 0;

    sockpp::initialize();

    error_code ec{};
    sockpp::canbus_address addr(canIface, ec);

    if (ec) {
        cerr << "Error finding the CAN interface '" << canIface << "': " << ec.message()
             << endl;
        return 1;
    }

    sockpp::canbusfd_socket sock(addr, ec);
    if (ec) {
        cerr << "Error opening CAN FD socket on '" << canIface << "': " << ec.message()
             << endl;
        return 1;
    }

    if (hasFilter) {
        can_filter filter{canID, CAN_SFF_MASK};
        if (auto res = sock.set_filters(&filter, 1); !res) {
            cerr << "Error setting filter: " << res << endl;
            return 1;
        }
    }

    cout << "Listening on " << sock.address();
    if (hasFilter)
        cout << " for CAN ID 0x" << hex << uppercase << canID;
    cout << endl;

    cout << hex << uppercase << setfill('0');
    cout.setf(ios::fixed, ios::floatfield);
    cout << setprecision(6);

    while (true) {
        auto res = sock.recv_any();
        if (!res) {
            cerr << "Error receiving frame: " << res << endl;
            break;
        }

        auto t = 0.0;
        if (auto ts = sock.last_frame_timestamp(); ts)
            t = ts.value();

        cout << t << "  ";

        visit(
            [](const auto& frame) {
                using T = decay_t<decltype(frame)>;
                if constexpr (is_same_v<T, sockpp::canbus_frame>)
                    cout << "CAN   ";
                else
                    cout << "CAN FD";
                cout << "  " << setw(3) << frame.id_value() << "  [" << dec
                     << unsigned(frame.len) << "]  " << hex;
                for (uint8_t i = 0; i < frame.len; ++i)
                    cout << setw(2) << unsigned(frame.data[i]) << " ";
            },
            res.value()
        );

        cout << "\n";
    }

    return 0;
}
