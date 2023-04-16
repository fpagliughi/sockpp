// datagram_socket.cpp
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

#include "sockpp/datagram_socket.h"
#include "sockpp/exception.h"
#include <algorithm>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
//								datagram_socket
/////////////////////////////////////////////////////////////////////////////

datagram_socket::datagram_socket(const sock_address& addr)
{
    auto domain = addr.family();
    socket_t h = create_handle(domain);

    if (check_socket_bool(h)) {
        reset(h);
        if (!bind(addr)) {
            close();
            throw std::runtime_error("Failed to bind the socket");
        }
    }
}

// --------------------------------------------------------------------------

ssize_t datagram_socket::recv_from(void* buf, size_t n, int flags, sock_address* srcAddr /*=nullptr*/)
{
    sockaddr* p = srcAddr ? srcAddr->sockaddr_ptr() : nullptr;
    socklen_t len = srcAddr ? srcAddr->size() : 0;

    ssize_t ret;
    do {
        #if defined(_WIN32)
        ret = ::recvfrom(handle(), reinterpret_cast<char*>(buf), int(n), flags, p, &len);
        #else
        ret = ::recvfrom(handle(), buf, n, flags, p, &len);
        #endif
    } while (ret == -1 && last_error() == EINTR);

    if (ret > 0 && static_cast<size_t>(ret) < n) {
        // The received data is smaller than the buffer, so fill the rest of the buffer with zeroes.
        // This is to ensure that the remaining buffer is zeroed out.
        std::memset(static_cast<char*>(buf) + ret, 0, n - ret);
    }

    if (ret == -1) {
        // Handle the case where the receive call failed.
        int err = last_error();
        if (err == EAGAIN || err == EWOULDBLOCK) {
            // If the receive operation would block, return 0 to indicate that no data was received.
            return 0;
        } else {
            // If the error is not a blocking error, return -1 to indicate that an error occurred.
            return -1;
        }
    }

    return ret;
}


/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}
