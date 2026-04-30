/**
 * @file tcp_acceptor.h
 *
 * Classes for TCP server acceptor sockets (IPv4 and IPv6).
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date December 2014
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2026 Frank Pagliughi All rights reserved.
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

#ifndef __sockpp_tcp_acceptor_h
#define __sockpp_tcp_acceptor_h

#include "sockpp/acceptor.h"
#include "sockpp/tcp_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * TCP v4 server acceptor socket.
 * Objects of this class bind and listen on TCP v4 ports for incoming
 * connections. Normally, a server thread creates one of these and blocks
 * on the call to accept incoming connections. The call to accept creates
 * and returns a @c tcp_socket which can then be used for the actual
 * communications.
 */
using tcp_acceptor = acceptor_tmpl<tcp_socket>;

/** Alias for a TCP v4 server acceptor socket. */
using tcp4_acceptor = tcp_acceptor;

/**
 * TCP v6 server acceptor socket.
 * Objects of this class bind and listen on TCP v6 ports for incoming
 * connections. Normally, a server thread creates one of these and blocks
 * on the call to accept incoming connections. The call to accept creates
 * and returns a @c tcp6_socket which can then be used for the actual
 * communications.
 */
using tcp6_acceptor = acceptor_tmpl<tcp6_socket>;

/////////////////////////////////////////////////////////////////////////////
};  // namespace sockpp

#endif  // __sockpp_tcp_acceptor_h
