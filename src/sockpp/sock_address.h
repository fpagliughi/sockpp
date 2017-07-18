/**
 * @file address.h
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

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Generic socket address.
 * This is a non-owning reference.
 */
struct sock_address
{
	/** Pointer to the address */
	const sockaddr* addr;
	/** Length of the address (in bytes) */
	socklen_t len;

	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address() : addr(nullptr), len(0) {}
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address(const sockaddr* a, socklen_t ln) : addr(a), len(ln) {}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

#endif		// __sockpp_sock_address_h

