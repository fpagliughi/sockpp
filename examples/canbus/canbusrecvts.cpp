// canbusrecvts.cpp
//
// Linux SocketCAN reader with kernel timestamps example.
//
// If the adapter supports hardware timestamps, outputs each received
// frame as:
//   <sw_timestamp>,  (<hw_timestamp>),  <id>  [<len>]  <data bytes>
//
// Otherwise, falls back to software-only timestamps:
//   <sw_timestamp>,  <id>  [<len>]  <data bytes>
//
// The software timestamp is the kernel network-stack entry time from
// SOF_TIMESTAMPING_RX_SOFTWARE, reported as floating-point seconds since the
// Unix epoch.  The hardware timestamp is the raw value from the CAN controller
// in its own clock domain, reported as an integer nanosecond count.
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

#include <linux/net_tstamp.h>

#include <iomanip>
#include <iostream>
#include <string>

#include "sockpp/canbus/canbus_socket.h"
#include "sockpp/version.h"

using namespace std;
using namespace std::chrono;

// --------------------------------------------------------------------------

// Converts a system_clock time_point to floating-point seconds since the epoch.
static double to_secs(system_clock::time_point tp) {
    return duration_cast<duration<double>>(tp.time_since_epoch()).count();
}

// Prints the frame ID, length, and data bytes to stdout.
// Assumes hex/uppercase/setfill('0') are already set on cout.
static void print_frame(const sockpp::canbus_frame& frame) {
    cout << setw(3) << frame.id_value() << "  [" << dec << frame.length() << "]  "
         << hex;
    for (uint8_t i = 0; i < frame.len; ++i) {
        cout << setw(2) << unsigned(frame.data[i]) << " ";
    }
    cout << "\n";
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    cout << "Sample SocketCAN timestamped reader for 'sockpp' " << sockpp::SOCKPP_VERSION
         << "\n";

    string canIface = (argc > 1) ? argv[1] : "can0";
    bool hasFilter = (argc > 2);
    canid_t canID = hasFilter ? canid_t(strtoul(argv[2], nullptr, 0)) : 0;

    sockpp::initialize();

    error_code ec{};
    sockpp::canbus_address addr(canIface, ec);

    if (ec) {
        cerr << "Error finding the CAN interface '" << canIface << "': " << ec.message()
             << "\n";
        return 1;
    }

    sockpp::canbus_socket sock(addr, ec);
    if (ec) {
        cerr << "Error binding to the CAN interface '" << canIface << "': " << ec.message()
             << "\n";
        return 1;
    }

    cout << "Listening on " << sock.address() << "..." << endl;

    if (hasFilter) {
        can_filter filter{canID, CAN_SFF_MASK};
        if (auto res = sock.set_filters(&filter, 1); !res) {
            cerr << "\nError setting filter: " << res.error().message() << endl;
            return 1;
        }
        cout << " for CAN ID 0x" << hex << uppercase << canID;
    }

    cout << "\n";

    // Enable software SO_TIMESTAMPNS

    if (auto res = sock.set_recv_timestamp(); !res) {
        cerr << "Error enabling SO_TIMESTAMPNS: " << res.error().message() << "\n";
        return 1;
    }

    // Check if the interface supports hardware timestamps
    const bool hwTs = sock.has_hw_timestamps();

    if (hwTs) {
        if (auto res = sock.set_timestamping(
                SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE |
                SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE
            );
            !res) {
            cerr << "Error enabling SO_TIMESTAMPING: " << res.error().message() << "\n";
            return 1;
        }
    }
    else {
        cout << "HW timestamps not supported" << endl;
    }

    // Display timestamps as time_t w/ usec resolution
    cout.setf(ios::fixed, ios::floatfield);
    cout << setprecision(6);

    if (hwTs) {
        // Retrieve frames with hardware and software timestamps
        while (true) {
            auto res = sock.recv_with_timestamps();
            if (!res) {
                cerr << "Error receiving frame: " << res.error().message() << "\n";
                break;
            }
            // Here ts is a `canbus_timestamps` struct.
            const auto& [frame, ts] = res.value();

            double sw = ts.sw ? to_secs(*ts.sw) : ts.socket ? to_secs(*ts.socket) : 0.0;

            cout << sw << ",  (" << dec << (ts.hw ? ts.hw->count() : 0) << "),  ";
            cout << hex << uppercase << setfill('0');
            print_frame(frame);
        }
    }
    else {
        // HW timestamps not supported; just grab SW timestamp.
        while (true) {
            auto res = sock.recv_with_timestamp();
            if (!res) {
                cerr << "Error receiving frame: " << res.error().message() << "\n";
                break;
            }
            const auto& [frame, ts] = res.value();

            cout << to_secs(ts) << ",  ";
            cout << hex << uppercase << setfill('0');
            print_frame(frame);
        }
    }

    return 0;
}
