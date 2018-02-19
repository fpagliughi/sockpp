// socket.cpp
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

#include "sockpp/socket.h"
#include "sockpp/exception.h"
#include <algorithm>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
// Some platform-specific functions

#if !defined(WIN32)
timeval to_timeval(const microseconds& dur)
{
	const seconds sec = duration_cast<seconds>(dur);

	timeval tv;
    tv.tv_sec  = sec.count();
    tv.tv_usec = duration_cast<microseconds>(dur - sec).count();
	return tv;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//								socket
/////////////////////////////////////////////////////////////////////////////

int socket::get_last_error()
{
	#if defined(WIN32)
		return ::WSAGetLastError();
	#else
		int err = errno;
		return err;
	#endif
}

// --------------------------------------------------------------------------

void socket::close(socket_t h)
{
	#if defined(WIN32)
		::closesocket(h);
	#else
		::close(h);
	#endif
}

// --------------------------------------------------------------------------

void socket::initialize()
{
	#if defined(WIN32)
		WSADATA wsadata;
		::WSAStartup(MAKEWORD(2, 0), &wsadata);
	#else
		// Don't signal on socket write errors.
		::signal(SIGPIPE, SIG_IGN);
	#endif
}

// --------------------------------------------------------------------------

void socket::destroy()
{
	#if defined(WIN32)
		::WSACleanup();
	#endif
}

// --------------------------------------------------------------------------

socket socket::clone() 
{
	socket_t h = INVALID_SOCKET;
	#if defined(WIN32)
		WSAPROTOCOL_INFO protInfo;
		if (::WSADuplicateSocket(handle_, ::GetCurrentProcessId(), &protInfo) != 0)
			h = ::WSASocket(AF_INET, SOCK_STREAM, 0, &protInfo, 0, WSA_FLAG_OVERLAPPED);
	#else
		h = ::dup(handle_);
	#endif

	return socket(h); 
}
// --------------------------------------------------------------------------

void socket::reset(socket_t h /*=INVALID_SOCKET*/)
{
	socket_t oh = handle_;
	handle_ = h;
	if (oh != INVALID_SOCKET)
		close(oh);
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound.

bool socket::address(inet_address& addr) const
{
	socklen_t len = sizeof(inet_address);
	return check_ret_bool(::getsockname(handle_, addr.sockaddr_ptr(), &len));
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound. Throw an exception
// on error.

inet_address socket::address() const
{
	inet_address addr;
	if (!address(addr))
		throw sys_error(lastErr_);
	return addr;
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is bound.

bool socket::peer_address(inet_address& addr) const
{
	socklen_t len = sizeof(inet_address);
	return check_ret_bool(::getpeername(handle_, addr.sockaddr_ptr(), &len));
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is bound. Throw an
// exception on error.

inet_address socket::peer_address() const
{
	inet_address addr;
	if (!peer_address(addr))
		throw sys_error(lastErr_);
	return addr;
}

// --------------------------------------------------------------------------
// Closes the socket

void socket::close()
{
	if (handle_ != INVALID_SOCKET) {
		socket_t h = release();
		close(h);
	}
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

