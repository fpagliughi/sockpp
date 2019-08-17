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

// Used to explicitly ignore the returned value of a function call.
#define ignore_result(x) if (x) {}

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
// Some platform-specific functions

#if !defined(_WIN32)
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
	#if defined(_WIN32)
		return ::WSAGetLastError();
	#else
		int err = errno;
		return err;
	#endif
}

// --------------------------------------------------------------------------

void socket::close(socket_t h)
{
	#if defined(_WIN32)
		::closesocket(h);
	#else
		::close(h);
	#endif
}

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

void socket::reset(socket_t h /*=INVALID_SOCKET*/)
{
	socket_t oh = handle_;
	handle_ = h;
	if (oh != INVALID_SOCKET)
		close(oh);
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound.
// Throw an exception on error.

sock_address_any socket::address() const
{
    sockaddr_storage addrStore;
	socklen_t len = sizeof(sockaddr_storage);
	check_ret(::getsockname(handle_,
        reinterpret_cast<sockaddr*>(&addrStore), &len));
    return sock_address_any(addrStore, len);
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is bound. Throw an
// exception on error.

sock_address_any socket::peer_address() const
{
    sockaddr_storage addrStore;
	socklen_t len = sizeof(sockaddr_storage);
	check_ret(::getpeername(handle_,
        reinterpret_cast<sockaddr*>(&addrStore), &len));
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

// --------------------------------------------------------------------------
// Gets a description of the last error encountered.

std::string socket::error_str(int err)
{
	#if defined(_WIN32)
        char buf[1024];
		buf[0] = '\0';
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buf, sizeof(buf), NULL);
        return std::string(buf);
    #else
        char buf[512];
        buf[0] = '\x0';

    	#ifdef _GNU_SOURCE
            return std::string(strerror_r(err, buf, sizeof(buf)));
        #else
            ignore_result(strerror_r(err, buf, sizeof(buf)));
            return std::string(buf);
        #endif
    #endif
}

// --------------------------------------------------------------------------
// Shuts down all or part of the connection.

void socket::shutdown(int how /*=SHUT_RDWR*/)
{
	::shutdown(handle_, how);
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

