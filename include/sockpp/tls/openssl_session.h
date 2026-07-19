/**
 * @file tls/openssl_session.h
 *
 * TLS session token for OpenSSL — used for session resumption.
 *
 * @author Frank Pagliughi
 * @date July 2026
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

#ifndef __sockpp_tls_openssl_session_h
#define __sockpp_tls_openssl_session_h

#include <openssl/ssl.h>

#include <memory>

#include "sockpp/result.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * An opaque TLS session token obtained from a completed handshake.
 *
 * Pass a @c tls_session to @c tls_connector::set_session() before
 * @c connect() to attempt session resumption.  Whether the server
 * honours the offer is at its discretion; call
 * @c tls_socket::session_reused() after the handshake to confirm.
 *
 * Tokens can be serialised to bytes and restored with @c from_bytes(),
 * allowing them to be stored in an external cache or persisted across
 * process restarts.
 */
class tls_session
{
    /** The underlying OpenSSL session struct. */
    std::shared_ptr<SSL_SESSION> sess_;

    /** Private constructor; only socket/connector create tokens. */
    explicit tls_session(SSL_SESSION* s) : sess_{s, SSL_SESSION_free} {}

    friend class tls_socket;
    friend class tls_connector;

public:
    /**
     * Creates an empty (invalid) session token.
     */
    tls_session() = default;

    /**
     * Returns true if this token holds a valid session.
     */
    bool is_valid() const { return sess_ != nullptr; }

    /**
     * Serialises the session to a portable byte blob.
     *
     * The returned bytes can be stored externally and reloaded with
     * @c from_bytes().  Returns an empty binary if the session is not
     * valid.
     *
     * @return DER-encoded session bytes, or empty on failure.
     */
    binary to_bytes() const;

    /**
     * Deserialises a session from bytes previously produced by @c to_bytes().
     *
     * @param data The serialised session bytes.
     * @return A valid @c tls_session on success, or an error code on failure.
     */
    static result<tls_session> from_bytes(const binary& data);
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_openssl_session_h
