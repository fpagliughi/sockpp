/**
 * @file can_frame.h
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

#ifndef __sockpp_can_frame_h
#define __sockpp_can_frame_h

#include <linux/can.h>

#include <cstring>
// #include <string>
#include <algorithm>

#include "sockpp/platform.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN frame.
 * This inherits from the Linux CAN frame struct, just providing easier
   construction.
 */
class can_frame : public ::can_frame
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
    can_frame() : base{} {}
    /**
     * Constructs a frame with the specified ID and no data.
     * @param canID The CAN identifier for the frame
     */
    can_frame(canid_t canID) : can_frame{canID, nullptr, 0} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     */
    can_frame(canid_t canID, const string& data)
        : can_frame{canID, data.data(), data.length()} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     * @param n The number of bytes in the data field
     */
    can_frame(canid_t canID, const void* data, size_t n) : base{} {
        this->can_id = canID;
        if (data && n != 0) {
            n = std::min(n, size_t(CAN_MAX_DLEN));
            this->can_dlc = n;
            ::memcpy(&this->data, data, n);
        }
    }
    /**
     * Construct a frame from a C library CAN frame.
     * @param frame A C lib CAN frame.
     */
    can_frame(const base& frame) : base{frame} {}
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
class can_remote_frame : public can_frame
{
    /** The base class is the CAN frame */
    using base = can_frame;

    /** The size of the underlying address struct, in bytes */
    static constexpr size_t SZ = sizeof(can_frame);

public:
    /** Create a default remote frame */
    can_remote_frame() : base{CAN_RTR_FLAG} {}
    /**
     * Create a remote frame for the specified ID.
     * @param canID The CAN identifier for the frame
     */
    can_remote_frame(canid_t canID) : base{CAN_RTR_FLAG | canID} {}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN FD frame.
 * This inherits from the Linux CAN fdframe struct, just providing easier
 * construction.
 */
class canfd_frame : public ::canfd_frame
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
    canfd_frame() : base{} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     */
    canfd_frame(canid_t canID, const string& data)
        : canfd_frame{canID, data.data(), data.length()} {}
    /**
     * Constructs a frame with the specified ID and data.
     * @param canID The CAN identifier for the frame
     * @param data The data field for the frame
     * @param n The number of bytes in the data field
     */
    canfd_frame(canid_t canID, const void* data, size_t n) : base{} {
        this->can_id = canID;
        if (data && n != 0) {
            this->len = n;
            ::memcpy(&this->data, data, n);
        }
    }
    /**
     * Construct a frame from a C library CAN frame.
     * @param frame A C lib CAN frame.
     */
    canfd_frame(const base& frame) : base{frame} {}
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
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_can_frame_h
