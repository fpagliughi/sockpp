// can_socket.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2021 Frank Pagliughi
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

#include "sockpp/can_socket.h"
#include "sockpp/socket.h"
#include <sys/ioctl.h>
#include <linux/sockios.h>

using namespace std;
using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

can_socket::can_socket(const can_address& addr)
{
	socket_t h = create_handle(SOCK_RAW, CAN_RAW);

	if (check_socket_bool(h)) {
		reset(h);
		bind(addr);
	}
}

system_clock::time_point can_socket::last_frame_time()
{
	timeval tv {};

	// TODO: Handle error
	::ioctl(handle(), SIOCGSTAMP, &tv);
	return to_timepoint(tv);
}

double can_socket::last_frame_timestamp()
{
	timeval tv {};

	// TODO: Handle error
	::ioctl(handle(), SIOCGSTAMP, &tv);
	return double(tv.tv_sec) + 1.0e-6 * tv.tv_usec;
}


// --------------------------------------------------------------------------

ssize_t can_socket::recv_from(can_frame *frame, int flags, can_address* srcAddr /*=nullptr*/)
{
    sockaddr* p = srcAddr ? srcAddr->sockaddr_ptr() : nullptr;
    socklen_t len = srcAddr ? srcAddr->size() : 0;

    // Only receive the exact size of the can_frame object
    ssize_t ret = ::recvfrom(handle(), frame, sizeof(*frame), flags, p, &len);

    if (ret == -1) {
        throw std::system_error(errno, std::system_category(), "recvfrom failed");
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}
