/**
 * @file canbus_frame.h
 *
 * Class for the Linux SocketCAN frames.
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

#ifndef __sockpp_canbus_frame_h
#define __sockpp_canbus_frame_h

#include <linux/can.h>

#include <algorithm>
#include <cstring>
#include <optional>
#include <variant>
#if __cplusplus >= 202002L
    #include <span>
#endif

#include "sockpp/platform.h"
#include "sockpp/result.h"
#include "sockpp/types.h"

namespace sockpp {

class canbusfd_frame;

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN frame.
 * This inherits from the Linux CAN frame struct, just providing easier
 * construction.
 *
 * The underlying @p can_frame struct uses the field name @p len for the
 * payload length. The old name @p can_dlc was deprecated in Linux 5.11
 * (February 2021) and is kept only as a compatibility alias in the kernel
 * headers.
 */
class canbus_frame : public ::can_frame
{
    /** The base class is the C library CAN frame struct. */
    using base = ::can_frame;

    /** The size of the underlying address struct, in bytes */
    static constexpr size_t SZ = sizeof(::can_frame);

public:
    /**
     * Constructs an empty frame.
     * The frame is initialized to all zeroes.
     */
    canbus_frame() : base{} {}
    /**
     * Constructs a frame with the specified ID and no data.
     * @param canID The CAN identifier for the frame
     */
    canbus_frame(canid_t canID) : canbus_frame{canID, nullptr, 0} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     */
    canbus_frame(canid_t canID, const string& data)
        : canbus_frame{canID, data.data(), data.length()} {}
    /**
     * Constructs a frame with the specified ID and data.
     *
     * Data is silently clamped to CAN_MAX_DLEN bytes if @p n exceeds it.
     * Passing a null @p data pointer with a non-zero @p n is a caller error
     * and throws invalid_argument.
     *
     * @param canID The CAN identifier for the frame
     * @param data Pointer to the data bytes; may be nullptr only when @p n is 0.
     * @param n The number of bytes in the data field
     * @throws invalid_argument if @p data is null and @p n is non-zero.
     */
    canbus_frame(canid_t canID, const void* data, size_t n);
    /**
     * Constructs a frame with the specified ID and data.
     *
     * Data is silently clamped to CAN_MAX_DLEN bytes if @p n exceeds it.
     * Sets @p ec to invalid_argument if @p data is null and @p n is non-zero.
     *
     * @param canID The CAN identifier for the frame
     * @param data Pointer to the data bytes; may be nullptr only when @p n is 0.
     * @param n The number of bytes in the data field
     * @param ec Gets the error code on failure.
     */
    canbus_frame(canid_t canID, const void* data, size_t n, error_code& ec) noexcept;
    /**
     * Construct a frame from a C library CAN frame.
     * @param frame A C lib CAN frame.
     */
    canbus_frame(const base& frame) : base{frame} {}
    /**
     * Try to convert a CAN FD frame into a classic one.
     * This only succeeds if the FD data fits into this object.
     * @param fdframe The frame to convert.
     * @throws invalid_argument if the data does not fit.
     */
    canbus_frame(const canbusfd_frame& fdframe);
    /**
     * Try to convert a CAN FD frame into a classic one.
     * This only succeeds if the FD data fits into this object.
     * @param fdframe The frame to convert.
     * @param ec Gets the error code, invalid_argument, if the data does not
     *  		 fit.
     */
    canbus_frame(const canbusfd_frame& fdframe, error_code& ec);
    /**
     * Gets a pointer to the underlying C frame.
     * @return A pointer to the underlying C frame.
     */
    ::can_frame* frame_ptr() { return static_cast<::can_frame*>(this); }
    /**
     * Gets a const pointer to the underlying C frame.
     * @return A const pointer to the underlying C frame.
     */
    const ::can_frame* frame_ptr() const { return static_cast<const ::can_frame*>(this); }
    /**
     * Gets the length of data (number of bytes) as a size_t.
     * @return The number of valid data bytes.
     */
    size_t length() const { return size_t(this->len); }
#if __cplusplus >= 202002L
    /**
     * Returns the payload as a span of bytes.
     * @return A span covering the valid data bytes in this frame.
     */
    std::span<uint8_t> payload() noexcept { return {this->data, len}; }
    /** @overload */
    std::span<const uint8_t> payload() const noexcept { return {this->data, len}; }
#endif
    /**
     * Determines if this frame has an extended (29-bit) CAN ID.
     * @return @em true if this frame has an extended ID, @em false if not
     */
    bool has_extended_id() const { return (can_id & CAN_EFF_FLAG) != 0; }
    /**
     * Determines if this is a remote transmission request (RTR) frame.
     * @return @em true if this is an RTR frame, @em false if not
     */
    bool is_remote() const { return (can_id & CAN_RTR_FLAG) != 0; }
    /**
     * Determines if this is an error frame.
     * @return @em true if this is an error frame, @em false if not
     */
    bool is_error() const { return (can_id & CAN_ERR_FLAG) != 0; }
    /**
     * Gets the numeric CAN ID.
     * @return The numeric CAN ID.
     */
    canid_t id_value() const {
        if (has_extended_id())
            return can_id & CAN_EFF_MASK;
        return can_id & CAN_SFF_MASK;
    }
    /**
     * Sets the ID as a standard 11-bit value.
     * @param canID The new CAN ID, in the range 0 - 0x7FF.
     */
    void set_standard_id(canid_t canID) {
        can_id &= ~(CAN_EFF_FLAG | CAN_SFF_MASK);
        can_id |= canID & CAN_SFF_MASK;
    }
    /**
     * Sets the ID as an extended 29-bit value.
     * @param canID The new CAN ID, in the range 0 - 0x1FFFFFFF.
     */
    void set_extended_id(canid_t canID) {
        can_id &= ~CAN_EFF_MASK;
        can_id |= CAN_EFF_FLAG | (canID & CAN_EFF_MASK);
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A remote transfer request (RTR) frame.
 * This is a classic frame with no data and the RTR flag set.
 */
class canbus_remote_frame : public canbus_frame
{
    /** The base class is the CAN frame */
    using base = canbus_frame;

    /** The size of the underlying address struct, in bytes */
    static constexpr size_t SZ = sizeof(canbus_frame);

public:
    /** Create a default remote frame */
    canbus_remote_frame() : base{CAN_RTR_FLAG} {}
    /**
     * Create a remote frame for the specified ID.
     * @param canID The CAN identifier for the frame
     */
    canbus_remote_frame(canid_t canID) : base{CAN_RTR_FLAG | canID} {}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN FD frame.
 * This inherits from the Linux CAN fdframe struct, just providing easier
 * construction.
 */
class canbusfd_frame : public ::canfd_frame
{
    /** The base class is the C library CAN frame struct. */
    using base = ::canfd_frame;

    /** The size of the underlying address struct, in bytes */
    static constexpr size_t SZ = sizeof(::canfd_frame);

public:
    /**
     * Constructs an empty frame.
     * The frame is initialized to all zeroes.
     */
    canbusfd_frame() : base{} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     */
    canbusfd_frame(canid_t canID, const string& data)
        : canbusfd_frame{canID, data.data(), data.length()} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     * @param n The number of bytes in the data field
     */
    canbusfd_frame(canid_t canID, const void* data, size_t n) : base{} {
        this->can_id = canID;
        if (data && n != 0) {
            n = std::min(n, size_t(CANFD_MAX_DLEN));
            this->len = n;
            ::memcpy(&this->data, data, n);
        }
    }
    /**
     * Construct an FD frame from a classic CAN 2.0 frame.
     * @param frame A classic CAN 2.0 frame.
     */
    canbusfd_frame(const canbus_frame& frame) : base{} {
        // TODO: Should we reject RTR or error frames?
        std::memcpy(frame_ptr(), frame.frame_ptr(), sizeof(::can_frame));
    }
    /**
     * Construct a frame from a C library CAN frame.
     * @param frame A C lib CAN frame.
     */
    canbusfd_frame(const base& frame) : base{frame} {}
    /**
     * Gets a pointer to the underlying C frame.
     * @return A pointer to the underlying C frame.
     */
    ::canfd_frame* frame_ptr() { return static_cast<::canfd_frame*>(this); }
    /**
     * Gets a const pointer to the underlying C frame.
     * @return A const pointer to the underlying C frame.
     */
    const ::canfd_frame* frame_ptr() const { return static_cast<const ::canfd_frame*>(this); }
#if __cplusplus >= 202002L
    /**
     * Returns the payload as a span of bytes.
     * @return A span covering the valid data bytes in this frame.
     */
    std::span<uint8_t> payload() noexcept { return {this->data, len}; }
    /** @overload */
    std::span<const uint8_t> payload() const noexcept { return {this->data, len}; }
#endif
    /**
     * Determines if this frame has an extended (29-bit) CAN ID.
     * @return @em true if this frame has an extended ID, @em false if not
     */
    bool has_extended_id() const { return (can_id & CAN_EFF_FLAG) != 0; }
    /**
     * Gets the numeric CAN ID.
     * @return The numeric CAN ID.
     */
    canid_t id_value() const {
        if (has_extended_id())
            return can_id & CAN_EFF_MASK;
        return can_id & CAN_SFF_MASK;
    }
    /**
     * Sets the ID as a standard 11-bit value.
     * @param canID The new CAN ID, in the range 0 - 0x7FF.
     */
    void set_standard_id(canid_t canID) {
        can_id &= ~(CAN_EFF_FLAG | CAN_SFF_MASK);
        can_id |= canID & CAN_SFF_MASK;
    }
    /**
     * Sets the ID as an extended 29-bit value.
     * @param canID The new CAN ID, in the range 0 - 0x1FFFFFFF.
     */
    void set_extended_id(canid_t canID) {
        can_id &= ~CAN_EFF_MASK;
        can_id |= CAN_EFF_FLAG | (canID & CAN_EFF_MASK);
    }
    /**
     * Converts a CAN FD DLC code (0–15) to the corresponding payload length
     * in bytes.
     *
     * CAN FD DLC codes 0–8 map one-to-one; codes 9–15 map to the
     * non-contiguous lengths 12, 16, 20, 24, 32, 48, 64.
     *
     * @param dlc A DLC code in the range 0–15.
     * @return The payload length in bytes.
     */
    static constexpr uint8_t dlc_to_len(uint8_t dlc) noexcept {
        constexpr uint8_t tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
        return tbl[dlc < 16 ? dlc : 15];
    }
    /**
     * Converts a payload byte count to the smallest CAN FD DLC code that
     * accommodates it.
     *
     * @param len A payload length in bytes (0–64).
     * @return The DLC code (0–15).
     */
    static constexpr uint8_t len_to_dlc(uint8_t len) noexcept {
        if (len <= 8)
            return len;
        if (len <= 12)
            return 9;
        if (len <= 16)
            return 10;
        if (len <= 20)
            return 11;
        if (len <= 24)
            return 12;
        if (len <= 32)
            return 13;
        if (len <= 48)
            return 14;
        return 15;
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A variant type that can hold either a classic CAN frame or a CAN FD frame.
 */
using canbus_any_frame = std::variant<canbus_frame, canbusfd_frame>;

// ----- Free-function accessors for canbus_any_frame -----

/**
 * Gets the numeric CAN ID from any frame type.
 * @param frame A classic or FD frame variant.
 * @return The CAN ID with flag bits stripped.
 */
inline canid_t id_value(const canbus_any_frame& frame) noexcept {
    return std::visit([](const auto& f) { return f.id_value(); }, frame);
}
/**
 * Gets the payload length from any frame type.
 * @param frame A classic or FD frame variant.
 * @return The number of valid data bytes.
 */
inline uint8_t len(const canbus_any_frame& frame) noexcept {
    return std::visit([](const auto& f) { return f.len; }, frame);
}
#if __cplusplus >= 202002L
/**
 * Gets the payload as a span of bytes from any frame type.
 * @param frame A classic or FD frame variant.
 * @return A span covering the valid data bytes.
 */
inline std::span<const uint8_t> payload(const canbus_any_frame& frame) noexcept {
    return std::visit(
        [](const auto& f) -> std::span<const uint8_t> { return f.payload(); }, frame
    );
}
#endif

/////////////////////////////////////////////////////////////////////////////

/**
 * Kernel timestamps associated with a received CAN frame.
 *
 * Fields are populated according to which socket options were enabled before
 * the receive call:
 *  - @p socket — from @p SO_TIMESTAMPNS; enabled via set_recv_timestamp()
 *  - @p sw     — from @p SOF_TIMESTAMPING_RX_SOFTWARE; enabled via set_timestamping()
 *  - @p hw     — from @p SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
 *                enabled via set_timestamping(). Reported in the hardware clock's
 *                domain, not wall-clock time.
 */
struct canbus_timestamps
{
    /** Socket-layer arrival time (SO_TIMESTAMPNS), wall clock. */
    std::optional<system_clock::time_point> socket;
    /** Network-stack entry time (SOF_TIMESTAMPING_RX_SOFTWARE), wall clock. */
    std::optional<system_clock::time_point> sw;
    /** Raw hardware clock value (SOF_TIMESTAMPING_RX_HARDWARE). Not wall-clock time. */
    std::optional<nanoseconds> hw;
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_canbus_frame_h
