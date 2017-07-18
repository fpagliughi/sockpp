/**
 * @file stream_connector.h
 *
 * Class for creating client-side TCP connections
 *
 * @author	Frank Pagliughi
 * @author	SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	December 2014
 */

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

#ifndef __sockpp_stream_connector_h
#define __sockpp_stream_connector_h

#include "sockpp/stream_socket.h"
#include "sockpp/sock_address.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to create a client stream connection.
 */
class stream_connector : public stream_socket
{
	// Non-copyable
	stream_connector(const stream_connector&) =delete;
	stream_connector& operator=(const stream_connector&) =delete;

public:
	/**
	 * Creates an unconnected connector.
	 */
	stream_connector() {}
	/**
	 * Creates the connector and attempts to connect to the specified
	 * address.
	 * @param addr The remote server address. 
	 * @param len The length of the address structure. 
	 */
	stream_connector(const sockaddr* addr, socklen_t len);
	/**
	 * Creates the connector and attempts to connect to the specified
	 * address.
	 * @param addr The remote server address. 
	 */
	stream_connector(const sock_address& a) : stream_connector(a.addr, a.len) {}
	/**
	 * Determines if the socket connected to a remote host.
	 * Note that this is not a reliable determination if the socket is
	 * currently connected, but rather that an initial connection was
	 * established.
	 * @return @em true If the socket connected to a remote host,
	 *  	   @em false if not.
	 */
	bool is_connected() const { return is_open(); }
	/**
	 * Attempts to connects to the specified server.
	 * If the socket is currently connected, this will close the current
	 * connection and open the new one.
	 * @param addr The remote server address. 
	 * @param len The length of the address structure in bytes 
	 * @return @em true on success, @em false on error
	 */
	bool connect(const sockaddr* addr, socklen_t len);
	/**
	 * Attempts to connects to the specified server.
	 * If the socket is currently connected, this will close the current
	 * connection and open the new one.
	 * @param addr The remote server address.
	 * @return @em true on success, @em false on error
	 */
	bool connect(const sock_address& addr) {
		return connect(addr.addr, addr.len);
	}
	#if 0
	/**
	 * Attempts to connects to the specified server.
	 * If the socket is currently connected, this will close the current
	 * connection and open the new one.
	 * @param addr The remote server address.
	 * @return @em true on success, @em false on error
	 */
	template <typename ADDR>
	bool connect(const ADDR& addr) {
		return connect(addr.sockaddr_ptr(), addr.size());
	}
	#endif
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
};

#endif		// __sockpp_stream_connector_h
