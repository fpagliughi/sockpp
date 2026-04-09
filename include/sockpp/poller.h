/**
 * @file poller.h
 *
 * Class for polling on a collection of sockets.
 *
 * @author  Frank Pagliughi
 * @author  SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date    April 2026
 */

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

#ifndef __sockpp_poller_h
#define __sockpp_poller_h

#include "sockpp/platform.h"
#include "sockpp/result.h"
#include "sockpp/socket.h"
#include "sockpp/types.h"

#if !defined(_WIN32)
    #include <poll.h>
#endif

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Polls a collection of sockets for I/O readiness.
 *
 * Sockets are registered with an event mask (POLLIN, POLLOUT, etc.) and
 * the poller keeps a non-owning pointer to each one alongside the
 * corresponding pollfd entry used by the OS call. The sockets must remain
 * alive for as long as they are registered.
 *
 * On POSIX systems the underlying mechanism is poll(2); on Windows it is
 * WSAPoll().
 */
class poller
{
    /** Non-owning pointers to registered sockets, parallel to pfds_. */
    vector<socket*> socks_;
    /** pollfd array passed directly to the OS poll call. */
    vector<pollfd> pfds_;

public:
    /**
     * Event flag indicating the socket is ready to read.
     * Maps to POLLIN on all platforms.
     */
    static constexpr short POLL_IN = POLLIN;
    /**
     * Event flag indicating the socket is ready to write.
     * Maps to POLLOUT on all platforms.
     */
    static constexpr short POLL_OUT = POLLOUT;
    /**
     * Event flag indicating both read and write readiness.
     */
    static constexpr short POLL_INOUT = POLLIN | POLLOUT;

    /**
     * A single poll result, pairing a socket pointer with the events
     * that were observed on it.
     */
    struct event
    {
        /** Non-owning pointer to the socket that has an event. */
        socket* sock;
        /** The events observed (POLLIN, POLLOUT, POLLERR, etc.). */
        short events;
    };

    /**
     * Creates an empty poller.
     */
    poller() = default;

    /**
     * Creates a poller pre-sized for an expected number of sockets.
     * @param n The expected number of sockets to be registered.
     */
    explicit poller(size_t n) {
        socks_.reserve(n);
        pfds_.reserve(n);
    }

    /**
     * Registers a socket with the poller.
     *
     * The socket is not owned by the poller and must remain alive until
     * it is removed or the poller is destroyed.
     *
     * @param sock The socket to watch.
     * @param events The event mask to monitor (e.g. POLL_IN, POLL_OUT).
     */
    void add(socket& sock, short events = POLL_IN);

    /**
     * Removes a previously registered socket.
     * Does nothing if the socket is not registered.
     * @param sock The socket to remove.
     */
    void remove(const socket& sock);

    /**
     * Returns the number of sockets currently registered.
     */
    size_t size() const { return socks_.size(); }

    /**
     * Returns true if no sockets are registered.
     */
    bool empty() const { return socks_.empty(); }

    /**
     * Waits for I/O events on the registered sockets.
     *
     * @param timeout How long to wait. A negative duration waits forever;
     *                zero returns immediately (non-blocking check).
     * @return A vector of events for each socket that became ready, or an
     *         error code on failure.
     */
    result<vector<event>> wait(milliseconds timeout = milliseconds{-1});
    /**
     * Waits for I/O events on the registered sockets.
     *
     * This overload accepts any chrono duration and converts it to
     * milliseconds before calling the OS poll function.
     *
     * @param timeout How long to wait. A negative duration waits forever;
     *                zero returns immediately (non-blocking check).
     * @return A vector of events for each socket that became ready, or an
     *         error code on failure.
     */
    template <class Rep, class Period>
    result<vector<event>> wait(const duration<Rep, Period>& timeout) {
        return wait(std::chrono::duration_cast<milliseconds>(timeout));
    }
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}  // namespace sockpp

#endif  // __sockpp_poller_h
