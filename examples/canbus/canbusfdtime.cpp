// canbusfdtime.cpp
//
// Linux SocketCAN FD writer example.
//
// This writes the current time to the CAN bus 10 times per second as a
// 12-byte CAN FD frame with nanosecond resolution. The payload format is:
//
//   Bytes 0-7:  int64_t  seconds since the Unix epoch (big-endian host order)
//   Bytes 8-11: uint32_t nanosecond fraction
//
// This is a simple (though not overly precise) way to synchronize the time
// for nodes on the CAN bus.
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

#include <time.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "sockpp/canbus/canbus_socket.h"
#include "sockpp/version.h"

using namespace std;

// The clock to use to pace the app.
using sysclock = chrono::system_clock;

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "Sample SocketCAN FD time writer for 'sockpp' " << sockpp::SOCKPP_VERSION << endl;

    string canIface = (argc > 1) ? argv[1] : "can0";
    canid_t canID = (argc > 2) ? canid_t(strtoul(argv[2], nullptr, 0)) : 0x20;

    sockpp::initialize();

    error_code ec{};
    sockpp::canbus_address addr(canIface, ec);

    if (ec) {
        cerr << "Error finding the CAN interface: " << canIface << "\n\t" << ec.message()
             << endl;
        return 1;
    }

    sockpp::canbusfd_socket sock(addr, ec);
    if (ec) {
        cerr << "Error opening CAN FD socket on " << canIface << "\n\t" << ec.message()
             << endl;
        return 1;
    }

    cout << "Created CAN FD socket on " << sock.address() << endl;

    // Schedule the first send at the next 100ms boundary.
    auto next = sysclock::now();

    while (true) {
        next += chrono::milliseconds(100);
        this_thread::sleep_until(next);

        // Get the current time with nanosecond resolution
        timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);

        // Pack into a 12-byte payload: 8-byte int64_t seconds + 4-byte uint32_t ns
        int64_t secs = int64_t(ts.tv_sec);
        uint32_t nsec = uint32_t(ts.tv_nsec);

        uint8_t payload[12];
        memcpy(payload, &secs, sizeof(secs));
        memcpy(payload + sizeof(secs), &nsec, sizeof(nsec));

        sockpp::canbusfd_frame frame{canID, payload, sizeof(payload)};
        if (auto res = sock.send(frame); !res) {
            cerr << "Error sending frame: " << res << endl;
            return 1;
        }
    }

    return 0;
}
