// tcp_acceptor.cpp
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (C) 2014 Frank Pagliughi
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

int tcp_acceptor::open(const inet_address& addr, int queSize /*=DFLT_QUE_SIZE*/)
{
	// TODO: What to do if we are open but bound to a different address?
	if (is_open())
		return 0;

	// TODO: Set errno?
	if (!addr.is_set())
		return -1;

	socket_t h = tcp_socket::create();
	if (h < 0) {
		get_last_error();
		return -1;
	}

	reset(h);

	int ret = -1;

	#if !defined(WIN32)
		int reuse = 1;
		if ((ret = ::setsockopt(h, SOL_SOCKET, SO_REUSEADDR, 
								&reuse, sizeof(int))) < 0) {
			get_last_error();
			close();
			return ret;
		}
	#endif	
	
	if ((ret=bind(addr)) < 0 || (ret=listen(queSize)) < 0) {
		get_last_error();
		close();
		return ret;
	}

	addr_ = addr;
	return 0;
}

// --------------------------------------------------------------------------

tcp_socket tcp_acceptor::accept()
{
	inet_address	clientAddr;
	socklen_t	len = sizeof(inet_address);

	socket_t s = ::accept(handle(), clientAddr.sockaddr_ptr(), &len);
	if (s < 0)
		get_last_error();

	return tcp_socket(s);
}
// --------------------------------------------------------------------------

tcp_socket tcp_acceptor::accept(inet_address* clientAddr)
{
	if (!clientAddr)
		return accept();

	socklen_t len = sizeof(inet_address);
	socket_t  s = ::accept(handle(), clientAddr->sockaddr_ptr(), &len);
	if (s < 0)
		get_last_error();

	return tcp_socket(s);
}

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

