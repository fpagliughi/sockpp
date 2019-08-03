// inet_address.cpp
//
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

#include "sockpp/inet_address.h"

using namespace std;

namespace sockpp {

// --------------------------------------------------------------------------

bool inet_address::is_set() const
{
	const uint8_t* b = reinterpret_cast<const uint8_t*>(this);

	for (size_t i=0; i<sizeof(inet_address); ++i) {
		if (b[i] != 0)
			return true;
	}
	return false;
}

// --------------------------------------------------------------------------

in_addr_t inet_address::resolve_name(const std::string& saddr)
{
	#if defined(NET_LWIP)
		return in_addr_t(0);
	#endif

	#if !defined(WIN32)
		in_addr ia;
		if (::inet_aton(saddr.c_str(), &ia) != 0)
			return ia.s_addr;
	#endif

	// On error this sets h_error (not errno). Errors could be 
	// HOST_NOT_FOUND, NO_ADDRESS, etc.
	hostent *host = ::gethostbyname(saddr.c_str());
	return (host) ? *((in_addr_t*) host->h_addr_list[0]) : in_addr_t(0);
}

// --------------------------------------------------------------------------

void inet_address::create(uint32_t addr, in_port_t port)
{
	zero();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = htonl(addr);
	addr_.sin_port = htons(port);
}

// --------------------------------------------------------------------------

void inet_address::create(const std::string& saddr, in_port_t port)
{
	zero();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = resolve_name(saddr.c_str());
	addr_.sin_port = htons(port);
}

/////////////////////////////////////////////////////////////////////////////

ostream& operator<<(ostream& os, const inet_address& addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* str = inet_ntop(AF_INET, (void*) &(addr.sockaddr_in_ptr()->sin_addr), buf, INET_ADDRSTRLEN);
	os << (str ? str : "<unknown>") << ":" << unsigned(addr.port());
	return os;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

