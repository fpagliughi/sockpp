// stream_socket.cpp
//
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

#include "sockpp/stream_socket.h"
#include "sockpp/exception.h"
#include <algorithm>
#include <memory>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// Reads from the socket. Note that we use ::recv() rather then ::read()
// because many non-*nix operating systems make a distinction.

ssize_t stream_socket::read(void *buf, size_t n)
{
    #if defined(_WIN32)
        return check_ret(::recv(handle(), reinterpret_cast<char*>(buf),
								int(n), 0));
    #else
        return check_ret(::recv(handle(), buf, n, 0));
    #endif
}

ioresult stream_socket::read_r(void *buf, size_t n)
{
    #if defined(_WIN32)
        return ioresult(::recv(handle(), reinterpret_cast<char*>(buf),
                               int(n), 0));
    #else
        return ioresult(::recv(handle(), buf, n, 0));
    #endif
}

// --------------------------------------------------------------------------
// Attempts to read the requested number of bytes by repeatedly calling
// read() until it has the data or an error occurs.
//

ssize_t stream_socket::read_n(void *buf, size_t n)
{
	size_t	nr = 0;
	ssize_t	nx = 0;

	uint8_t *b = reinterpret_cast<uint8_t*>(buf);

	while (nr < n) {
		if ((nx = read(b+nr, n-nr)) < 0 && last_error() == EINTR)
			continue;

		if (nx <= 0)
			break;

		nr += nx;
	}

	return (nr == 0 && nx < 0) ? nx : ssize_t(nr);
}

ioresult stream_socket::read_n_r(void *buf, size_t n)
{
    ioresult result;
	uint8_t *b = reinterpret_cast<uint8_t*>(buf);

	while (result.count < n) {
        ioresult r = read_r(b + result.count, n - result.count);
		if (r.count == 0) {
            result.error = r.error;
			break;
        }
		result.count += r.count;
	}

	return result;
}

// --------------------------------------------------------------------------

bool stream_socket::read_timeout(const microseconds& to)
{
    auto tv = 
        #if defined(_WIN32)
            DWORD(duration_cast<milliseconds>(to).count());
        #else
            to_timeval(to);
        #endif
    return set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

// --------------------------------------------------------------------------

ssize_t stream_socket::write(const void *buf, size_t n)
{
    #if defined(_WIN32)
        return check_ret(::send(handle(), reinterpret_cast<const char*>(buf),
                                int(n) , 0));
    #else
        return check_ret(::send(handle(), buf, n , 0));
    #endif
}

ioresult stream_socket::write_r(const void *buf, size_t n)
{
    #if defined(_WIN32)
        return ioresult(::send(handle(), reinterpret_cast<const char*>(buf),
                               int(n) , 0));
    #else
        return ioresult(::send(handle(), buf, n , 0));
    #endif
}

// --------------------------------------------------------------------------
// Attempts to write the entire buffer by repeatedly calling write() until
// either all of the data is sent or an error occurs.

ssize_t stream_socket::write_n(const void *buf, size_t n)
{
	size_t	nw = 0;
	ssize_t	nx = 0;

	const uint8_t *b = reinterpret_cast<const uint8_t*>(buf);

	while (nw < n) {
		if ((nx = write(b+nw, n-nw)) < 0 && last_error() == EINTR)
			continue;

		if (nx <= 0)
			break;

		nw += nx;
	}

	return (nw == 0 && nx < 0) ? nx : ssize_t(nw);
}

ioresult stream_socket::write_n_r(const void *buf, size_t n)
{
    ioresult result;
	const uint8_t *b = reinterpret_cast<const uint8_t*>(buf);

	while (result.count < n) {
        ioresult r = write_r(b + result.count, n - result.count);
		if (r.count == 0) {
            result.error = r.error;
			break;
        }
		result.count += r.count;
	}

	return result;
}

// --------------------------------------------------------------------------

ssize_t stream_socket::write(const std::vector<iovec> &ranges) {
#if !defined(_WIN32)
    msghdr msg = {};
    msg.msg_iov = const_cast<iovec*>(ranges.data());
    msg.msg_iovlen = int(ranges.size());
    if (msg.msg_iovlen == 0)
        return 0;
    return check_ret(sendmsg(handle(), &msg, 0));
#else
	if(ranges.empty()) {
		return 0;
	}
	
	std::vector<WSABUF> buffers;
	for(const auto& iovec : ranges) {
		buffers.push_back({static_cast<ULONG>(iovec.iov_len), static_cast<CHAR FAR *>(iovec.iov_base)});
	}

	DWORD written = 0;
	ssize_t ret = check_ret(WSASend(handle(), buffers.data(), ULONG(buffers.size()), &written, 0, nullptr, nullptr));
	return ret == SOCKET_ERROR ? ret : written;
#endif
}


// --------------------------------------------------------------------------

bool stream_socket::write_timeout(const microseconds& to)
{
    auto tv = 
        #if defined(_WIN32)
            DWORD(duration_cast<milliseconds>(to).count());
        #else
            to_timeval(to);
        #endif

    return set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

    

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

