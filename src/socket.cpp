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
#include <cstring>
#include <fcntl.h>

// Used to explicitly ignore the returned value of a function call.
#define ignore_result(x) if (x) {}

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
// Some aux functions

timeval to_timeval(const microseconds& dur)
{
	const seconds sec = duration_cast<seconds>(dur);

	timeval tv;
	#if defined(_WIN32)
		tv.tv_sec  = long(sec.count());
	#else
		tv.tv_sec  = time_t(sec.count());
	#endif
	tv.tv_usec = suseconds_t(duration_cast<microseconds>(dur - sec).count());
	return tv;
}

/////////////////////////////////////////////////////////////////////////////
//								socket
/////////////////////////////////////////////////////////////////////////////

int socket::get_last_error()
{
	#if defined(_WIN32)
		return ::WSAGetLastError();
	#else
		int err = errno;
		return err;
	#endif
}

// --------------------------------------------------------------------------

bool socket::close(socket_t h)
{
	#if defined(_WIN32)
		return ::closesocket(h) >= 0;
	#else
		return ::close(h) >= 0;
	#endif
}

// --------------------------------------------------------------------------
// Closes the socket and updates the last error on failure.

// --------------------------------------------------------------------------

void socket::initialize()
{
	#if defined(_WIN32)
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
	#if defined(_WIN32)
		::WSACleanup();
	#endif
}

// --------------------------------------------------------------------------

socket socket::create(int domain, int type, int protocol /*=0*/)
{
	socket sock(::socket(domain, type, protocol));
	if (!sock)
		sock.clear(get_last_error());
	return sock;
}

// --------------------------------------------------------------------------

socket socket::clone() const
{
	socket_t h = INVALID_SOCKET;
	#if defined(_WIN32)
		WSAPROTOCOL_INFO protInfo;
		if (::WSADuplicateSocket(handle_, ::GetCurrentProcessId(), &protInfo) == 0)
			h = ::WSASocket(AF_INET, SOCK_STREAM, 0, &protInfo, 0, WSA_FLAG_OVERLAPPED);
		// TODO: Set lastErr_ on failure
	#else
		h = ::dup(handle_);
	#endif

	return socket(h); 
}

// --------------------------------------------------------------------------

std::tuple<socket, socket> socket::pair(int domain, int type, int protocol /*=0*/)
{
	socket sock0, sock1;

	#if !defined(_WIN32)
		int sv[2];
		int ret = ::socketpair(domain, type, protocol, sv);

		if (ret == 0) {
			sock0.reset(sv[0]);
			sock1.reset(sv[1]);
		}
		else {
			int err = get_last_error();
			sock0.clear(err);
			sock1.clear(err);
		}
	#else
		sock0.clear(ENOTSUP);
		sock1.clear(ENOTSUP);
	#endif

	// TODO: Should we set an "unsupported" error on Windows?

	return std::make_tuple<socket, socket>(std::move(sock0), std::move(sock1));
}

// --------------------------------------------------------------------------

void socket::reset(socket_t h /*=INVALID_SOCKET*/)
{
	socket_t oh = handle_;
	handle_ = h;
	if (oh != INVALID_SOCKET)
		close(oh);
	clear();
}

// --------------------------------------------------------------------------
// Binds the socket to the specified address.

bool socket::bind(const sock_address& addr)
{
	return check_ret_bool(::bind(handle_, addr.sockaddr_ptr(), addr.size()));
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound.

sock_address_any socket::address() const
{
	auto addrStore = sockaddr_storage{};
	socklen_t len = sizeof(sockaddr_storage);

	if (!check_ret_bool(::getsockname(handle_,
				reinterpret_cast<sockaddr*>(&addrStore), &len)))
		return sock_address_any{};

	return sock_address_any(addrStore, len);
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is connected.

sock_address_any socket::peer_address() const
{
	auto addrStore = sockaddr_storage{};
	socklen_t len = sizeof(sockaddr_storage);

	if (!check_ret_bool(::getpeername(handle_,
				reinterpret_cast<sockaddr*>(&addrStore), &len)))
		return sock_address_any{};

	return sock_address_any(addrStore, len);
}

// --------------------------------------------------------------------------

bool socket::get_option(int level, int optname, void* optval, socklen_t* optlen) const
{
	#if defined(_WIN32)
		int len = static_cast<int>(*optlen);
		return check_ret_bool(::getsockopt(handle_, level, optname,
										   static_cast<char*>(optval), &len));
		*optlen = static_cast<socklen_t>(len);
	#else
		return check_ret_bool(::getsockopt(handle_, level, optname, optval, optlen));
	#endif
}

// --------------------------------------------------------------------------

bool socket::set_option(int level, int optname, const void* optval, socklen_t optlen)
{
	#if defined(_WIN32)
		return check_ret_bool(::setsockopt(handle_, level, optname, 
										   static_cast<const char*>(optval), 
										   static_cast<int>(optlen)));
	#else
		return check_ret_bool(::setsockopt(handle_, level, optname, optval, optlen));
	#endif
}

/// --------------------------------------------------------------------------

bool socket::set_non_blocking(bool on /*=true*/)
{
	#if defined(_WIN32)
		unsigned long mode = on ? 1 : 0;
		return check_ret_bool(::ioctlsocket(handle_, FIONBIO, &mode));
	#else
		/**
		 * TODO: Consider a generic function:
		 *   bool set_flag(int flag, bool on=true);
		 * Used like:
		 *   set_flag(O_NONBLOCK, on);
		 */
		int flags = ::fcntl(handle_, F_GETFL, 0);

		if (flags == -1) {
			set_last_error();
			return false;
		}
		flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);

		if (::fcntl(handle_, F_SETFL, flags) == -1) {
			set_last_error();
			return false;
		}
		return true;
	#endif
}

// --------------------------------------------------------------------------
// Gets a description of the last error encountered.

std::string socket::error_str(int err)
{
	return sys_error::error_str(err);
}

// --------------------------------------------------------------------------
// Shuts down all or part of the connection.

bool socket::shutdown(int how /*=SHUT_RDWR*/)
{
	return check_ret_bool(::shutdown(handle_, how));
}

// --------------------------------------------------------------------------
// Closes the socket and updates the last error on failure.

bool socket::close()
{
	if (handle_ != INVALID_SOCKET) {
		if (!close(release())){
			set_last_error();
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

