// socket.cpp
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

#include "sockpp/socket.h"

#include <fcntl.h>

#include <algorithm>
#include <cstring>

#include "sockpp/error.h"
#include "sockpp/version.h"

#if defined(SOCKPP_OPENSSL)
    #include <openssl/err.h>
    #include <openssl/ssl.h>
#endif

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
// Some aux functions

timeval to_timeval(const microseconds& dur) {
    const seconds sec = duration_cast<seconds>(dur);

    timeval tv;
#if defined(_WIN32)
    tv.tv_sec = long(sec.count());
#else
    tv.tv_sec = time_t(sec.count());
#endif
    tv.tv_usec = suseconds_t(duration_cast<microseconds>(dur - sec).count());
    return tv;
}

/////////////////////////////////////////////////////////////////////////////
//							socket_initializer
/////////////////////////////////////////////////////////////////////////////

socket_initializer::socket_initializer() {
#if defined(_WIN32)
    WSADATA wsadata;
    ::WSAStartup(MAKEWORD(2, 0), &wsadata);
#else
    // Don't signal on socket write errors.
    ::signal(SIGPIPE, SIG_IGN);
#endif

#if defined(SOCKPP_OPENSSL)
    SSL_library_init();
    SSL_load_error_strings();
#endif
}

socket_initializer::~socket_initializer() {
#if defined(_WIN32)
    ::WSACleanup();
#endif
}

// --------------------------------------------------------------------------

void initialize() { socket_initializer::initialize(); }

/////////////////////////////////////////////////////////////////////////////
//									socket
/////////////////////////////////////////////////////////////////////////////

result<> socket::close(socket_t h) noexcept {
#if defined(_WIN32)
    return check_res_none(::closesocket(h));
#else
    return check_res_none(::close(h));
#endif
}

// --------------------------------------------------------------------------

result<socket> socket::create(int domain, int type, int protocol /*=0*/) noexcept {
    if (auto res = check_socket(::socket(domain, type, protocol)); !res)
        return res.error();
    else
        return socket(res.value());
}

// --------------------------------------------------------------------------

result<socket> socket::clone() const {
    socket_t h = INVALID_SOCKET;
#if defined(_WIN32)
    WSAPROTOCOL_INFOW protInfo;
    if (::WSADuplicateSocketW(handle_, ::GetCurrentProcessId(), &protInfo) == 0)
        h = check_socket(
                ::WSASocketW(AF_INET, SOCK_STREAM, 0, &protInfo, 0, WSA_FLAG_OVERLAPPED)
        )
                .value();
#else
    h = ::dup(handle_);
#endif

    if (auto res = check_socket(h); !res)
        return res.error();

    return socket(h);
}

// --------------------------------------------------------------------------

#if !defined(_WIN32)

result<int> socket::get_flags() const { return check_res(::fcntl(handle_, F_GETFL, 0)); }

result<> socket::set_flags(int flags) {
    return check_res_none(::fcntl(handle_, F_SETFL, flags));
}

result<> socket::set_flag(int flag, bool on /*=true*/) {
    auto res = get_flags();
    if (!res) {
        return res.error();
    }

    int flags = res.value();
    flags = on ? (flags | flag) : (flags & ~flag);
    return set_flags(flags);
}

// TODO: result<bool>?
bool socket::is_non_blocking() const {
    auto res = get_flags();
    return (res) ? ((res.value() & O_NONBLOCK) != 0) : false;
}

#endif

// --------------------------------------------------------------------------

result<std::tuple<socket, socket>>
socket::pair(int domain, int type, int protocol /*=0*/) noexcept {
    result<std::tuple<socket, socket>> res;
    socket sock0, sock1;

#if !defined(_WIN32)
    int sv[2];

    if (::socketpair(domain, type, protocol, sv) == 0) {
        res = std::make_tuple<socket, socket>(socket{sv[0]}, socket{sv[1]});
    }
    else {
        res = result<std::tuple<socket, socket>>::from_last_error();
    }
#else
    (void)domain;
    (void)type;
    (void)protocol;

    res = errc::function_not_supported;
#endif

    return res;
}

// --------------------------------------------------------------------------

void socket::reset(socket_t h /*=INVALID_SOCKET*/) noexcept {
    if (h != handle_) {
        std::swap(h, handle_);
        if (h != INVALID_SOCKET)
            close(h);
    }
}

// --------------------------------------------------------------------------
// Binds the socket to the specified address.

result<> socket::bind(const sock_address& addr, int reuse) noexcept {
    if (reuse) {
#if defined(_WIN32) || defined(__CYGWIN__)
        if (reuse != SO_REUSEADDR) {
#else
        if (reuse != SO_REUSEADDR && reuse != SO_REUSEPORT) {
#endif
            return errc::invalid_argument;
        }

        if (auto res = set_option(SOL_SOCKET, reuse, true); !res) {
            return res;
        }
    }

    return check_res_none(::bind(handle_, addr.sockaddr_ptr(), addr.size()));
}

// --------------------------------------------------------------------------
// Gets the local address to which the socket is bound.

sock_address_any socket::address() const {
    auto addrStore = sockaddr_storage{};
    socklen_t len = sizeof(sockaddr_storage);

    // TODO: Return the result
    auto res =
        check_res(::getsockname(handle_, reinterpret_cast<sockaddr*>(&addrStore), &len));
    if (!res)
        return sock_address_any{};

    return sock_address_any(addrStore, len);
}

// --------------------------------------------------------------------------
// Gets the address of the remote peer, if this socket is connected.

sock_address_any socket::peer_address() const {
    auto addrStore = sockaddr_storage{};
    socklen_t len = sizeof(sockaddr_storage);

    // TODO: Return the result?
    auto res =
        check_res(::getpeername(handle_, reinterpret_cast<sockaddr*>(&addrStore), &len));
    if (!res)
        return sock_address_any{};

    return sock_address_any(addrStore, len);
}

// --------------------------------------------------------------------------

result<> socket::get_option(
    int level, int optname, void* optval, socklen_t* optlen
) const noexcept {
    result<int> res;
#if defined(_WIN32)
    if (optval && optlen) {
        int len = static_cast<int>(*optlen);
        res = check_res(
            ::getsockopt(handle_, level, optname, static_cast<char*>(optval), &len)
        );
        if (res) {
            *optlen = static_cast<socklen_t>(len);
        }
    }
#else
    res = check_res(::getsockopt(handle_, level, optname, optval, optlen));
#endif
    return (res) ? error_code{} : res.error();
}

// --------------------------------------------------------------------------

result<> socket::set_option(
    int level, int optname, const void* optval, socklen_t optlen
) noexcept {
#if defined(_WIN32)
    return check_res_none(
        ::setsockopt(
            handle_, level, optname, static_cast<const char*>(optval),
            static_cast<int>(optlen)
        )
    );
#else
    return check_res_none(::setsockopt(handle_, level, optname, optval, optlen));
#endif
}

/// --------------------------------------------------------------------------

result<> socket::set_non_blocking(bool on /*=true*/) {
#if defined(_WIN32)
    unsigned long mode = on ? 1 : 0;
    auto res = check_res(::ioctlsocket(handle_, FIONBIO, &mode));
    return (res) ? error_code{} : res.error();
#else
    return set_flag(O_NONBLOCK, on);
#endif
}

// --------------------------------------------------------------------------
// Shuts down all or part of the connection.

result<> socket::shutdown(int how /*=SHUT_RDWR*/) {
    if (handle_ == INVALID_SOCKET)
        return errc::invalid_argument;

    return check_res_none(::shutdown(handle_, how));
}

// --------------------------------------------------------------------------
// Closes the socket and updates the last error on failure.

result<> socket::close() {
    if (handle_ != INVALID_SOCKET) {
        return close(release());
    }
    return error_code{};
}

// --------------------------------------------------------------------------

result<size_t>
socket::recv_from(void* buf, size_t n, int flags, sock_address* srcAddr /*=nullptr*/) {
    sockaddr* p = srcAddr ? srcAddr->sockaddr_ptr() : nullptr;
    socklen_t len = srcAddr ? srcAddr->size() : 0;

    // TODO: Check returned length

#if defined(_WIN32)
    return check_res<ssize_t, size_t>(
        ::recvfrom(handle(), reinterpret_cast<char*>(buf), int(n), flags, p, &len)
    );
#else
    return check_res<ssize_t, size_t>(::recvfrom(handle(), buf, n, flags, p, &len));
#endif
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
