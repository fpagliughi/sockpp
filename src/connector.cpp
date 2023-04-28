// connector.cpp
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

#include "sockpp/connector.h"
#include <cerrno>
#if !defined(_WIN32)
	#include <sys/poll.h>
	#if defined(__APPLE__)
		#include <net/if.h>
	#endif
#endif

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

bool connector::recreate(const sock_address& addr)
{
	sa_family_t domain = addr.family();
	socket_t h = create_handle(domain);

	if (!check_socket_bool(h))
		return false;

	// This will close the old connection, if any.
	reset(h);
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool connector::connect(const sock_address& addr)
{
	if (!recreate(addr))
		return false;

	if (!check_ret_bool(::connect(handle(), addr.sockaddr_ptr(), addr.size())))
		return close_on_err();

	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool connector::connect(const sock_address& addr, microseconds timeout)
{
	if (timeout.count() <= 0)
		return connect(addr);

	if (!recreate(addr))
		return false;

	bool non_blocking =
		#if defined(_WIN32)
			false;
		#else
			is_non_blocking();
		#endif

	if (!non_blocking)
		set_non_blocking(true);

	if (!check_ret_bool(::connect(handle(), addr.sockaddr_ptr(), addr.size()))) {
		auto err = last_error();
		if (err == errc::operation_in_progress || err == errc::operation_would_block) {
			// TODO: Windows has a WSAPoll() function we can use.
			#if defined(_WIN32)
    			// Non-blocking connect -- call `select` to wait until the timeout:
    			// Note:  Windows returns errors in exceptset so check it too, the
    			// logic afterwords doesn't change
    			fd_set readset;
    			FD_ZERO(&readset);
    			FD_SET(handle(), &readset);
    			fd_set writeset = readset;
    			fd_set exceptset = readset;
    			timeval tv = to_timeval(timeout);
    			int n = check_ret(::select(int(handle())+1, &readset, &writeset, &exceptset, &tv));
			#else
				pollfd fds = { handle(), POLLIN|POLLOUT, 0 };
				int ms = int(duration_cast<milliseconds>(timeout).count());
				int n = check_ret(::poll(&fds, 1, ms));
			#endif

			if (n > 0) {
				// Got a socket event, but it might be an error, so check:
				int err;
				if (get_option(SOL_SOCKET, SO_ERROR, &err))
					clear(err);
			}
			else if (n == 0) {
				clear(ETIMEDOUT);
			}
		}

		if (last_error()) {
			close();
			return false;
		}
	}

	// Restore blocking mode for socket, if needed.
	if (!non_blocking)
		set_non_blocking(false);

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

