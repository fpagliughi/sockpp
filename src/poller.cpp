// poller.cpp
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

#include "sockpp/poller.h"

#include <algorithm>
#include <climits>

#if !defined(_WIN32)
    #include <sys/poll.h>
#endif

using namespace std::chrono;

namespace sockpp {

// --------------------------------------------------------------------------

void poller::add(socket& sock, short events /*=POLL_READ*/) {
    socks_.push_back(&sock);
    pfds_.push_back(pollfd{sock.handle(), events, 0});
}

// --------------------------------------------------------------------------

void poller::remove(const socket& sock) {
    auto it = std::find(socks_.begin(), socks_.end(), &sock);
    if (it == socks_.end())
        return;

    auto idx = std::distance(socks_.begin(), it);
    socks_.erase(it);
    pfds_.erase(pfds_.begin() + idx);
}

// --------------------------------------------------------------------------

result<vector<poller::event>> poller::wait(milliseconds timeout /*=milliseconds{-1}*/) {
    if (pfds_.empty())
        return vector<event>{};

    int ms = (timeout.count() < 0) ? -1 : int(std::clamp(timeout.count(),
        decltype(timeout.count()){0},
        decltype(timeout.count()){INT_MAX}));

#if defined(_WIN32)
    int n = ::WSAPoll(pfds_.data(), ULONG(pfds_.size()), ms);
#else
    int n = ::poll(pfds_.data(), nfds_t(pfds_.size()), ms);
#endif

    if (n < 0)
        return result<vector<event>>::from_last_error();

    vector<event> evts;
    evts.reserve(size_t(n));

    for (size_t i = 0; i < pfds_.size(); ++i) {
        if (pfds_[i].revents != 0)
            evts.push_back(event{socks_[i], pfds_[i].revents});
    }

    return evts;
}

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}  // namespace sockpp
