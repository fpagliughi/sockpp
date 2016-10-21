// socket.cpp

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

#include "sockpp/socket.h"
#include "sockpp/exception.h"
#include <algorithm>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
int win32_closesocket(socket_t s)
{
	::closesocket(s);
	return 0;
}
#else
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

int socket::check_ret(int n) const
{
	lastErr_ = (n < 0) ? errno : 0;
	return n;
}

// --------------------------------------------------------------------------

int socket::get_last_error() const
{
	#if defined(WIN32)
		return lastErr_ = ::WSAGetLastError();
	#else
		return lastErr_;
	#endif
}

// --------------------------------------------------------------------------

void socket::close(socket_t h)
{
	#if defined(WIN32)
		::closesocket(h);
	#else
		::close(h);
	//#elif defined(NET_LWIP)
	//	lwip_close(s);
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
	//#elif defined(NET_LWIP)
	//	lwip_init();
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

void socket::reset(socket_t h /*=INVALID_SOCKET*/)
{
	socket_t oh = handle_;
	handle_ = h;
	if (oh != INVALID_SOCKET)
		close(oh);
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound.

int socket::address(inet_address& addr) const
{
	socklen_t len = sizeof(inet_address);
	return check_ret(::getsockname(handle_, addr.sockaddr_ptr(), &len));
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound. Throw an exception
// on error.

inet_address socket::address() const
{
	inet_address addr;
	if (address(addr) < 0)
		throw sys_error(lastErr_);
	return addr;
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is bound.

int socket::peer_address(inet_address& addr) const
{
	socklen_t len = sizeof(inet_address);
	return check_ret(::getpeername(handle_, addr.sockaddr_ptr(), &len));
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is bound. Throw an
// exception on error.

inet_address socket::peer_address() const
{
	inet_address addr;
	if (address(addr) < 0)
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
//								udp_socket
/////////////////////////////////////////////////////////////////////////////

#if !defined(WIN32)

udp_socket::udp_socket(in_port_t port) : socket(create())
{
	bind(inet_address(port));
}

udp_socket::udp_socket(const inet_address& addr) : socket(create())
{
	bind(addr);
}

// Opens a UDP socket. If it was already open, it just succeeds without
// doing anything.

/*
int udp_socket::open()
{
	if (!is_open())
		reset(create());

	return is_open() ? 0 : -1;
}
*/

int udp_socket::recvfrom(void* buf, size_t n, int flags, inet_address& addr)
{
	sockaddr_in sa;
	socklen_t alen = sizeof(inet_address);

	int ret = ::recvfrom(handle(), buf, n, flags, (sockaddr*) &sa, &alen);

	if (ret >= 0)
		addr = sa;
	else
		get_last_error();

	return ret;
}

#endif

/////////////////////////////////////////////////////////////////////////////
//								tcp_socket
/////////////////////////////////////////////////////////////////////////////

// Opens a TCP socket. If it was already open, it just succeeds without
// doing anything.

int tcp_socket::open()
{
	if (!is_open())
		reset(create());

	return is_open() ? 0 : -1;
}

// --------------------------------------------------------------------------
// Reads from the socket. Note that we use ::recv() rather then ::read()
// because many non-*nix operating systems make a distinction.

int tcp_socket::read(void *buf, size_t n)
{
	return ::recv(handle(), (char*) buf, n, 0);
}

// --------------------------------------------------------------------------
// Attempts to read the requested number of bytes by repeatedly calling
// read() until it has the data or an error occurs.
//

int tcp_socket::read_n(void *buf, size_t n)
{
	size_t	nr = 0;
	int		nx = 0;

	uint8_t *b = reinterpret_cast<uint8_t*>(buf);

	while (nr < n) {
		if ((nx = read(b+nr, n-nr)) <= 0)
			break;

		nr += nx;
	}

	return (nr == 0 && nx < 0) ? nx : int(nr);
}

// --------------------------------------------------------------------------

bool tcp_socket::read_timeout(const microseconds& to)
{
	#if !defined(WIN32)
		timeval tv = to_timeval(to);
		return check_ret(::setsockopt(handle(), SOL_SOCKET, SO_RCVTIMEO,
									  &tv, sizeof(timeval))) == 0;
	#else
		return false;
	#endif
}

// --------------------------------------------------------------------------

int tcp_socket::write(const void *buf, size_t n)
{
	return ::send(handle(), (const char*) buf, n , 0);
}

// --------------------------------------------------------------------------
// Attempts to write the entire buffer by repeatedly calling write() until
// either all of the data is sent or an error occurs.

int tcp_socket::write_n(const void *buf, size_t n)
{
	size_t	nw = 0;
	int		nx = 0;

	const uint8_t *b = reinterpret_cast<const uint8_t*>(buf);

	while (nw < n) {
		if ((nx = write(b+nw, n-nw)) <= 0)
			break;

		nw += nx;
	}

	return (nw == 0 && nx < 0) ? nx : int(nw);
}

// --------------------------------------------------------------------------

bool tcp_socket::write_timeout(const microseconds& to)
{
	#if !defined(WIN32)
		timeval tv = to_timeval(to);
		return check_ret(::setsockopt(handle(), SOL_SOCKET, SO_SNDTIMEO,
									  &tv, sizeof(timeval))) == 0;
	#else
		return false;
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

