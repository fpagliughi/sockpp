/**
 * @file inet_address.h
 *
 * Class for a TCP/IP socket address.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date February 2014
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

#ifndef __sockpp_inet_addr_h
#define __sockpp_inet_addr_h

#include "sockpp/sock_address.h"
#include <iostream>
#include <string>
#include <cstring>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////


/**
 * Class that represents an internet (IPv4) address.
 * This inherits from the IP-specific form of a socket address,
 * @em sockaddr_in.
 */
class inet_address : public sockaddr_in
{
	// NOTE: This class makes heavy use of the fact that it is completely
	// binary compatible with a sockaddr/sockaddr_in, and the same size as
	// one of those structures. Do not add any other member variables,
	// without going through the whole of the class to fixup!

	/**
	 * Sets the contents of this object to all zero.
	 */
	void zero() { std::memset(this, 0, sizeof(inet_address)); }

public:
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	inet_address() { zero(); }
	/**
     * Constructs an address for any iface using the specified port.
     * This is a convenient way for a server to specify an address that will
     * bind to all interfaces.
	 * @param port The port number in native/host byte order.
	 */
	explicit inet_address(in_port_t port) { create(in_addr_t(INADDR_ANY), port); }
	/**
	 * Constructs an address for the specified host using the specified
	 * port.
	 * @param addr The 32-bit host address in native/host byte order.
	 * @param port The port number in native/host byte order.
	 */
	inet_address(uint32_t addr, in_port_t port) { create(addr, port); }
	/**
	 * Constructs an address using the name of the host and the specified
	 * port. This attempts to resolve the host name to an address.
	 *
	 * @param saddr The name of the host.
	 * @param port The port number in native/host byte order.
	 */
	inet_address(const std::string& saddr, in_port_t port) {
		create(saddr, port);
	}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	inet_address(const sockaddr& addr) {
		std::memcpy(sockaddr_ptr(), &addr, sizeof(sockaddr_in));
	}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	inet_address(const sock_address& addr) {
		std::memcpy(sockaddr_ptr(), addr.sockaddr_ptr(), sizeof(sockaddr_in));
	}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	inet_address(const sockaddr_in& addr) {
		std::memcpy(sockaddr_in_ptr(), &addr, sizeof(sockaddr_in));
	}
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr The other address
	 */
	inet_address(const inet_address& addr) {
		std::memcpy(this, &addr, sizeof(inet_address));
	}
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return bool
	 */
	bool is_set() const;
	/**
	 * Attempts to resolve the host name into a 32-bit internet address.
	 * @param saddr The string host name.
	 * @return The internet address in network byte order.
	 */
	static in_addr_t resolve_name(const std::string& saddr);
	/**
	 * Creates the socket address using the specified host address and port
	 * number.
	 * @param addr The host address.
	 * @param port The host port number.
	 */
	void create(in_addr_t addr, in_port_t port);
	/**
	 * Creates the socket address using the specified host name and port
	 * number.
	 * @param saddr The string host name.
	 * @param port The port number in native/host byte order.
	 */
	void create(const std::string& saddr, in_port_t port);
	/**
	 * Gets the 32-bit internet address.
	 * @return The internet address in the local host's byte order.
	 */
	in_addr_t address() const { return ntohl(this->sin_addr.s_addr); }
	/**
	 * Gets a byte of the 32-bit Internet Address
	 * @param i The byte to read (0-3)
	 * @return The specified byte in the 32-bit Internet Address
	 */
	uint8_t operator[](int i) const {
		in_addr_t addr = address();
		return ((const uint8_t*)&addr)[i];
	}
	/**
	 * Gets the port number.
	 * @return The port number in native/host byte order.
	 */
	in_port_t port() const { return ntohs(this->sin_port); }
	/**
	 * Gets the size of this structure.
	 * This is equivalent to sizeof(this) but more convenient in some
	 * places.
	 * @return The size of this structure.
	 */
	socklen_t size() const { return (socklen_t) sizeof(sockaddr_in); }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	const sockaddr* sockaddr_ptr() const {
		return reinterpret_cast<const sockaddr*>(this);
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	sockaddr* sockaddr_ptr() {
		return reinterpret_cast<sockaddr*>(this);
	}
	/**
	 * Gets this address as a sock_address. 
	 * @return This address as a sock_address. 
	 */
	sock_address to_sock_address() const {
		return sock_address(sockaddr_ptr(), size());
	}
	/**
	 * Gets a const pointer to this object cast to a @em sockaddr_in.
	 * @return const sockaddr_in pointer to this object.
	 */
	const sockaddr_in* sockaddr_in_ptr() const {
		return static_cast<const sockaddr_in*>(this);
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr_in.
	 * @return sockaddr_in pointer to this object.
	 */
	sockaddr_in* sockaddr_in_ptr() {
		return static_cast<sockaddr_in*>(this);
	}
	/**
	 * Gets a printable string for the address.
	 * This gets the simple dot notation of the address as returned from 
	 * inet_ntop(). It does not attempt a host lookup.
	 * @return A string representation of the address in the form 
	 *  	   'address:port'
	 */
	std::string to_string() const {
        char buf[INET_ADDRSTRLEN];
        const char* str = inet_ntop(AF_INET, (void*) &(sockaddr_in_ptr()->sin_addr), buf, INET_ADDRSTRLEN);
		return std::string(str ? str : "<unknown>")
            + ":" + std::to_string(unsigned(port()));
	}
};

// --------------------------------------------------------------------------

/**
 * Equality comparator.
 * This does a bitwise comparison.
 * @param lhs The first address to compare.
 * @param rhs The second address to compare.
 * @return @em true if they are binary equivalent, @em false if not.
 */
inline bool operator==(const inet_address& lhs, const inet_address& rhs) {
	return (&lhs == &rhs) || (std::memcmp(&lhs, &rhs, sizeof(inet_address)) == 0);
}

/**
 * Inequality comparator.
 * This does a bitwise comparison.
 * @param lhs The first address to compare.
 * @param rhs The second address to compare.
 * @return @em true if they are binary different, @em false if they are
 *  	   equivalent.
 */
inline bool operator!=(const inet_address& lhs, const inet_address& rhs) {
	return !operator==(lhs, rhs);
}

/**
 * Stream inserter for the address. 
 * This uses the simple dot notation of the address as returned from 
 * inet_ntop(). It does not attempt a host lookup.
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const inet_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
};

#endif		// __sockpp_inet_addr_h

