// canbus_socket.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2021-2023 Frank Pagliughi
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

#include "sockpp/canbus/canbus_socket.h"

#include <linux/ethtool.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <chrono>
#include <cstring>
#include <ctime>

#include "sockpp/socket.h"

using namespace std;
using namespace std::chrono;

namespace {

// Converts a timespec to a system_clock time_point (wall-clock).
system_clock::time_point ts_to_tp(const timespec& ts) {
    auto dur = seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
    return system_clock::time_point{duration_cast<system_clock::duration>(dur)};
}

// Converts a timespec to a nanoseconds duration (for hardware clock values).
nanoseconds ts_to_ns(const timespec& ts) {
    return seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
}

// Returns true if the timespec is non-zero (i.e. the timestamp was filled in).
bool ts_nonzero(const timespec& ts) { return ts.tv_sec != 0 || ts.tv_nsec != 0; }

}  // namespace

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

result<> canbus_socket::open(const canbus_address& addr) noexcept {
    if (auto createRes = create_handle(SOCK_RAW, CAN_RAW); !createRes) {
        return createRes.error();
    }
    else {
        reset(createRes.value());
        if (auto res = bind(addr); !res) {
            close();
            return res;
        }
    }
    return none{};
}

bool canbus_socket::has_hw_timestamps() const noexcept {
    error_code ec{};
    canbus_address addr{base::address(), ec};
    if (ec)
        return false;

    ethtool_ts_info info{};
    info.cmd = ETHTOOL_GET_TS_INFO;
    ifreq ifr{};
    const string iface = addr.iface();
    std::strncpy(ifr.ifr_name, iface.c_str(), IF_NAMESIZE - 1);
    ifr.ifr_data = reinterpret_cast<char*>(&info);

    return ::ioctl(handle(), SIOCETHTOOL, &ifr) >= 0 &&
           (info.so_timestamping & SOF_TIMESTAMPING_RX_HARDWARE) != 0;
}

result<system_clock::time_point> canbus_socket::last_frame_time() {
    timeval tv{};

    if (auto res = check_res_none(::ioctl(handle(), SIOCGSTAMP, &tv)); !res)
        return res.error();
    return to_timepoint(tv);
}

result<double> canbus_socket::last_frame_timestamp() {
    timeval tv{};

    if (auto res = check_res_none(::ioctl(handle(), SIOCGSTAMP, &tv)); !res)
        return res.error();
    return double(tv.tv_sec) + 1.0e-6 * tv.tv_usec;
}

result<size_t> canbus_socket::recv(canbus_frame* frame, int flags /*=0*/) {
    auto res = base::recv(frame->frame_ptr(), sizeof(canbus_frame), flags | MSG_TRUNC);
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbus_frame))
        return errc::message_size;
    return res;
}

result<canbus_frame> canbus_socket::recv(int flags /*=0*/) {
    canbus_frame frame;
    if (auto res = recv(&frame, flags); !res)
        return res.error();
    return frame;
}

result<pair<canbus_frame, system_clock::time_point>>
canbus_socket::recv_with_timestamp(int flags /*=0*/) {
    canbus_frame frame;
    iovec iov{};
    iov.iov_base = frame.frame_ptr();
    iov.iov_len = sizeof(canbus_frame);

    alignas(cmsghdr) char ctrl[CMSG_SPACE(sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbus_frame))
        return errc::message_size;

    system_clock::time_point ts{};
    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPNS) {
            timespec stamp{};
            std::memcpy(&stamp, CMSG_DATA(cm), sizeof(stamp));
            ts = ts_to_tp(stamp);
            break;
        }
    }

    return make_pair(frame, ts);
}

result<pair<canbus_frame, canbus_timestamps>>
canbus_socket::recv_with_timestamps(int flags /*=0*/) {
    std::pair<canbus_frame, canbus_timestamps> timed{};
    iovec iov{};
    iov.iov_base = timed.first.frame_ptr();
    iov.iov_len = sizeof(canbus_frame);

    alignas(
        cmsghdr
    ) char ctrl[CMSG_SPACE(sizeof(timespec)) + CMSG_SPACE(3 * sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbus_frame))
        return errc::message_size;

    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level != SOL_SOCKET)
            continue;
        if (cm->cmsg_type == SO_TIMESTAMPNS) {
            timespec stamp{};
            std::memcpy(&stamp, CMSG_DATA(cm), sizeof(stamp));
            timed.second.socket = ts_to_tp(stamp);
        }
        else if (cm->cmsg_type == SO_TIMESTAMPING) {
            timespec ts[3]{};
            std::memcpy(ts, CMSG_DATA(cm), sizeof(ts));
            if (ts_nonzero(ts[0]))
                timed.second.sw = ts_to_tp(ts[0]);
            if (ts_nonzero(ts[2]))
                timed.second.hw = ts_to_ns(ts[2]);
        }
    }

    return timed;
}

result<pair<canbus_frame, nanoseconds>>
canbus_socket::recv_with_hw_timestamp(int flags /*=0*/) {
    canbus_frame frame{};
    iovec iov{};
    iov.iov_base = frame.frame_ptr();
    iov.iov_len = sizeof(canbus_frame);

    alignas(cmsghdr) char ctrl[CMSG_SPACE(3 * sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbus_frame))
        return errc::message_size;

    nanoseconds hw{};
    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING) {
            timespec ts[3]{};
            std::memcpy(ts, CMSG_DATA(cm), sizeof(ts));
            hw = ts_to_ns(ts[2]);
            break;
        }
    }

    return make_pair(frame, hw);
}

/////////////////////////////////////////////////////////////////////////////

canbusfd_socket::canbusfd_socket(canbus_socket&& other) : base(std::move(other)) {
    if (is_open()) {
        if (auto res = set_fd_mode(); !res)
            throw std::system_error{res.error()};
    }
}

canbusfd_socket::canbusfd_socket(canbus_socket&& other, error_code& ec) noexcept
    : base(std::move(other)) {
    if (is_open()) {
        if (auto res = set_fd_mode(); !res) {
            ec = res.error();
            close();
        }
    }
}

result<canbusfd_socket> canbusfd_socket::try_from(canbus_socket&& sock) noexcept {
    canbusfd_socket fd_sock;
    static_cast<canbus_socket&>(fd_sock) = std::move(sock);
    if (fd_sock.is_open()) {
        if (auto res = fd_sock.set_fd_mode(); !res)
            return res.error();
    }
    return fd_sock;
}

result<> canbusfd_socket::open(const canbus_address& addr) noexcept {
    if (auto res = base::open(addr); !res)
        return res;
    if (auto res = set_fd_mode(); !res) {
        close();
        return res;
    }
    return none{};
}

result<size_t> canbusfd_socket::recv(canbusfd_frame* frame, int flags /*=0*/) {
    auto res = socket::recv(frame->frame_ptr(), sizeof(canbusfd_frame), flags | MSG_TRUNC);
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbusfd_frame))
        return errc::message_size;
    return res;
}

result<canbusfd_frame> canbusfd_socket::recv(int flags /*=0*/) {
    canbusfd_frame frame;
    if (auto res = recv(&frame, flags); !res)
        return res.error();
    return frame;
}

result<canbus_any_frame> canbusfd_socket::recv_any(int flags /*=0*/) {
    canbusfd_frame fdframe{};
    auto res = socket::recv(fdframe.frame_ptr(), sizeof(canbusfd_frame), flags | MSG_TRUNC);
    if (!res)
        return res.error();
    const auto n = res.value();
    if (n == sizeof(canbus_frame)) {
        // Error frames (CAN_ERR_FLAG set in can_id) also arrive as can_frame-
        // sized packets; they are returned as canbus_frame and can be
        // distinguished with canbus_frame::is_error().
        canbus_frame classic;
        std::memcpy(classic.frame_ptr(), fdframe.frame_ptr(), sizeof(canbus_frame));
        return canbus_any_frame{classic};
    }
    if (n == sizeof(canbusfd_frame))
        return canbus_any_frame{fdframe};
    return errc::message_size;
}

result<pair<canbusfd_frame, system_clock::time_point>>
canbusfd_socket::recv_with_timestamp(int flags /*=0*/) {
    canbusfd_frame frame;
    iovec iov{};
    iov.iov_base = frame.frame_ptr();
    iov.iov_len = sizeof(canbusfd_frame);

    alignas(cmsghdr) char ctrl[CMSG_SPACE(sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbusfd_frame))
        return errc::message_size;

    system_clock::time_point ts{};
    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPNS) {
            timespec stamp{};
            std::memcpy(&stamp, CMSG_DATA(cm), sizeof(stamp));
            ts = ts_to_tp(stamp);
            break;
        }
    }

    return make_pair(frame, ts);
}

result<pair<canbusfd_frame, canbus_timestamps>>
canbusfd_socket::recv_with_timestamps(int flags /*=0*/) {
    std::pair<canbusfd_frame, canbus_timestamps> timed{};
    iovec iov{};
    iov.iov_base = timed.first.frame_ptr();
    iov.iov_len = sizeof(canbusfd_frame);

    alignas(
        cmsghdr
    ) char ctrl[CMSG_SPACE(sizeof(timespec)) + CMSG_SPACE(3 * sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbusfd_frame))
        return errc::message_size;

    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level != SOL_SOCKET)
            continue;
        if (cm->cmsg_type == SO_TIMESTAMPNS) {
            timespec stamp{};
            std::memcpy(&stamp, CMSG_DATA(cm), sizeof(stamp));
            timed.second.socket = ts_to_tp(stamp);
        }
        else if (cm->cmsg_type == SO_TIMESTAMPING) {
            timespec ts[3]{};
            std::memcpy(ts, CMSG_DATA(cm), sizeof(ts));
            if (ts_nonzero(ts[0]))
                timed.second.sw = ts_to_tp(ts[0]);
            if (ts_nonzero(ts[2]))
                timed.second.hw = ts_to_ns(ts[2]);
        }
    }

    return timed;
}

result<pair<canbusfd_frame, nanoseconds>>
canbusfd_socket::recv_with_hw_timestamp(int flags /*=0*/) {
    canbusfd_frame frame{};
    iovec iov{};
    iov.iov_base = frame.frame_ptr();
    iov.iov_len = sizeof(canbusfd_frame);

    alignas(cmsghdr) char ctrl[CMSG_SPACE(3 * sizeof(timespec))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    auto res = check_res<size_t>(::recvmsg(handle(), &msg, flags | MSG_TRUNC));
    if (!res)
        return res.error();
    if (res.value() > sizeof(canbusfd_frame))
        return errc::message_size;

    nanoseconds hw{};
    for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING) {
            timespec ts[3]{};
            std::memcpy(ts, CMSG_DATA(cm), sizeof(ts));
            hw = ts_to_ns(ts[2]);
            break;
        }
    }

    return make_pair(frame, hw);
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
