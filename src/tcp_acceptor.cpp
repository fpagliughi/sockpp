// tcp_acceptor.cpp
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

#include <cstring>
#include "sockpp/tcp_acceptor.h"

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// This attempts to open the acceptor, bind to the requested address, and
// start listening. On any error it will be sure to leave the underlying
// socket in an unopened/invalid state.
// If the acceptor appears to already be opened, this will quietly succeed
// without doing anything.

bool tcp_acceptor::open(const sockaddr* addr, socklen_t len, int queSize /*=DFLT_QUE_SIZE*/)
{
	// TODO: What to do if we are open but bound to a different address?
	if (is_open())
		return true;

	sa_family_t domain;
	if (!addr || len < sizeof(sa_family_t)
			|| 	(domain = *(reinterpret_cast<const sa_family_t*>(addr))) == AF_UNSPEC) {
		// TODO: Set last error for "address unspecified"
		return false;
	}

	socket_t h = tcp_socket::create(domain);
	if (!check_ret_bool(h))
		return false;

	reset(h);

	#if !defined(WIN32)
		if (domain == AF_INET) {
			int reuse = 1;
			if (!check_ret_bool(::setsockopt(h, SOL_SOCKET, SO_REUSEADDR,
											 &reuse, sizeof(int)))) {
				close();
				return false;
			}
		}
	#endif

	if (!bind(addr, len) || !listen(queSize)) {
		close();
		return false;
	}

	//addr_ = addr;
	return true;
}

// --------------------------------------------------------------------------

tcp_socket tcp_acceptor::accept(inet_address* clientAddr /*=nullptr*/)
{
	sockaddr* cli = reinterpret_cast<sockaddr*>(clientAddr);
	socklen_t len = cli ? sizeof(inet_address) : 0;
	socket_t  s = check_ret(::accept(handle(), cli, &len));
	return tcp_socket(s);
}

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

