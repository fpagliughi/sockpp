/**
 * @file sock_address.h
 *
 * Generic address class for sockpp.
 *
 * @author	Frank Pagliughi
 * @author	SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	June 2017
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

#ifndef __sockpp_sock_address_h
#define __sockpp_sock_address_h

#include "sockpp/platform.h"
#include <cstring>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Generic socket address reference.
 * This is a non-owning reference to a generic socket address. Each of the
 * concrete address classes should be able to implicitly convert itself to
 * one of these for base, generic, communication classes that can work with
 * any type of socket.
 */
class sock_address_ref
{
	/** Pointer to the address */
	const sockaddr* addr_;
	/** Length of the address (in bytes) */
	socklen_t sz_;

public:
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address_ref() : addr_(nullptr), sz_(0) {}
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address_ref(const sockaddr* a, socklen_t n) : addr_(a), sz_(n) {}
	/**
	 * Gets the size of this structure.
	 * This is equivalent to sizeof(this) but more convenient in some
	 * places.
	 * @return The size of this structure.
	 */
	socklen_t size() const { return sz_; }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	const sockaddr* sockaddr_ptr() const { return addr_; }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Generic socket address.
 *
 * This is a wrapper around `sockaddr_storage` which can hold any family
 * address.
 */
class sock_address
{
    /** Storage for any kind of socket address */
    sockaddr_storage addr_;
	/** Length of the address (in bytes) */
	socklen_t sz_;

	/**
	 * Sets the contents of this object to all zero.
	 */
	void zero() {
        std::memset(&addr_, 0, sizeof(sockaddr_storage));
    }

public:
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address() : sz_(0) { zero(); }
	/**
	 * Constructs an address.
	 */
	sock_address(const sockaddr* addr, socklen_t n) {
        // TODO: Check size of n n?
        std::memcpy(&addr_, addr, sz_ = n);
    }
	/**
     * Constructs a address.
	 */
	sock_address(const sockaddr_storage& addr, socklen_t n) {
        // TODO: Check size of n n?
        std::memcpy(&addr_, &addr, sz_ = n);
    }
	/**
	 * Gets the size of this structure.
	 * This is equivalent to sizeof(this) but more convenient in some
	 * places.
	 * @return The size of this structure.
	 */
	socklen_t size() const { return sz_; }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	const sockaddr* sockaddr_ptr() const {
		return reinterpret_cast<const sockaddr*>(&addr_);
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	sockaddr* sockaddr_ptr() {
		return reinterpret_cast<sockaddr*>(&addr_);
	}
    /**
     * Implicit conversion to an address reference.
     * @return Reference to the address.
     */
    operator sock_address_ref() const {
        return sock_address_ref(reinterpret_cast<const sockaddr*>(&addr_), sz_);
    }
};

inline bool operator==(const sock_address& lhs, const sock_address& rhs) {
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.sockaddr_ptr(), rhs.sockaddr_ptr(), lhs.size()) == 0;
}

inline bool operator!=(const sock_address& lhs, const sock_address& rhs) {
	return !operator==(lhs, rhs);
}

/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

#endif		// __sockpp_sock_address_h

