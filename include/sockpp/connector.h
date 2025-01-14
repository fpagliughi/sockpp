/**
 * @file connector.h
 *
 * Class for creating client-side streaming connections.
 *
 * @author  Frank Pagliughi
 * @author  SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date    December 2014
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2023 Frank Pagliughi
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

#ifndef __sockpp_connector_h
#define __sockpp_connector_h

#include "sockpp/sock_address.h"
#include "sockpp/stream_socket.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to create a client stream connection.
 * This is a base class for creating active, streaming sockets that initiate
 * connections to a server. It can be used to derive classes that implement
 * TCP on IPv4 or IPv6.
 */
class connector : public stream_socket
{
    /** The base class */
    using base = stream_socket;

    // Non-copyable
    connector(const connector&) = delete;
    connector& operator=(const connector&) = delete;

    /** Recreate the socket with a new handle, closing any old one. */
    result<> recreate(const sock_address& addr);

public:
    /**
     * Creates an unconnected connector.
     */
    connector() noexcept {}
    /**
     * Creates the connector and attempts to connect to the specified
     * address.
     * @param addr The remote server address.
     * @throws std::system_error on failure
     */
    connector(const sock_address& addr) {
        if (auto res = connect(addr); !res)
            throw std::system_error{res.error()};
    }
    /**
     * Creates the connector and attempts to connect to the specified
     * address.
     * @param addr The remote server address.
     * @param ec The error code on failure.
     */
    connector(const sock_address& addr, error_code& ec) noexcept {
        ec = connect(addr).error();
    }
    /**
     * Creates the connector and attempts to connect to the specified
     * server, with a timeout.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @throws std::system_error on failure
     */
    template <class Rep, class Period>
    connector(const sock_address& addr, const duration<Rep, Period>& relTime) {
        if (auto res = connect(addr, microseconds(relTime)); !res)
            throw std::system_error{res.error()};
    }
    /**
     * Creates the connector and attempts to connect to the specified
     * server, with a timeout.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @param ec The error code on failure
     */
    template <class Rep, class Period>
    connector(
        const sock_address& addr, const duration<Rep, Period>& relTime, error_code& ec
    ) noexcept {
        ec = connect(addr, microseconds(relTime)).error();
    }
    /**
     * Creates the connector and attempts to connect to the specified
     * address, with a timeout.
     * If the operation times out, the \ref last_error will be set to
     * `timed_out`.
     * @param addr The remote server address.
     * @param t The duration after which to give up. Zero means never.
     * @throws std::system_error on failure
     */
    connector(const sock_address& addr, std::chrono::milliseconds t) {
        if (auto res = connect(addr, t); !res)
            throw std::system_error{res.error()};
    }
    /**
     * Creates the connector and attempts to connect to the specified
     * address, with a timeout.
     * If the operation times out, the \ref last_error will be set to
     * `timed_out`.
     * @param addr The remote server address.
     * @param t The duration after which to give up. Zero means never.
     */
    connector(const sock_address& addr, milliseconds t, error_code& ec) noexcept {
        ec = connect(addr, t).error();
    }
    /**
     * Move constructor.
     * Creates a connector by moving the other connector to this one.
     * @param conn Another connector.
     */
    connector(connector&& conn) noexcept : base(std::move(conn)) {}
    /**
     * Move assignment.
     * @param rhs The other connector to move into this one.
     * @return A reference to this object.
     */
    connector& operator=(connector&& rhs) noexcept {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Attempts to connect to the specified server.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
     * @param addr The remote server address.
     * @return The error code on failure.
     */
    result<> connect(const sock_address& addr);
    /**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
     * If the operation times out, the @ref error will be `errc::timed_out`.
     * @param addr The remote server address.
     * @param timeout The duration after which to give up. Zero means never.
     * @return The error code on failure.
     */
    result<> connect(const sock_address& addr, microseconds timeout);
    /**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
     * If the operation times out, the @ref last_error will be set to
     * `timed_out`.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @return The error code on failure
     */
    template <class Rep, class Period>
    result<> connect(const sock_address& addr, const duration<Rep, Period>& relTime) {
        return connect(addr, microseconds(relTime));
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to create a client TCP connection.
 */
template <typename STREAM_SOCK, typename ADDR = typename STREAM_SOCK::addr_t>
class connector_tmpl : public connector
{
    /** The base class */
    using base = connector;

    // Non-copyable
    connector_tmpl(const connector_tmpl&) = delete;
    connector_tmpl& operator=(const connector_tmpl&) = delete;

public:
    /** The type of streaming socket from the acceptor. */
    using stream_sock_t = STREAM_SOCK;
    /** The type of address for the connector. */
    using addr_t = ADDR;

    /**
     * Creates an unconnected connector.
     */
    connector_tmpl() noexcept {}
    /**
     * Creates the connector and attempts to connect to the specified
     * address.
     * @param addr The remote server address.
     * @throws std::system_error on failure.
     */
    connector_tmpl(const addr_t& addr) : base(addr) {}
    /**
     * Creates the connector and attempts to connect to the specified
     * address.
     * @param addr The remote server address.
     * @param ec The error code on failure.
     */
    connector_tmpl(const addr_t& addr, error_code& ec) noexcept : base(addr, ec) {}
    /**
     * Creates the connector and attempts to connect to the specified
     * server, with a timeout.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @throws std::system_error on failure
     */
    template <class Rep, class Period>
    connector_tmpl(const addr_t& addr, const duration<Rep, Period>& relTime)
        : base(addr, relTime) {}
    /**
     * Creates the connector and attempts to connect to the specified
     * server, with a timeout.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @param ec The error code on failure
     */
    template <class Rep, class Period>
    connector_tmpl(
        const addr_t& addr, const duration<Rep, Period>& relTime, error_code& ec
    ) noexcept
        : base(addr, relTime, ec) {}
    /**
     * Move constructor.
     * Creates a connector by moving the other connector to this one.
     * @param rhs Another connector.
     */
    connector_tmpl(connector_tmpl&& rhs) : base(std::move(rhs)) {}
    /**
     * Move assignment.
     * @param rhs The other connector to move into this one.
     * @return A reference to this object.
     */
    connector_tmpl& operator=(connector_tmpl&& rhs) {
        base::operator=(std::move(rhs));
        return *this;
    }
    /**
     * Gets the local address to which the socket is bound.
     * @return The local address to which the socket is bound.
     */
    addr_t address() const { return addr_t(base::address()); }
    /**
     * Gets the address of the remote peer, if this socket is connected.
     * @return The address of the remote peer, if this socket is connected.
     */
    addr_t peer_address() const { return addr_t(base::peer_address()); }
    /**
     * Binds the socket to the specified address.
     * This call is optional for a client connector, though it is rarely
     * used.
     * @param addr The address to which we get bound.
     * @return @em true on success, @em false on error
     */
    result<> bind(const addr_t& addr) { return base::bind(addr); }
    /**
     * Attempts to connects to the specified server.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
     * @param addr The remote server address.
     * @return @em true on success, @em false on error
     */
    result<> connect(const addr_t& addr) { return base::connect(addr); }
    /**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
     * If the operation times out, the @ref last_error will be set to
     * `timed_out`.
     * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
     * @return @em true on success, @em false on error
     */
    template <class Rep, class Period>
    result<> connect(
        const sock_address& addr, const std::chrono::duration<Rep, Period>& relTime
    ) {
        return base::connect(addr, std::chrono::microseconds(relTime));
    }
    /**
     * Attempts to connect to the server at the specified port.
     *
     * This requires that the socket's address type can be created from a
     * string host name and a port number.
     *
     * @param saddr The name of the host.
     * @param port The port number in native/host byte order.
     *
     * @return The result of the operation, with an error code on failure.
     */
    result<> connect(const string& saddr, in_port_t port) noexcept {
        auto addrRes = addr_t::create(saddr, port);
        if (!addrRes)
            return addrRes.error();
        if (auto res = connect(addrRes.value()); !res)
            return res;
        return none{};
    }
    /**
     * Attempts to connect to the server at the specified port.
     *
     * This requires that the socket's address type can be created from a
     * string host name and a port number.
     *
     * @param saddr The name of the host.
     * @param port The port number in native/host byte order.
     * @param relTime The duration after which to give up. Zero means never.
     *
     * @return The result of the operation, with an error code on failure.
     */
    template <class Rep, class Period>
    result<> connect(
        const string& saddr, in_port_t port, const duration<Rep, Period>& relTime
    ) noexcept {
        auto addrRes = addr_t::create(saddr, port);
        if (!addrRes)
            return addrRes.error();
        if (auto res = connect(addrRes.value(), relTime); !res)
            return res;
        return none{};
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_connector_h
