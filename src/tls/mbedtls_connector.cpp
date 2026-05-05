// mbedtls_connector.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2026 Frank Pagliughi
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

#include "sockpp/tls/mbedtls_connector.h"

#include "sockpp/connector.h"

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_connector::tls_connector(mbedtls_context& ctx, const sock_address& addr)
    : base{ctx, string{}} {
    connector tcp;
    if (auto res = tcp.connect(addr); !res)
        throw std::system_error{res.error()};
    if (auto res = tls_connect(std::move(tcp)); !res)
        throw std::system_error{res.error()};
}

tls_connector::tls_connector(
    mbedtls_context& ctx, const sock_address& addr, const string& hostname
)
    : base{ctx, hostname} {
    connector tcp;
    if (auto res = tcp.connect(addr); !res)
        throw std::system_error{res.error()};
    if (auto res = base::tls_connect(std::move(tcp)); !res)
        throw std::system_error{res.error()};
}

tls_connector::tls_connector(
    mbedtls_context& ctx, const sock_address& addr, const string& hostname, error_code& ec
) noexcept
    : base{ctx, hostname, ec} {
    if (ec)
        return;
    connector tcp;
    if (auto res = tcp.connect(addr); !res) {
        ec = res.error();
        return;
    }
    if (auto res = base::tls_connect(std::move(tcp)); !res)
        ec = res.error();
}

tls_connector::tls_connector(
    mbedtls_context& ctx, stream_socket&& sock, error_code& ec
) noexcept
    : base{ctx, string{}, ec} {
    if (!ec) {
        if (auto res = base::tls_connect(std::move(sock)); !res)
            ec = res.error();
    }
}

result<> tls_connector::connect(const sock_address& addr) noexcept {
    connector tcp;
    if (auto res = tcp.connect(addr); !res)
        return res;
    return base::tls_connect(std::move(tcp));
}

result<> tls_connector::connect(const sock_address& addr, microseconds timeout) noexcept {
    connector tcp;
    if (auto res = tcp.connect(addr, timeout); !res)
        return res;
    return base::tls_connect(std::move(tcp));
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
