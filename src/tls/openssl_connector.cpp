// openssl_connector.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023-2025 Frank Pagliughi All rights reserved.
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

#include "sockpp/tls/openssl_connector.h"

#include "sockpp/tls/openssl_context.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_connector::tls_connector(const tls_context& ctx, const sock_address& addr)
    : base{ctx, connector{addr}} {
    if (auto res = tls_connect(); !res)
        throw tls_error{res.error()};
}

tls_connector::tls_connector(
    const tls_context& ctx, const sock_address& addr, string& hostname
)
    : base{ctx, connector{addr}} {
    if (auto res = set_host_name(hostname); !res)
        throw tls_error{res.error()};

    if (auto res = tls_connect(); !res)
        throw tls_error{res.error()};
}

tls_connector::tls_connector(
    const tls_context& ctx, const sock_address& addr, string& hostname, error_code& ec
) noexcept
    : base{ctx, connector{addr}, ec} {
    if (!ec) {
        if (auto res = set_host_name(hostname); !res)
            ec = res.error();
    }
    if (!ec) {
        if (auto res = tls_connect(); !res)
            ec = res.error();
    }
}

tls_connector& tls_connector::operator=(tls_connector&& rhs) {
    if (&rhs != this) {
        base::operator=(std::move(rhs));
    }
    return *this;
}

result<> tls_connector::connect(const sock_address& addr) noexcept {
    connector conn;
    if (auto res = conn.connect(addr); !res)
        return res;
    return tls_connect(std::move(conn));
}

result<> tls_connector::connect(const sock_address& addr, microseconds timeout) {
    connector conn;
    if (auto res = conn.connect(addr, timeout); !res)
        return res;
    return tls_connect(std::move(conn));
}

result<> tls_connector::tls_connect(stream_socket&& sock) noexcept {
    if (auto res = attach(std::move(sock)); !res)
        return res;
    return tls_check_res_none(::SSL_connect(ssl()));
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
