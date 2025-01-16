/**
 * @file tls/context.h
 *
 * Master include for sockpp `tls_context` objects.
 *
 * @date August 2019
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2024 Frank Pagliughi
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

#ifndef __sockpp_tls_context_h
#define __sockpp_tls_context_h

#include "sockpp/version.h"

#if defined(SOCKPP_OPENSSL)
    #include "sockpp/tls/openssl_context.h"
#elif defined(SOCKPP_MBEDTLS)
    #include "sockpp/tls/mbedtls_context.h"
#else
    #error "No TLS library chosen for sockpp"
#endif

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to build a TLS context.
 */
class tls_context_builder
{
    /** The context gets built in-line */
    tls_context ctx_;
    /** The error code of the first setting that fails */
    error_code ec_;

    // Non-copyable
    tls_context_builder(const tls_context_builder&) = delete;
    tls_context_builder& operator=(const tls_context_builder&) = delete;

public:
    /** This class */
    using self = tls_context_builder;

    explicit tls_context_builder(tls_context::role_t role = tls_context::role_t::CLIENT)
        : ctx_{role} {}
    /**
     * Creates a builder for a client context.
     * @return A builder for a client context.
     */
    static tls_context_builder client() {
        return tls_context_builder{tls_context::role_t::CLIENT};
    }
    /**
     * Creates a builder for a server context.
     * @return A builder for a server context.
     */
    static tls_context_builder server() {
        return tls_context_builder{tls_context::role_t::SERVER};
    }
    /**
     * Move assignment.
     * @param rhs The other TLS context builder to move into this one.
     * @return A reference to this object.
     */
    tls_context_builder& operator=(tls_context_builder&& rhs) {
        if (&rhs != this)
            std::swap(ctx_, rhs.ctx_);
        return *this;
    }
    /**
     * Gets the error code for the first setting that failed.
     * @return The error code for the first setting that failed.
     */
    error_code error() const { return ec_; }
    /**
     * Specify that application should use the default locations of the CA
     * certificates as the trust store.
     */
    auto default_trust_locations() -> self& {
        if (!ec_) {
            if (auto res = ctx_.set_default_trust_locations(); !res)
                ec_ = res.error();
        }
        return *this;
    }
    /**
     * Sets a file of CA certificates as the trust store. The file should be
     * in PEM format.
     * @param caFile The path to a file of CA certificates to be used as the
     *               trust store.
     */
    auto trust_file(const string& caFile) -> self& {
        if (!ec_) {
            if (auto res = ctx_.set_trust_file(caFile); !res)
                ec_ = res.error();
        }
        return *this;
    }
    /**
     * Sets a directory containing CA certificate files as the trust store.
     * @param caPath The  directory containing CA certificate files as the
     *               trust store.
     */
    auto trust_path(const string& caPath) -> self& {
        if (!ec_) {
            if (auto res = ctx_.set_trust_path(caPath); !res)
                ec_ = res.error();
        }
        return *this;
    }
    /**
     * Sets the verification mode for connections.
     * @param mode The verification mode to use for connections.
     */
    auto verify(tls_context::verify_t mode) -> self& {
        if (!ec_)
            ctx_.set_verify(mode);
        return *this;
    }
    /**
     * The connections should not verify the peer certificates.
     */
    auto verify_none() -> self& {
        if (!ec_)
            ctx_.set_verify(tls_context::verify_t::NONE);
        return *this;
    }
    /**
     * The connections should verify the peer certificates.
     */
    auto verify_peer() -> self& {
        if (!ec_)
            ctx_.set_verify(tls_context::verify_t::PEER);
        return *this;
    }
    /**
     * Load the certificate chain from a file.
     * @param certFile The certificate chain file.
     * @return Error code on failure.
     */
    auto cert_file(const string& certFile) -> self& {
        if (!ec_) {
            if (auto res = ctx_.set_cert_file(certFile); !res)
                ec_ = res.error();
        }
        return *this;
    }
    /**
     * Set the private key from a file.
     * @param keyFile The private key file.
     * @return Error code on failure.
     */
    auto key_file(const string& keyFile) -> self& {
        if (!ec_) {
            if (auto res = ctx_.set_key_file(keyFile); !res)
                ec_ = res.error();
        }
        return *this;
    }
    /**
     * Get the context that has been built.
     * This can only be called once for the given configuration. After it is
     * called, the builder should not be used again.
     * @return The context that has been built.
     */
    tls_context finalize() { return std::move(ctx_); }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tls_context_h
