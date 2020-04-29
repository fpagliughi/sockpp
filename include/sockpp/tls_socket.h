/**
 * @file tls_socket.h
 *
 * TLS (SSL) sockets.
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
// Copyright (c) 2014-2019 Frank Pagliughi
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

#ifndef __sockpp_tls_socket_h
#define __sockpp_tls_socket_h

#include "sockpp/stream_socket.h"
#include "sockpp/tls_context.h"
#include <memory>
#include <string>

namespace sockpp {

    /**
     * Abstract base class of TLS (SSL) sockets.
     * Instances are created by the \ref wrap_socket factory method of \ref tls_context.
     * First you create/open a regular \ref stream_socket, then immediately wrap it with TLS.
     */
    class tls_socket : public stream_socket
    {
        /** The base class   */
        using base = stream_socket;

    public:
        /**
         * Returns the verification status of the peer's certificate.
         * If the certificate is valid, returns zero.
         * Otherwise, returns a nonzero value whose interpretation depends on the actual
         * TLS library in use.
         * (For example, if using mbedTLS the return value is a bitwise combination of
         * \c MBEDTLS_X509_BADCERT_XXX and \c MBEDTLS_X509_BADCRL_XXX flags
         * defined in <mbedtls/x509.h>, or -1 if the TLS handshake failed earlier.)
         */
        virtual uint32_t peer_certificate_status() =0;

        /**
         * Returns an error message describing any problem with the peer's certificate.
         */
        virtual std::string peer_certificate_status_message() =0;

        /**
         * Returns the peer's X.509 certificate data, in binary DER format.
         */
        virtual std::string peer_certificate() =0;

        /**
         * Move assignment.
         * @param rhs The other socket to move into this one.
         * @return A reference to this object.
         */
        tls_socket& operator=(tls_socket&& rhs) {
            base::operator=(std::move(rhs));
            stream_ = std::move(rhs.stream_);
            return *this;
        }

        // I/O primitives must be reimplemented in subclasses:

        virtual ssize_t read(void *buf, size_t n) override = 0;
        virtual ioresult read_r(void *buf, size_t n) override = 0;
        virtual bool read_timeout(const std::chrono::microseconds& to) override = 0;
        virtual ssize_t write(const void *buf, size_t n) override = 0;
        virtual ioresult write_r(const void *buf, size_t n) override = 0;
        virtual bool write_timeout(const std::chrono::microseconds& to) override = 0;

        virtual ssize_t write(const std::vector<iovec> &ranges) override {
            return ranges.empty() ? 0 : write(ranges[0].iov_base, ranges[0].iov_len);
        }

        virtual bool set_non_blocking(bool on) override = 0;

        virtual bool close() override {
            bool ok = true;
            if (stream_) {
                ok = stream_->close();
                if (!ok && !last_error())
                    clear(stream_->last_error());
                stream_.reset();
            }
            release();
            return ok;
        }

        /**
         * The underlying socket stream that this socket wraps.
         * The TLS code reads and writes this stream.
         */
        stream_socket &stream() const {
            return *stream_;
        }

        virtual ~tls_socket() {
            close();
        }


    protected:
        static constexpr socket_t PLACEHOLDER_SOCKET = -999;

        tls_socket(std::unique_ptr<stream_socket> stream)
        :base(PLACEHOLDER_SOCKET)
        ,stream_(move(stream))
        { }

        /**
         * Creates a TLS socket by copying the socket handle from the
         * specified socket object and transfers ownership of the socket.
         */
        tls_socket(tls_socket&& sock) : tls_socket(std::move(sock.stream_)) {}

    private:
        std::unique_ptr<stream_socket> stream_;
    };

}

#endif
