/**
 * @file datagram_socket.h
 *
 * Classes for datagram and UDP socket.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date December 2014
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
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

#ifndef __sockpp_datagram_socket_h
#define __sockpp_datagram_socket_h

#include "sockpp/socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
/// Class that wraps UDP sockets

#if !defined(WIN32)

class datagram_socket : public socket
{
protected:
	static socket_t create(int domain=AF_INET) {
		return (socket_t) ::socket(domain, SOCK_DGRAM, 0 /*IPPROTO_UDP*/);
	}

public:
	/**
	 * Creates an unbound UDP socket.
	 * This can be used as a client or later bound as a server socket.
	 */
	datagram_socket() : socket(create()) {}
	/**
	 * Creates a UDP socket and binds it to the specified port.
	 * @param port The port to bind.
	 */
	datagram_socket(in_port_t port);
	/**
	 * Creates a UDP socket and binds it to the address.
	 * @param addr The address to bind.
	 */
	datagram_socket(const inet_address& addr);
	/**
	 * Binds the socket to the local address.
	 * UDP sockets can bind to a local address/adapter to filter which
	 * incoming packets to receive.
	 * @param addr
	 * @return @em true on success, @em false on failure
	 */
	bool bind(const inet_address& addr) {
		return check_ret_bool(::bind(handle(), addr.sockaddr_ptr(),
									 sizeof(sockaddr_in)));
	}
	/**
	 * Connects the socket to the remote address.
	 * In the case of UDP sockets, this does not create an actual
	 * connection, but rather specified the address to which datagrams are
	 * sent by default, and the only address from which datagrams are
	 * received.
	 * @param addr
	 * @return @em true on success, @em false on failure
	 */
	bool connect(const inet_address& addr) {
		return check_ret_bool(::connect(handle(), addr.sockaddr_ptr(),
										sizeof(sockaddr_in)));
	}

	// ----- I/O -----

	/**
	 * Sends a UDP packet to the specified internet address.
	 * @param buf The date to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	int	sendto(const void* buf, size_t n, const inet_address& addr) {
		return check_ret(::sendto(handle(), buf, n, 0,
								  addr.sockaddr_ptr(), addr.size()));
	}
	int sendto(const std::string& s, const inet_address& addr) {
		return sendto(s.data(), s.length(), addr);
	}
	/**
	 * Sends a UDP packet to the specified internet address.
	 * @param buf The data to send.
	 * @param n The number of bytes in the data buffer.
	 * @param flags
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	int	sendto(const void* buf, size_t n, int flags, const inet_address& addr) {
		return check_ret(::sendto(handle(), buf, n, flags,
								  addr.sockaddr_ptr(), addr.size()));
	}
	int	sendto(const std::string& s, int flags, const inet_address& addr) {
		return sendto(s.data(), s.length(), flags, addr);
	}
	/**
	 * Sends a UDP packet to the default address.
	 * The socket should be connected before calling this
	 * @param buf The data to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return @em zero on success, @em -1 on failure.
	 */
	int	send(const void* buf, size_t n, int flags=0) {
		return check_ret(::send(handle(), buf, n, flags));
	}
	int	send(const std::string& s, int flags=0) {
		return send(s.data(), s.length(), flags);
	}
	/**
	 * Receives a UDP packet.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Gets the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recvfrom(void* buf, size_t n, int flags, inet_address& addr);
	/**
	 * Receives a UDP packet.
	 * @sa recvfrom
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Gets the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recvfrom(void* buf, size_t n, inet_address& addr) {
		return recvfrom(buf, n, 0, addr);
	}
	/**
	 * Receives a UDP packet.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recv(void* buf, size_t n, int flags=0) {
		return check_ret(::recv(handle(), buf, n, flags));
	}
};

#endif	// !WIN32

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_socket_h

