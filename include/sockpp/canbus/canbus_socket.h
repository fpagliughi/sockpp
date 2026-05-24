/**
 * @file canbus_socket.h
 *
 * Class for Linux SocketCAN socket.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date March 2021
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2021-2026 Frank Pagliughi
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

#ifndef __sockpp_canbus_socket_h
#define __sockpp_canbus_socket_h

#include <linux/can/raw.h>

#include <vector>

#include "sockpp/canbus/canbus_address.h"
#include "sockpp/canbus/canbus_frame.h"
#include "sockpp/raw_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Raw Linux CANbus (SocketCAN) sockets.
 *
 * Raw CAN sockets are comparable to using the old character drivers to send
 * and receive individual CAN frames to one or more interfaces.
 *
 * According to the SocketCAN notes: defaults are set at RAW socket binding
 * time:
 * @li The filters are set to exactly one filter receiving everything
 * @li The socket only receives valid data frames (=> no error message
 * frames)
 * @li The loopback of sent CAN frames is enabled (see Local Loopback of
 * Sent Frames)
 * @li The socket does not receive its own sent frames (in loopback mode)
 *
 * These can all be changed by setting options on the socket.
 */
class canbus_socket : public raw_socket
{
    /** The base class */
    using base = raw_socket;

    // Non-copyable
    canbus_socket(const canbus_socket&) = delete;
    canbus_socket& operator=(const canbus_socket&) = delete;

    /**
     * We can't connect to a raw CAN socket;
     * we can only bind the address/iface.
     */
    result<> connect(const sock_address&) = delete;

protected:
    static result<socket_t> create_handle(int type, int protocol) {
        return check_socket(socket_t(::socket(PROTOCOL_FAMILY, type, protocol)));
    }

public:
    /**
     *  The SocketCAN protocol family.
     *  Note that AF_CAN == PF_CAN, which is used in many of the CAN
     *  examples.
     */
    static const int PROTOCOL_FAMILY = AF_CAN;

    /** The socket 'type' for communications semantics. */
    static constexpr int COMM_TYPE = SOCK_RAW;

    /**
     * Creates an uninitialized CAN socket.
     */
    canbus_socket() noexcept {}
    /**
     * Creates a CAN socket from an existing OS socket handle and
     * claims ownership of the handle.
     * @param handle A socket handle from the operating system.
     */
    explicit canbus_socket(socket_t handle) noexcept : base(handle) {}
    /**
     * Creates a CAN socket and binds it to the address.
     * @param addr The address to bind.
     * @throws std::system_error on failure
     */
    explicit canbus_socket(const canbus_address& addr) {
        if (auto res = open(addr); !res)
            throw std::system_error{res.error()};
    }
    /**
     * Creates a CAN socket and binds it to the address.
     * @param addr The address to bind.
     * @param ec The error code, on failure
     */
    explicit canbus_socket(const canbus_address& addr, error_code& ec) noexcept {
        ec = open(addr).error();
    }
    /**
     * Move constructor.
     * @param other The other socket to move to this one
     */
    canbus_socket(canbus_socket&& other) : base(std::move(other)) {}
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    canbus_socket& operator=(canbus_socket&& rhs) {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Opens the CANbus socket and binds it to the address.
     * @param addr The address to bind the socket
     * @return The error code, on failure.
     */
    result<> open(const canbus_address& addr) noexcept;
    /**
     * Gets the system time of the last frame read from the socket.
     * @return The system time of the last frame read from the socket with
     *  	   microsecond precision.
     */
    result<std::chrono::system_clock::time_point> last_frame_time();
    /**
     * Gets a floating point timestamp of the last frame read from the
     * socket.
     * This is the number of seconds since the Unix epoch (time_t), with
     * floating-point, microsecond precision.
     * @return A floating-point timestamp with microsecond precision.
     */
    result<double> last_frame_timestamp();

    // ----- Filters -----

    /**
     * Sets the filters for receiving CAN frames on this socket.
     *
     * A filter matches when:
     * \verbatim
     * <received_can_id> & mask == can_id & mask
     * \endverbatim
     *
     * @param filters The CAN filters
     * @param n The number of CAN filters.
     * @return @em true if the filters were set, @em false otherwise.
     */
    result<> set_filters(const can_filter* filters, size_t n) {
        return set_option(
            SOL_CAN_RAW, CAN_RAW_FILTER, filters, socklen_t(n * sizeof(can_filter))
        );
    }

    /**
     * Sets the filters for receiving CAN frames on this socket.
     *
     * A filter matches when:
     * \verbatim
     * <received_can_id> & mask == can_id & mask
     * \endverbatim
     *
     * @param filters The CAN filters
     * @return @em true if the filters were set, @em false otherwise.
     */
    result<> set_filters(const vector<can_filter>& filters) {
        return set_filters(filters.data(), filters.size());
    }

    // ----- I/O -----

    /**
     * Sends a frame to the CAN bus.
     * @param frame The CAN frame to send.
     * @param flags The option bit flags. See send(2).
     * @return The number of bytes sent on success, or the error code on
     *         failure.
     */
    result<size_t> send(const canbus_frame& frame, int flags = 0) {
        return base::send(frame.frame_ptr(), sizeof(canbus_frame), flags);
    }
    /**
     * Receives a classic CAN frame on the socket.
     *
     * Do not call this function if the socket is in FD mode; use
     * canbusfd_socket instead. If an FD frame arrives, an error is returned
     * and the frame is lost.
     *
     * @param frame CAN frame to get the incoming data.
     * @param flags The option bit flags. See recv(2).
     * @return The number of bytes read on success, or the error code on
     *         failure.
     */
    result<size_t> recv(canbus_frame* frame, int flags = 0);
    /**
     * Receives a classic CAN frame on the socket.
     *
     * Do not call this function if the socket is in FD mode; use
     * canbusfd_socket instead. If an FD frame arrives, an error is returned
     * and the frame is lost.
     *
     * @param flags The option bit flags. See recv(2).
     * @return The frame read on success, or the error code on failure.
     */
    result<canbus_frame> recv(int flags = 0);
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Linux CANbus FD (SocketCAN) socket.
 *
 * Extends canbus_socket with FD mode, allowing reception and transmission of
 * the larger CAN FD frames. The socket is placed in FD mode during open(),
 * so recv() is replaced with versions that return canbusfd_frame. Sending
 * classic canbus_frame is still supported via the inherited send().
 */
class canbusfd_socket : public canbus_socket
{
    /** The base class */
    using base = canbus_socket;

    // Non-copyable
    canbusfd_socket(const canbusfd_socket&) = delete;
    canbusfd_socket& operator=(const canbusfd_socket&) = delete;

    /**
     * Turn on FD mode for the socket on or off.
     *
     * This allows the socket to read or write CAN FD frames.
     */
    result<> set_fd_mode() {
        int val = 1;
        return set_option(SOL_CAN_RAW, CAN_RAW_FD_FRAMES, val);
    }

public:
    /**
     * Creates an uninitialized CAN FD socket.
     */
    canbusfd_socket() noexcept {}
    /**
     * Creates a CAN FD socket and binds it to the address.
     * The socket is put into FD mode after binding.
     * @param addr The address to bind.
     * @throws std::system_error on failure
     */
    explicit canbusfd_socket(const canbus_address& addr) {
        if (auto res = open(addr); !res)
            throw std::system_error{res.error()};
    }
    /**
     * Creates a CAN FD socket and binds it to the address.
     * The socket is put into FD mode after binding.
     * @param addr The address to bind.
     * @param ec The error code, on failure
     */
    explicit canbusfd_socket(const canbus_address& addr, error_code& ec) noexcept {
        ec = open(addr).error();
    }
    /**
     * Move constructor.
     * @param other The other FD socket to move to this one.
     */
    canbusfd_socket(canbusfd_socket&& other) noexcept : base(std::move(other)) {}
    /**
     * Move constructor from a base canbus_socket.
     * If the incoming socket is open, it is put into FD mode. Throws if the
     * socket is open but cannot be set to FD mode.
     * @param other The canbus_socket to move into this one.
     * @throws std::system_error if the open socket cannot enter FD mode.
     */
    explicit canbusfd_socket(canbus_socket&& other);
    /**
     * Move constructor from a base canbus_socket.
     * If the incoming socket is open, it is put into FD mode.
     * @param other The canbus_socket to move into this one.
     * @param ec Gets the error code on failure; the socket is closed on error.
     */
    explicit canbusfd_socket(canbus_socket&& other, error_code& ec) noexcept;
    /**
     * Attempts to create a CAN FD socket from an existing canbus_socket.
     * If the incoming socket is open, it is put into FD mode.
     * @param sock The canbus_socket to convert.
     * @return The new FD socket on success, or the error code on failure.
     */
    static result<canbusfd_socket> try_from(canbus_socket&& sock) noexcept;
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    canbusfd_socket& operator=(canbusfd_socket&& rhs) noexcept {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Opens the CANbus FD socket and binds it to the address.
     * After binding, the socket is put into FD mode.
     * @param addr The address to bind the socket.
     * @return The error code, on failure.
     */
    result<> open(const canbus_address& addr) noexcept;

    // ----- I/O -----

    /**
     * Sends a classic CAN frame to the CAN bus.
     * An FD socket can still send classic frames.
     */
    using base::send;
    /**
     * Sends an FD frame to the CAN bus.
     * @param frame The CAN FD frame to send.
     * @param flags The option bit flags. See send(2).
     * @return The number of bytes sent on success, or the error code on
     *         failure.
     */
    result<size_t> send(const canbusfd_frame& frame, int flags = 0) {
        return socket::send(frame.frame_ptr(), sizeof(canbusfd_frame), flags);
    }
    /**
     * Sends either a classic or FD frame to the CAN bus.
     * @param frame The frame to send (classic or FD).
     * @param flags The option bit flags. See send(2).
     * @return The number of bytes sent on success, or the error code on
     *         failure.
     */
    result<size_t> send(const canbus_any_frame& frame, int flags = 0) {
        return std::visit([&](const auto& f) { return send(f, flags); }, frame);
    }
    /**
     * Receives a CAN FD frame on the socket.
     * @param frame CAN FD frame to get the incoming data.
     * @param flags The option bit flags. See recv(2).
     * @return The number of bytes read on success, or the error code on
     *         failure.
     */
    result<size_t> recv(canbusfd_frame* frame, int flags = 0);
    /**
     * Receives a CAN FD frame on the socket.
     * @param flags The option bit flags. See recv(2).
     * @return The frame read on success, or the error code on failure.
     */
    result<canbusfd_frame> recv(int flags = 0);
    /**
     * Receives either a classic or FD frame from the socket.
     * The frame type is determined by the size of the received packet.
     * @param flags The option bit flags. See recv(2).
     * @return The frame read on success, or the error code on failure.
     */
    result<canbus_any_frame> recv_any(int flags = 0);
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_canbus_socket_h
