/**
 * @file raw_socket.h
 *
 * Classes for raw sockets.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date April 2023
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi
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

#ifndef __sockpp_raw_socket_h
#define __sockpp_raw_socket_h

#include "sockpp/socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Base class for raw sockets.
 */
class raw_socket : public socket
{
    /** The base class */
    using base = socket;

    // Non-copyable
    raw_socket(const raw_socket&) = delete;
    raw_socket& operator=(const raw_socket&) = delete;

protected:
    /**
     * Creates a raw socket.
     * @return An OS handle to a raw socket on success, or an error code on
     *         failure.
     */
    static result<socket_t> create_handle(int domain, int protocol = 0) {
        return check_socket(socket_t(::socket(domain, COMM_TYPE, protocol)));
    }

public:
    /** The socket 'type' for communications semantics. */
    static constexpr int COMM_TYPE = SOCK_RAW;

    /**
     * Creates an uninitialized raw socket.
     */
    raw_socket() noexcept {}
    /**
     * Creates a raw socket from an existing OS socket handle and
     * claims ownership of the handle.
     * @param handle A socket handle from the operating system.
     */
    explicit raw_socket(socket_t handle) noexcept : base(handle) {}
    /**
     * Move constructor.
     * @param other The other socket to move to this one
     */
    raw_socket(raw_socket&& other) noexcept : base(std::move(other)) {}
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    raw_socket& operator=(raw_socket&& rhs) noexcept {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Creates a new raw socket that refers to this one.
     * This creates a new object with an independent lifetime, but refers
     * back to this same socket. On most systems, this duplicates the file
     * handle using the dup() call. A typical use of this is to have
     * separate threads for reading and writing the socket. One thread would
     * get the original socket and the other would get the cloned one.
     * @return A new raw socket object that refers to the same socket
     *  	   as this one.
     */
    raw_socket clone() const {
        auto h = base::clone().release();
        return raw_socket(h);
    }
    /**
     * Connects the socket to the remote address.
     * In the case of raw sockets, this does not create an actual
     * connection, but rather specifies the address to which raws are
     * sent by default and the only address from which packets are
     * received.
     * @param addr The address on which to "connect".
     * @return @em true on success, @em false on failure
     */
    result<> connect(const sock_address& addr) {
        return check_res_none(::connect(handle(), addr.sockaddr_ptr(), addr.size()));
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Base class for raw sockets.
 *
 * Datagram sockets are normally connectionless, where each packet is
 * individually routed and delivered.
 */
template <typename ADDR>
class raw_socket_tmpl : public raw_socket
{
    /** The base class */
    using base = raw_socket;

public:
    /** The address family for this type of address */
    static constexpr sa_family_t ADDRESS_FAMILY = ADDR::ADDRESS_FAMILY;
    /** The type of address for the socket. */
    using addr_t = ADDR;

    /**
     * Creates an unbound raw socket.
     * This can be used as a client or later bound as a server socket.
     * @throws std::system_error on failure.
     */
    raw_socket_tmpl() noexcept {}
    /**
     * Creates a raw socket from an existing OS socket handle and
     * claims ownership of the handle.
     * @param handle A socket handle from the operating system.
     */
    raw_socket_tmpl(socket_t handle) noexcept : base(handle) {}
    /**
     * Creates a raw socket and binds it to the address.
     * @param addr The address to bind.
     */
    raw_socket_tmpl(const ADDR& addr) : base(addr) {}
    /**
     * Move constructor.
     * Creates a raw socket by moving the other socket to this one.
     * @param sock Another raw socket.
     */
    raw_socket_tmpl(raw_socket&& sock) : base(std::move(sock)) {}
    /**
     * Move constructor.
     * @param other The other socket to move to this one
     */
    raw_socket_tmpl(raw_socket_tmpl&& other) : base(std::move(other)) {}
    /**
     * Move assignment.
     * @param rhs The other socket to move into this one.
     * @return A reference to this object.
     */
    raw_socket_tmpl& operator=(raw_socket_tmpl&& rhs) {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Creates a pair of connected stream sockets.
     *
     * Whether this will work at all is highly system and domain dependent.
     * Currently it is only known to work for Unix-domain sockets on *nix
     * systems.
     *
     * @param protocol The protocol to be used with the socket. (Normally 0)
     *
     * @return A std::tuple of stream sockets. On error both sockets will be
     *  	   in an error state with the last error set.
     */
    static std::tuple<raw_socket_tmpl, raw_socket_tmpl> pair(int protocol = 0) {
        auto pr = base::pair(addr_t::ADDRESS_FAMILY, COMM_TYPE, protocol);
        return std::make_tuple<raw_socket_tmpl, raw_socket_tmpl>(
            raw_socket_tmpl{std::get<0>(pr).release()},
            raw_socket_tmpl{std::get<1>(pr).release()}
        );
    }
    /**
     * Binds the socket to the local address.
     * Datagram sockets can bind to a local address/adapter to filter which
     * incoming packets to receive.
     * @param addr The address on which to bind.
     * @return @em true on success, @em false on failure
     */
    result<> bind(const ADDR& addr) { return base::bind(addr); }
    /**
     * Connects the socket to the remote address.
     * In the case of raw sockets, this does not create an actual
     * connection, but rather specifies the address to which raws are
     * sent by default and the only address from which packets are
     * received.
     * @param addr The address on which to "connect".
     * @return @em true on success, @em false on failure
     */
    result<> connect(const ADDR& addr) { return base::connect(addr); }

    // ----- I/O -----

    /**
     * Sends a message to the socket at the specified address.
     * @param buf The data to send.
     * @param n The number of bytes in the data buffer.
     * @param flags The option bit flags. See send(2).
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const void* buf, size_t n, int flags, const ADDR& addr) {
        return base::send_to(buf, n, flags, addr);
    }
    /**
     * Sends a string to the socket at the specified address.
     * @param s The string to send.
     * @param flags The flags. See send(2).
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const std::string& s, int flags, const ADDR& addr) {
        return base::send_to(s, flags, addr);
    }
    /**
     * Sends a message to another socket.
     * @param buf The data to send.
     * @param n The number of bytes in the data buffer.
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const void* buf, size_t n, const ADDR& addr) {
        return base::send_to(buf, n, 0, addr);
    }
    /**
     * Sends a string to another socket.
     * @param s The string to send.
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const std::string& s, const ADDR& addr) {
        return base::send_to(s, addr);
    }
    /**
     * Receives a message on the socket.
     * @param buf Buffer to get the incoming data.
     * @param n The number of bytes to read.
     * @param flags The option bit flags. See send(2).
     * @param srcAddr Receives the address of the peer that sent
     *  			the message
     * @return The number of bytes read or @em -1 on error.
     */
    result<size_t> recv_from(void* buf, size_t n, int flags, ADDR* srcAddr) {
        return base::recv_from(buf, n, flags, srcAddr);
    }
    /**
     * Receives a message on the socket.
     * @param buf Buffer to get the incoming data.
     * @param n The number of bytes to read.
     * @param srcAddr Receives the address of the peer that sent
     *  			the message
     * @return The number of bytes read or @em -1 on error.
     */
    result<size_t> recv_from(void* buf, size_t n, ADDR* srcAddr = nullptr) {
        return base::recv_from(buf, n, srcAddr);
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_raw_socket_h
