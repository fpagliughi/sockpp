/**
 * @file mbedtls_socket.h
 *
 * TLS (SSL) socket implementation using mbedTLS.
 *
 * @author Jens Alfke
 * @author Couchbase, Inc.
 * @author www.couchbase.com
 *
 * @date August 2019
 */

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

#ifndef __sockpp_mbedtls_socket_h
#define __sockpp_mbedtls_socket_h

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

#include <cassert>
#include <chrono>
#include <mutex>

#include "sockpp/connector.h"
#include "sockpp/tls/mbedtls/mbedtls_context.h"
#include "sockpp/tls/mbedtls/mbedtls_error.h"

#ifdef __APPLE__
    #include <TargetConditionals.h>
    #include <fcntl.h>
    #ifdef TARGET_OS_OSX
        // For macOS read_system_root_certs():
        #include <Security/Security.h>
    #endif
#elif !defined(_WIN32)
    // For Unix read_system_root_certs():
    #include <dirent.h>
    #include <fcntl.h>
    #include <fnmatch.h>
    #include <sys/stat.h>

    #include <fstream>
    #include <iostream>
    #include <sstream>
#else
    #include <wincrypt.h>

    #include <sstream>

    #pragma comment(lib, "crypt32.lib")
    #pragma comment(lib, "cryptui.lib")
#endif

namespace sockpp {

/** MbedTLS uses unsigned char buffers for read/write */
using uchar = unsigned char;

/////////////////////////////////////////////////////////////////////////////

/** Concrete implementation of tls_socket using mbedTLS. */
class mbedtls_socket : public tls_socket
{
    /** Abstract base class */
    using base = tls_socket;
    /** Concrete base class above base */
    using stream = stream_socket;

    mbedtls_context& ctx_;
    mbedtls_ssl_context ssl_;
    std::chrono::microseconds read_timeout_{0L};
    bool open_ = false;

    // -------- error handling

    // Translates MbedTLS error code to POSIX (errno), if it's a common error,
    // otherwise returns the MbedTLS-specific error
    error_code translate_mbed_err(int mbedErr);

    // Handles an mbedTLS error return value during setup, closing me on error
    result<int> check_mbed_setup(int ret);

    // Handles an mbedTLS read/write return value, storing any error in last_error
    template <typename T, typename Tout = T>
    inline result<Tout> check_mbed_io(T res) {
        if (res < 0)
            return translate_mbed_err(int(res));
        return result<Tout>{Tout(res)};
    }

    // Translates result to an mbedTLS error code to return from a BIO function.
    template <typename T, bool Reading, typename Tout = T>
    result<Tout> bio_return_value(result<T> res) {
        if (res)
            return res.value();

        auto err = res.error();

        if (err == std::errc::broken_pipe || err == std::errc::connection_reset)
            return make_tls_error_code(MBEDTLS_ERR_NET_CONN_RESET);

        if (err == std::errc::interrupted || err == std::errc::operation_would_block ||
            err == std::errc::resource_unavailable_try_again)
            return Reading ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_SSL_WANT_WRITE;

        return Reading ? MBEDTLS_ERR_NET_RECV_FAILED : MBEDTLS_ERR_NET_SEND_FAILED;
    }

public:
    mbedtls_socket(stream_socket&& sock, mbedtls_context& ctx, const string& hostname);
    ~mbedtls_socket();

    void setup_bio(bool nonblocking);

    result<> close() override;

    // -------- certificate / trust API

    uint32_t peer_certificate_status() override {
        return mbedtls_ssl_get_verify_result(&ssl_);
    }

    string peer_certificate_status_message() override;
    string peer_certificate() override;

    // -------- stream_socket I/O

    result<size_t> read(void* buf, size_t n) override;
    result<> read_timeout(const microseconds& to) override;

    result<size_t> write(const void* buf, size_t n) override;
    result<> write_timeout(const microseconds& to) override;

    result<> set_non_blocking(bool nonblocking) override;

    // -------- mbedTLS BIO callbacks

    result<size_t> bio_send(const void* buf, size_t n);
    result<size_t> bio_recv(void* buf, size_t n);
    result<size_t> bio_recv_timeout(void* buf, size_t n, uint32_t timeout);
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_mbedtls_socket_h
