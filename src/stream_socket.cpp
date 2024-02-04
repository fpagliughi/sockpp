// stream_socket.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2023 Frank Pagliughi
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

#include <algorithm>
#include <memory>

#include "sockpp/error.h"

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// Creates a stream socket for the given domain/protocol.

result<stream_socket> stream_socket::create(int domain, int protocol /*=0*/) {
    if (auto res = create_handle(domain, protocol); !res)
        return res.error();
    else
        return stream_socket{res.value()};
}

// --------------------------------------------------------------------------
// Reads from the socket. Note that we use ::recv() rather then ::read()
// because many non-*nix operating systems make a distinction.

result<size_t> stream_socket::read(void* buf, size_t n) {
#if defined(_WIN32)
    auto cbuf = reinterpret_cast<char*>(buf);
    return check_res<ssize_t, size_t>(::recv(handle(), cbuf, int(n), 0));
#else
    return check_res<ssize_t, size_t>(::recv(handle(), buf, n, 0));
#endif
}

// --------------------------------------------------------------------------
// Attempts to read the requested number of bytes by repeatedly calling
// read() until it has the data or an error occurs.
//
result<size_t> stream_socket::read_n(void* buf, size_t n) {
    uint8_t* b = reinterpret_cast<uint8_t*>(buf);
    size_t nx = 0;

    while (nx < n) {
        auto res = read(b + nx, n - nx);
        if (!res && res != errc::interrupted)
            return res.error();

        nx += size_t(res.value());
    }

    return nx;
}

// --------------------------------------------------------------------------

result<size_t> stream_socket::read(const std::vector<iovec>& ranges) {
    if (ranges.empty())
        return 0;

#if !defined(_WIN32)
    return check_res<ssize_t, size_t>(::readv(handle(), ranges.data(), int(ranges.size())));
#else
    std::vector<WSABUF> bufs;
    for (const auto& iovec : ranges) {
        bufs.push_back({static_cast<ULONG>(iovec.iov_len), static_cast<CHAR*>(iovec.iov_base)}
        );
    }

    DWORD flags = 0, nread = 0, nbuf = DWORD(bufs.size());

    auto ret = ::WSARecv(handle(), bufs.data(), nbuf, &nread, &flags, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
        return result<size_t>::from_last_error();
    return size_t(nread);
#endif
}

// --------------------------------------------------------------------------

result<> stream_socket::read_timeout(const microseconds& to) {
    auto tv =
#if defined(_WIN32)
        DWORD(duration_cast<milliseconds>(to).count());
#else
        to_timeval(to);
#endif
    return set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

// --------------------------------------------------------------------------

result<size_t> stream_socket::write(const void* buf, size_t n) {
#if defined(_WIN32)
    auto cbuf = reinterpret_cast<const char*>(buf);
    return check_res<ssize_t, size_t>(::send(handle(), cbuf, int(n), 0));
#else
    return check_res<ssize_t, size_t>(::send(handle(), buf, n, 0));
#endif
}

// --------------------------------------------------------------------------

result<size_t> stream_socket::write_n(const void* buf, size_t n) {
    const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
    size_t nx = 0;

    while (nx < n) {
        auto res = write(b + nx, n - nx);
        if (!res && res != errc::interrupted)
            return res.error();
        nx += size_t(res.value());
    }

    return nx;
}

// --------------------------------------------------------------------------

result<size_t> stream_socket::write(const std::vector<iovec>& ranges) {
#if !defined(_WIN32)
    return check_res<ssize_t, size_t>(::writev(handle(), ranges.data(), int(ranges.size())));
#else
    std::vector<WSABUF> bufs;
    for (const auto& iovec : ranges) {
        bufs.push_back({static_cast<ULONG>(iovec.iov_len), static_cast<CHAR*>(iovec.iov_base)}
        );
    }

    DWORD nwritten = 0, nmsg = DWORD(bufs.size());

    if (::WSASend(handle(), bufs.data(), nmsg, &nwritten, 0, nullptr, nullptr) ==
        SOCKET_ERROR)
        return result<size_t>::from_last_error();
    return size_t(nwritten);
#endif
}

// --------------------------------------------------------------------------

result<> stream_socket::write_timeout(const microseconds& to) {
    auto tv =
#if defined(_WIN32)
        DWORD(duration_cast<milliseconds>(to).count());
#else
        to_timeval(to);
#endif

    return set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
