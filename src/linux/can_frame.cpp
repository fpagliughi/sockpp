// can_frame.cpp
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

#include "sockpp/can_frame.h"

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

can_frame::can_frame(canid_t canID, const void* data, size_t n) : base{} {
    this->can_id = canID;
    if (data && n != 0) {
        n = std::min(n, size_t(CAN_MAX_DLEN));
        this->can_dlc = n;
        std::memcpy(&this->data, data, n);
    }
}

can_frame::can_frame(const canfd_frame& fdframe) {
    if (fdframe.len > CAN_MAX_DLEN)
        throw system_error{make_error_code(errc::invalid_argument)};
    std::memcpy(frame_ptr(), fdframe.frame_ptr(), SZ);
}

can_frame::can_frame(const canfd_frame& fdframe, error_code& ec) {
    if (fdframe.len > CAN_MAX_DLEN)
        ec = make_error_code(errc::invalid_argument);
    ec = error_code{};
    std::memcpy(frame_ptr(), fdframe.frame_ptr(), SZ);
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
