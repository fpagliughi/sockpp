// mbedtls_context.cpp
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

#include "sockpp/tls/mbedtls/mbedtls_socket.h"

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

using namespace std;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

/////////////////////////////////////////////////////////////////////////////

namespace sockpp {

mbedtls_socket::mbedtls_socket(
    stream_socket&& sock, mbedtls_context& ctx, const string& hostname
)
    : base(std::move(sock)), ctx_(ctx) {
    mbedtls_ssl_init(&ssl_);
    if (ctx.status() != 0)
        // TODO: Is this the right error type?
        throw tls_error{ctx.status()};

    if (check_mbed_setup(mbedtls_ssl_setup(&ssl_, ctx_.ssl_config_.get())))
        return;
    if (!hostname.empty() &&
        check_mbed_setup(mbedtls_ssl_set_hostname(&ssl_, hostname.c_str())))
        return;

#if defined(_WIN32)
    // Winsock does not allow us to tell if a socket is nonblocking, so assume it isn't
    bool nonblocking = false;
#else
    int flags = fcntl(stream::handle(), F_GETFL, 0);
    bool nonblocking = (flags >= 0 && (flags & O_NONBLOCK) != 0);
#endif
    setup_bio(nonblocking);

    // Run the TLS handshake:
    open_ = true;
    int status;
    do {
        status = mbedtls_ssl_handshake(&ssl_);
    } while (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE ||
             status == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS);
    if (check_mbed_setup(status) != 0)
        return;

    uint32_t verify_flags = mbedtls_ssl_get_verify_result(&ssl_);
    if (verify_flags != 0 && verify_flags != uint32_t(-1) &&
        !(verify_flags & MBEDTLS_X509_BADCERT_SKIP_VERIFY)) {
        // char vrfy_buf[512];
        // mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", verify_flags);
        throw tls_error{MBEDTLS_ERR_X509_CERT_VERIFY_FAILED};
    }
}

mbedtls_socket::~mbedtls_socket() {
    close();
    mbedtls_ssl_free(&ssl_);
    // TODO: Shouldn't close() replace the file descriptor?
    reset();  // remove bogus file descriptor so base class won't call close() on it
}

void mbedtls_socket::setup_bio(bool nonblocking) {
    mbedtls_ssl_send_t* f_send = [](void* ctx, const uchar* buf, size_t n) {
        if (auto res = ((mbedtls_socket*)ctx)->bio_send(buf, n); res)
            return (int)res.value();
        else
            return (int)res.error().value();
    };

    mbedtls_ssl_recv_t* f_recv = nullptr;
    mbedtls_ssl_recv_timeout_t* f_recv_timeout = nullptr;
    // "The two most common use cases are:
    //	- non-blocking I/O, f_recv != nullptr, f_recv_timeout == nullptr
    //	- blocking I/O, f_recv == nullptr, f_recv_timeout != nullptr"
    if (nonblocking) {
        f_recv = [](void* ctx, uchar* buf, size_t n) {
            if (auto res = ((mbedtls_socket*)ctx)->bio_recv(buf, n); res)
                return (int)res.value();
            else
                return (int)res.error().value();
        };
    }
    else {
        f_recv_timeout = [](void* ctx, uchar* buf, size_t n, uint32_t timeout) {
            if (auto res = ((mbedtls_socket*)ctx)->bio_recv_timeout(buf, n, timeout); res)
                return (int)res.value();
            else
                return (int)res.error().value();
        };
    }
    mbedtls_ssl_set_bio(&ssl_, this, f_send, f_recv, f_recv_timeout);
}

result<> mbedtls_socket::close() {
    if (open_) {
        mbedtls_ssl_close_notify(&ssl_);
        open_ = false;
    }
    return base::close();
}

// -------- certificate / trust API

string mbedtls_socket::peer_certificate_status_message() {
    uint32_t verify_flags = mbedtls_ssl_get_verify_result(&ssl_);
    if (verify_flags == 0 || verify_flags == UINT32_MAX) {
        return string();
    }
    char message[512];
    mbedtls_x509_crt_verify_info(
        message, sizeof(message), "", verify_flags & ~MBEDTLS_X509_BADCERT_OTHER
    );
    size_t len = strlen(message);
    if (len > 0 && message[len] == '\0')
        --len;

    string result(message, len);
    if (verify_flags & MBEDTLS_X509_BADCERT_OTHER) {  // flag set by verify_callback()
        if (!result.empty()) {
            result = "\n" + result;
        }
        result = "The certificate does not match the known pinned certificate" + result;
    }
    return result;
}

string mbedtls_socket::peer_certificate() {
    auto cert = mbedtls_ssl_get_peer_cert(&ssl_);
    if (!cert) {
        // This should only happen in a failed handshake scenario, or if there
        // was no cert to begin with
        return ctx_.get_peer_certificate();
    }

    return string((const char*)cert->raw.p, cert->raw.len);
}

// -------- stream_socket I/O

result<size_t> mbedtls_socket::read(void* buf, size_t n) {
    auto ucbuf = reinterpret_cast<uchar*>(buf);
    return check_mbed_io<int, size_t>(mbedtls_ssl_read(&ssl_, ucbuf, n));
}

result<> mbedtls_socket::read_timeout(const microseconds& to) {
    auto res = stream::read_timeout(to);
    if (res)
        read_timeout_ = to;
    return res;
}

result<size_t> mbedtls_socket::write(const void* buf, size_t n) {
    if (n == 0)
        return 0;
    auto ucbuf = reinterpret_cast<const uchar*>(buf);
    return check_mbed_io<int, size_t>(mbedtls_ssl_write(&ssl_, ucbuf, n));
}

result<> mbedtls_socket::write_timeout(const microseconds& to) {
    return stream::write_timeout(to);
}

result<> mbedtls_socket::set_non_blocking(bool nonblocking) {
    auto res = stream::set_non_blocking(nonblocking);
    if (res)
        setup_bio(nonblocking);
    return res;
}

// -------- mbedTLS BIO callbacks

result<size_t> mbedtls_socket::bio_send(const void* buf, size_t n) {
    if (!open_)
        return make_tls_error_code(MBEDTLS_ERR_NET_CONN_RESET);
    return bio_return_value<size_t, false>(stream::write(buf, n));
}

result<size_t> mbedtls_socket::bio_recv(void* buf, size_t n) {
    if (!open_)
        return make_tls_error_code(MBEDTLS_ERR_NET_CONN_RESET);
    return bio_return_value<size_t, true>(stream::read(buf, n));
}

result<size_t> mbedtls_socket::bio_recv_timeout(void* buf, size_t n, uint32_t timeout) {
    if (!open_)
        return make_tls_error_code(MBEDTLS_ERR_NET_CONN_RESET);

    if (timeout > 0)
        stream::read_timeout(milliseconds(timeout));

    auto res = bio_recv(buf, n);

    if (timeout > 0)
        stream::read_timeout(read_timeout_);
    return res;
}

// -------- error handling

// Translates mbedTLS error code to POSIX (errno)
error_code mbedtls_socket::translate_mbed_err(int mbedErr) {
    switch (mbedErr) {
        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        case MBEDTLS_ERR_NET_CONN_RESET:
            return std::make_error_code(std::errc::connection_reset);
        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            return std::make_error_code(std::errc::operation_would_block);
        case MBEDTLS_ERR_NET_RECV_FAILED:
        case MBEDTLS_ERR_NET_SEND_FAILED:
            return std::make_error_code(std::errc::io_error);
        default:
            // TODO: _Should_ this be negated?
            return make_tls_error_code(-mbedErr);
    }
}

// Handles an mbedTLS error return value during setup, closing me on error
result<int> mbedtls_socket::check_mbed_setup(int ret) {
    result<int> res;

    if (ret != 0) {
        error_code ec = translate_mbed_err(ret);
        //        if (ret == MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE)
        //            // TODO: What is ssl_...?
        //            err = mbedtls_context::FATAL_ERROR_ALERT_BASE;  //- ssl_.in_msg[1];

        // TODO: Is this right?
        reset();  // marks me as closed/invalid
        res = result<int>{ec};

        // Signal we're done by shutting down the socket's write stream. That lets the
        // client finish sending any data and receive our error alert. Wait until the
        // client closes, by reading data until we get 0 bytes, then finally close.
        stream::shutdown(SHUT_WR);
        stream::read_timeout(2000ms);
        char buf[512];
        while (true) {
            if (auto res = stream::read(buf, sizeof(buf)); !res || res.value() == 0)
                break;
        }
        stream::close();
        open_ = false;
    }
    else
        res = ret;
    return res;
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
