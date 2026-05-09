// mbedtls_acceptor.cpp
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

#include "sockpp/tls/mbedtls_acceptor.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

tls_acceptor::tls_acceptor(mbedtls_context& ctx, const sock_address& addr, int backlog)
    : ctx_{&ctx} {
    if (auto res = base::open(addr, backlog); !res)
        throw std::system_error{res.error()};
}

tls_acceptor::tls_acceptor(
    mbedtls_context& ctx, const sock_address& addr, int backlog, error_code& ec
) noexcept
    : ctx_{&ctx} {
    if (auto res = base::open(addr, backlog); !res)
        ec = res.error();
}

result<tls_socket> tls_acceptor::wrap(result<stream_socket> raw) {
    if (!raw)
        return raw.error();
    auto tls_res = ctx_->wrap_socket(raw.release());
    if (!tls_res)
        return tls_res.error();
    return tls_socket{std::move(*tls_res.release())};
}

result<tls_socket> tls_acceptor::accept(sock_address* peer_addr) {
    return wrap(base::accept(peer_addr));
}

result<tls_socket> tls_acceptor::accept(microseconds timeout, sock_address* peer_addr) {
    return wrap(base::accept(timeout, peer_addr));
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
