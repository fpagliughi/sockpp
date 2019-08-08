/**
 * @file datagram_socket.h
 *
 * Classes for datagram sockets.
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
// Copyright (c) 2014-2019 Frank Pagliughi
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

/**
 * Base class for datagram sockets.
 *
 * Datagram sockets are normally connectionless, where each packet is
 * individually routed and delivered.
 */
class datagram_socket : public socket
{
protected:
	static socket_t create(int domain) {
		return socket_t(::socket(domain, SOCK_DGRAM, 0));
	}

public:
	/**
	 * Creates an unbound datagram socket.
	 * This can be used as a client or later bound as a server socket.
	 */
	datagram_socket(int domain) : socket(create(domain)) {}
	/**
	 * Creates a UDP socket and binds it to the address.
	 * @param addr The address to bind.
	 */
	datagram_socket(const sock_address& addr);
	/**
	 * Binds the socket to the local address.
	 * Datagram sockets can bind to a local address/adapter to filter which
	 * incoming packets to receive.
	 * @param addr The address on which to bind.
	 * @return @em true on success, @em false on failure
	 */
	bool bind(const sock_address& addr) {
		return check_ret_bool(::bind(handle(), addr.sockaddr_ptr(),
                                     addr.size()));
	}
	/**
	 * Connects the socket to the remote address.
	 * In the case of datagram sockets, this does not create an actual
	 * connection, but rather specifies the address to which datagrams are
	 * sent by default and the only address from which packets are
	 * received.
	 * @param addr The address on which to "connect".
	 * @return @em true on success, @em false on failure
	 */
	bool connect(const sock_address& addr) {
		return check_ret_bool(::connect(handle(), addr.sockaddr_ptr(),
										addr.size()));
	}

	// ----- I/O -----

	/**
	 * Sends a message to the socket at the specified address.
	 * @param buf The data to send.
	 * @param n The number of bytes in the data buffer.
	 * @param flags The flags. See send(2).
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const void* buf, size_t n, int flags, const sock_address& addr) {
        #if defined(_WIN32)
            return check_ret(::sendto(handle(), reinterpret_cast<const char*>(buf), int(n), 
                                      flags, addr.sockaddr_ptr(), addr.size()));
        #else
            return check_ret(::sendto(handle(), buf, n, flags,
                                      addr.sockaddr_ptr(), addr.size()));
        #endif
	}
	/**
	 * Sends a string to the socket at the specified address.
	 * @param s The string to send.
	 * @param flags The flags. See send(2).
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const std::string& s, int flags, const sock_address& addr) {
		return send_to(s.data(), s.length(), flags, addr);
	}
	/**
	 * Sends a message to another socket.
	 * @param buf The data to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const void* buf, size_t n, const sock_address& addr) {
		return send_to(buf, n, 0, addr);
	}
	/**
	 * Sends a string to another socket.
	 * @param buf The string to send.
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const std::string& s, const sock_address& addr) {
		return send_to(s.data(), s.length(), 0, addr);
	}
	/**
	 * Sends a message to the socket at the default address.
	 * The socket should be connected before calling this.
	 * @param buf The date to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return @em zero on success, @em -1 on failure.
	 */
	ssize_t send(const void* buf, size_t n, int flags=0) {
        #if defined(_WIN32)
            return check_ret(::send(handle(), reinterpret_cast<const char*>(buf), 
                                    int(n), flags));
        #else
            return check_ret(::send(handle(), buf, n, flags));
        #endif
	}
	/**
	 * Sends a string to the socket at the default address.
	 * The socket should be connected before calling this
	 * @param s The string to send.
	 * @param addr The remote destination of the data.
	 * @return @em zero on success, @em -1 on failure.
	 */
	ssize_t send(const std::string& s, int flags=0) {
		return send(s.data(), s.length(), flags);
	}
	/**
	 * Receives a message on the socket.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Receives the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	ssize_t recv_from(void* buf, size_t n, int flags, sock_address& srcAddr);
	/**
	 * Receives a message on the socket.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Receives the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	ssize_t recv_from(void* buf, size_t n, sock_address& addr) {
		return recv_from(buf, n, 0, addr);
	}
	/**
	 * Receives a message on the socket.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @return The number of bytes read or @em -1 on error.
	 */
	ssize_t recv(void* buf, size_t n, int flags=0) {
        #if defined(_WIN32)
            return check_ret(::recv(handle(), reinterpret_cast<char*>(buf), 
                                    int(n), flags));
        #else
            return check_ret(::recv(handle(), buf, n, flags));
        #endif
	}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_datagram_socket_h

