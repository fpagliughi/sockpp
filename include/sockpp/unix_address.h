/**
 * @file unix_address.h
 *
 * Class for a UNIX-domain socket address.
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

#ifndef __sockpp_unix_addr_h
#define __sockpp_unix_addr_h

#include "sockpp/platform.h"
#include "sockpp/sock_address.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/un.h>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a UNIX domain address.
 * This inherits from the UNIX form of a socket address, @em sockaddr_un.
 */
class unix_address : public sockaddr_un
{
	static constexpr sa_family_t ADDRESS_FAMILY = AF_UNIX;
	// TODO: This only applies to Linux
	static constexpr size_t MAX_PATH_NAME = 108;

	// NOTE: This class makes heavy use of the fact that it is completely
	// binary compatible with a sockaddr/sockaddr_un, and the same size as 
	// one of those structures. Do not add any other member variables, 
	// without going through the whole of the class to fixup! 

	/**
	 * Sets the contents of this object to all zero.
	 */
	void zero() { 
		std::memset(this, 0, sizeof(unix_address));
		sun_family = ADDRESS_FAMILY;
	}

public:
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	unix_address() { zero(); }
	/**
	 * Constructs an address for the specified path.
	 * @param path The 
	 */
	unix_address(const std::string& path);
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The generic address
     * @throws std::invalid_argument if the address is not a UNIX-domain
     *            address (i.e. family is not AF_UNIX)
	 */
	explicit unix_address(const sockaddr& addr);
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	unix_address(const sock_address& addr) {
        // TODO: Maybe throw EINVAL if wrong size
		std::memcpy(sockaddr_ptr(), addr.sockaddr_ptr(), sizeof(sockaddr_un));
	}
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The other address
     * @throws std::invalid_argument if the address is not properly
     *            initialized as a UNIX-domain address (i.e. family is not
     *            AF_UNIX)
	 */
	unix_address(const sockaddr_un& addr);
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr The other address
	 */
	unix_address(const unix_address& addr) {
		std::memcpy(this, &addr, sizeof(unix_address));
	}
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return @em true if the address has been set, @em false otherwise.
	 */
	bool is_set() const { return sun_path[0] != '\0'; }
	/**
	 * Gets the path to which this address refers.
	 * @return The path to which this address refers.
	 */
	std::string path() const { return std::string(sun_path); }
	/**
	 * Gets the size of the address structure. 
	 * Note: In this implementation, this should return sizeof(this) but 
	 * more convenient in some places, and the implementation might change 
	 * in the future, so it might be more compatible with future revisions 
	 * to use this call. 
	 * @return The size of the address structure.
	 */
	socklen_t size() const { return (socklen_t) sizeof(sockaddr_un); }

    // TODO: Do we need a:
    //   create(path)
    // to mimic the inet_address behavior?

    /**
	 * Gets a pointer to this object cast to a const @em sockaddr.
	 * @return A pointer to this object cast to a const @em sockaddr.
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
	 * Gets a const pointer to this object cast to a @em sockaddr_un.
	 * @return const sockaddr_un pointer to this object.
	 */
	const sockaddr_un* sockaddr_un_ptr() const {
		return static_cast<const sockaddr_un*>(this);
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr_un.
	 * @return sockaddr_un pointer to this object.
	 */
	sockaddr_un* sockaddr_un_ptr() {
		return static_cast<sockaddr_un*>(this);
	}
    /**
     * Implicit conversion to an address reference.
     * @return Reference to the address.
     */
    operator sock_address_ref() const {
        return sock_address_ref(reinterpret_cast<const sockaddr*>(this),
                                sizeof(sockaddr_un));
    }
	/**
	 * Gets a printable string for the address.
	 * @return A string representation of the address in the form 
	 *  	   'unix:<path>'
	 */
	std::string to_string() const {
		return std::string("unix:") + std::string(sun_path);
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
inline bool operator==(const unix_address& lhs, const unix_address& rhs) {
	return (&lhs == &rhs) || (std::memcmp(&lhs, &rhs, sizeof(unix_address)) == 0);
}

/**
 * Inequality comparator.
 * This does a bitwise comparison.
 * @param lhs The first address to compare.
 * @param rhs The second address to compare.
 * @return @em true if they are binary different, @em false if they are
 *  	   equivalent.
 */
inline bool operator!=(const unix_address& lhs, const unix_address& rhs) {
	return !operator==(lhs, rhs);
}

/**
 * Stream inserter for the address. 
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const unix_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
};

#endif		// __sockpp_unix_addr_h

