/**
 * @file sock_address.h
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

#ifndef __sockpp_sock_address_h
#define __sockpp_sock_address_h

#include "sockpp/platform.h"
#include <memory>
#include <string>
#include <cstring>
#include <sys/un.h>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a generic socket address.
 */
class sock_address : public sockaddr_un
{
	// NOTE: This class makes heavy use of the fact that it is completely
	// binary compatible with a sockaddr/sockaddr_un, and the same size as 
	// one of those structures. Do not add any other member variables, 
	// without going through the whole of the class to fixup! 

	/** Smart pointer hold address data */
	std::unique_ptr<uint8_t[]> addr_;
	/** The number of bytes of data */
	socklen_t len_;

public:
	/**
	 * Constructs an empty address.
	 */
	sock_address() : len_(0) {}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr Pointer to a sockaddr structure. 
	 * @param len The number of bytes in the sockaddr structure.
	 */
	sock_address(const sockaddr* addr, socklen_t len);
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr
	 */
	sock_address(const sock_address& addr);
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return bool
	 */
	bool is_set() const {
		return address_family() != AF_UNSPEC;
	}
	/**
	 * Gets the size of the address structure. 
	 */
	socklen_t size() const { return len_; }
	/**
	 * Gets the address family (socket domain) of this address. 
	 * @return The address family (socket domain) of this address. If no 
	 *  	   address has been assigned, this returns AF_UNSPEC (0).
	 */
	sa_family_t address_family() const;
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	const sockaddr* sockaddr_ptr() const {
		return reinterpret_cast<const sockaddr*>(addr_.get());
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	sockaddr* sockaddr_ptr() {
		return reinterpret_cast<sockaddr*>(addr_.get());
	}
	/**
	 * Gets a printable string for the address.
	 * @return A string representation of the address in the form 
	 *  	   'address:port'
	 */
	std::string to_string() const {
		// TODO: Does anything here make sense?
		return std::string();
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
inline bool operator==(const sock_address& lhs, const sock_address& rhs) {
	return (&lhs == &rhs) || (std::memcmp(&lhs, &rhs, sizeof(sock_address)) == 0);
}

/**
 * Inequality comparator.
 * This does a bitwise comparison.
 * @param lhs The first address to compare.
 * @param rhs The second address to compare.
 * @return @em true if they are binary different, @em false if they are
 *  	   equivalent.
 */
inline bool operator!=(const sock_address& lhs, const sock_address& rhs) {
	return !operator==(lhs, rhs);
}

/**
 * Stream inserter for the address. 
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const sock_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
};

#endif		// __sockpp_sock_address_h

