// mbedtls_context.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
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

#include "sockpp/mbedtls_context.h"
#include "sockpp/connector.h"
#include "sockpp/exception.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mutex>
#include <chrono>
#include <cassert>

#ifdef __APPLE__
    #include <fcntl.h>
    #include <TargetConditionals.h>
    #ifdef TARGET_OS_OSX
        // For macOS read_system_root_certs():
        #include <Security/Security.h>
    #endif
#elif !defined(_WIN32)
    // For Unix read_system_root_certs():
    #include <dirent.h>
    #include <fcntl.h>
    #include <fnmatch.h>
    #include <fstream>
    #include <iostream>
    #include <sstream>
    #include <sys/stat.h>
#else
	#include <wincrypt.h>
    #include <sstream>

    #pragma comment (lib, "crypt32.lib")
	#pragma comment (lib, "cryptui.lib")
#endif


// TODO: Better logging(?)
#define log(FMT,...) fprintf(stderr, "TLS: " FMT "\n", ## __VA_ARGS__)


namespace sockpp {
    using namespace std;


    static std::string read_system_root_certs();


    static int log_mbed_ret(int ret, const char *fn) {
        if (ret != 0) {
            char msg[100];
            mbedtls_strerror(ret, msg, sizeof(msg));
            log("mbedtls error -0x%04X from %s: %s", -ret, fn, msg);
        }
        return ret;
    }


    // Simple RAII helper for mbedTLS cert struct
    struct mbedtls_context::cert : public mbedtls_x509_crt
    {
        cert()  {mbedtls_x509_crt_init(this);}
        ~cert() {mbedtls_x509_crt_free(this);}
    };


    // Simple RAII helper for mbedTLS cert struct
    struct mbedtls_context::key : public mbedtls_pk_context
    {
        key()  {mbedtls_pk_init(this);}
        ~key() {mbedtls_pk_free(this);}
    };


#pragma mark - SOCKET:


    /** Concrete implementation of tls_socket using mbedTLS. */
    class mbedtls_socket : public tls_socket {
    private:
        mbedtls_context& context_;
        mbedtls_ssl_context ssl_;
        chrono::microseconds read_timeout_ {0L};
        bool open_ = false;

    public:

        mbedtls_socket(unique_ptr<stream_socket> base,
                       mbedtls_context &context,
                       const string &hostname)
        :tls_socket(move(base))
        ,context_(context)
        {
            mbedtls_ssl_init(&ssl_);
            if (context.status() != 0) {
                clear(context.status());
                return;
            }

            if (check_mbed_setup(mbedtls_ssl_setup(&ssl_, context_.ssl_config_.get()),
                               "mbedtls_ssl_setup"))
                return;
            if (!hostname.empty() && check_mbed_setup(mbedtls_ssl_set_hostname(&ssl_, hostname.c_str()),
                                                    "mbedtls_ssl_set_hostname"))
                return;

#if defined(_WIN32)
            // Winsock does not allow us to tell if a socket is nonblocking, so assume it isn't
            bool blocking = true;
#else
            int flags = fcntl(stream().handle(), F_GETFL, 0);
            bool blocking = (flags < 0 || (flags & O_NONBLOCK) == 0);
#endif
            setup_bio(blocking);

            // Run the TLS handshake:
            int status;
            do {
                open_ = true;  // temporarily, so BIO methods won't fail
                status = mbedtls_ssl_handshake(&ssl_);
                open_ = false;
            } while (status == MBEDTLS_ERR_SSL_WANT_READ || status == MBEDTLS_ERR_SSL_WANT_WRITE
                            || status == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS);
            if (check_mbed_setup(status, "mbedtls_ssl_handshake") != 0)
                return;

            uint32_t verify_flags = mbedtls_ssl_get_verify_result(&ssl_);
            if (verify_flags != 0 && verify_flags != uint32_t(-1)
                                  && !(verify_flags & MBEDTLS_X509_BADCERT_SKIP_VERIFY)) {
                char vrfy_buf[512];
                mbedtls_x509_crt_verify_info(vrfy_buf, sizeof( vrfy_buf ), "", verify_flags);
                log("Cert verify failed: %s", vrfy_buf );
                reset();
                clear(MBEDTLS_ERR_X509_CERT_VERIFY_FAILED);
                return;
            }
            open_ = true;
        }


        void setup_bio(bool nonblocking) {
            mbedtls_ssl_send_t *f_send = [](void *ctx, const uint8_t *buf, size_t len) {
                return ((mbedtls_socket*)ctx)->bio_send(buf, len); };
            mbedtls_ssl_recv_t *f_recv = nullptr;
            mbedtls_ssl_recv_timeout_t *f_recv_timeout = nullptr;
            if (nonblocking)
                f_recv = [](void *ctx, uint8_t *buf, size_t len) {
                    return ((mbedtls_socket*)ctx)->bio_recv(buf, len); };
            else
                f_recv_timeout = [](void *ctx, uint8_t *buf, size_t len, uint32_t timeout) {
                    return ((mbedtls_socket*)ctx)->bio_recv_timeout(buf, len, timeout); };
            mbedtls_ssl_set_bio(&ssl_, this, f_send, f_recv, f_recv_timeout);
        }


        ~mbedtls_socket() {
            close();
            mbedtls_ssl_free(&ssl_);
            reset(); // remove bogus file descriptor so base class won't call close() on it
        }


        virtual bool close() override {
            if (open_) {
                mbedtls_ssl_close_notify(&ssl_);
                open_ = false;
            }
            return tls_socket::close();
        }


        // -------- certificate / trust API


        uint32_t peer_certificate_status() override {
            return mbedtls_ssl_get_verify_result(&ssl_);
        }


        string peer_certificate_status_message() override {
            uint32_t verify_flags = mbedtls_ssl_get_verify_result(&ssl_);
            if (verify_flags == 0 || verify_flags == UINT32_MAX)
                return "";
            char message[512];
            mbedtls_x509_crt_verify_info(message, sizeof( message ), "",
                                         verify_flags & ~MBEDTLS_X509_BADCERT_OTHER);
            size_t len = strlen(message);
            if (len > 0 && message[len] == '\0')
                --len;

            string result(message, len);
            if (verify_flags & MBEDTLS_X509_BADCERT_OTHER) {    // flag set by verify_callback()
                if (!result.empty())
                    result = "\n" + result;
                result = "The certificate does not match the known pinned certificate" + result;
            }
            return result;
        }


        string peer_certificate() override {
            auto cert = mbedtls_ssl_get_peer_cert(&ssl_);
            if (!cert)
                return "";
            return string((const char*)cert->raw.p, cert->raw.len);
        }


        // -------- stream_socket I/O


        ssize_t read(void *buf, size_t length) override {
            return check_mbed_io( mbedtls_ssl_read(&ssl_, (uint8_t*)buf, length) );
        }


        ioresult read_r(void *buf, size_t length) override {
            return ioresult_from_mbed( mbedtls_ssl_read(&ssl_, (uint8_t*)buf, length) );
        }


        bool read_timeout(const chrono::microseconds& to) override {
            bool ok = stream().read_timeout(to);
            if (ok)
                read_timeout_ = to;
            return ok;
        }


        ssize_t write(const void *buf, size_t length) override {
            if (length == 0)
                return 0;
            return check_mbed_io( mbedtls_ssl_write(&ssl_, (const uint8_t*)buf, length) );
        }


        ioresult write_r(const void *buf, size_t length) override {
            if (length == 0)
                return {};
            return ioresult_from_mbed( mbedtls_ssl_write(&ssl_, (const uint8_t*)buf, length) );
        }


        bool write_timeout(const chrono::microseconds& to) override {
            return stream().write_timeout(to);
        }


        bool set_non_blocking(bool nonblocking) override {
            bool ok = stream().set_non_blocking(nonblocking);
            if (ok)
                setup_bio(nonblocking);
            return ok;
        }


        // -------- mbedTLS BIO callbacks


        int bio_send(const void* buf, size_t length) {
            if (!open_)
                return MBEDTLS_ERR_NET_CONN_RESET;
            return bio_return_value<false>(stream().write_r(buf, length));
        }


        int bio_recv(void* buf, size_t length) {
            if (!open_)
                return MBEDTLS_ERR_NET_CONN_RESET;
            return bio_return_value<true>(stream().read_r(buf, length));
        }


        int bio_recv_timeout(void* buf, size_t length, uint32_t timeout) {
            if (!open_)
                return MBEDTLS_ERR_NET_CONN_RESET;
            if (timeout > 0)
                stream().read_timeout(chrono::milliseconds(timeout));

            int n = bio_recv(buf, length);

            if (timeout > 0)
                stream().read_timeout(read_timeout_);
            return (int)n;
        }


        // -------- error handling


        // Translates mbedTLS error code to POSIX (errno)
        static int translate_mbed_err(int mbedErr) {
            switch (mbedErr) {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    return 0;
                case MBEDTLS_ERR_SSL_WANT_READ:
                case MBEDTLS_ERR_SSL_WANT_WRITE:
                    log(">>> mbedtls_socket returning EWOULDBLOCK");
                    return EWOULDBLOCK;
                case MBEDTLS_ERR_NET_CONN_RESET:
                    return ECONNRESET;
                case MBEDTLS_ERR_NET_RECV_FAILED:
                case MBEDTLS_ERR_NET_SEND_FAILED:
                    return EIO;
                default:
                    return mbedErr;
            }
        }


        // Handles an mbedTLS error return value during setup, closing me on error
        int check_mbed_setup(int ret, const char *fn) {
            if (ret != 0) {
                log_mbed_ret(ret, fn);
                reset(); // marks me as closed/invalid
                clear(translate_mbed_err(ret)); // sets last_error
                stream().close();
                open_ = false;
            }
            return ret;
        }


        // Handles an mbedTLS read/write return value, storing any error in last_error
        inline ssize_t check_mbed_io(int mbedResult) {
            if (mbedResult < 0) {
                clear(translate_mbed_err(mbedResult));     // sets last_error
                return -1;
            }
            return mbedResult;
        }


        // Handles an mbedTLS read/write return value, converting it to an ioresult.
        static inline ioresult ioresult_from_mbed(int mbedResult) {
            if (mbedResult < 0)
                return ioresult(0, translate_mbed_err(mbedResult));
            else
                return ioresult(mbedResult, 0);
        }


        // Translates ioresult to an mbedTLS error code to return from a BIO function.
        template <bool reading>
        static int bio_return_value(ioresult result) {
            if (result.error == 0)
                return (int)result.count;
            switch (result.error) {
                case EPIPE:
                case ECONNRESET:
                    return MBEDTLS_ERR_NET_CONN_RESET;
                case EINTR:
                case EWOULDBLOCK:
#if defined(EAGAIN) && EAGAIN != EWOULDBLOCK    // these are usually synonyms
                case EAGAIN:
#endif
                    log(">>> BIO returning MBEDTLS_ERR_SSL_WANT_%s", reading ?"READ":"WRITE");
                    return reading ? MBEDTLS_ERR_SSL_WANT_READ
                                   : MBEDTLS_ERR_SSL_WANT_WRITE;
                default:
                    return reading ? MBEDTLS_ERR_NET_RECV_FAILED
                                   : MBEDTLS_ERR_NET_SEND_FAILED;
            }
        }


    };


#pragma mark - CONTEXT:


    static tls_context *s_default_context = nullptr;

    mbedtls_context::cert *mbedtls_context::s_system_root_certs;


    tls_context& tls_context::default_context() {
        if (!s_default_context)
            s_default_context = new mbedtls_context();
        return *s_default_context;
    }


    // Returns a shared mbedTLS random-number generator context.
    static mbedtls_ctr_drbg_context* get_drbg_context() {
        static const char* k_entropy_personalization = "sockpp";
        static mbedtls_entropy_context  s_entropy;
        static mbedtls_ctr_drbg_context s_random_ctx;

        static once_flag once;
        call_once(once, []() {
            mbedtls_entropy_init( &s_entropy );
            mbedtls_ctr_drbg_init( &s_random_ctx );
            int ret = mbedtls_ctr_drbg_seed(&s_random_ctx, mbedtls_entropy_func, &s_entropy,
                                            (const uint8_t *)k_entropy_personalization,
                                            strlen(k_entropy_personalization));
            if (ret != 0) {
                log_mbed_ret(ret, "mbedtls_ctr_drbg_seed");
                throw sys_error(ret);   //FIXME: Not an errno; use different exception?
            }
        });
        return &s_random_ctx;
    }


    unique_ptr<mbedtls_context::cert> mbedtls_context::parse_cert(const std::string &cert_data, bool partialOk) {
        unique_ptr<cert> c(new cert);
        mbedtls_x509_crt_init(c.get());
        int ret = mbedtls_x509_crt_parse(c.get(),
                                         (const uint8_t*)cert_data.data(), cert_data.size() + 1);
        if (ret != 0) {
	        if(ret < 0 || !partialOk) {
		        log_mbed_ret(ret, "mbedtls_x509_crt_parse");
	        	if(ret > 0) {
	        		ret = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
	        	}
	        	
				throw sys_error(ret);
	        }
        }
        return c;
    }


    void mbedtls_context::set_root_certs(const std::string &cert_data) {
        root_certs_ = parse_cert(cert_data, true);
        mbedtls_ssl_conf_ca_chain(ssl_config_.get(), root_certs_.get(), nullptr);
    }


    // Returns the set of system trusted root CA certs.
    mbedtls_x509_crt* mbedtls_context::get_system_root_certs() {
        static once_flag once;
        call_once(once, []() {
            // One-time initialization:
            string certsPEM = read_system_root_certs();
            if (!certsPEM.empty())
                s_system_root_certs = parse_cert(certsPEM, true).release();
        });
        return s_system_root_certs;
    }


    mbedtls_context::mbedtls_context(role_t r)
    :ssl_config_(new mbedtls_ssl_config)
    {
        mbedtls_ssl_config_init(ssl_config_.get());
        mbedtls_ssl_conf_rng(ssl_config_.get(), mbedtls_ctr_drbg_random, get_drbg_context());
        int endpoint = (r == CLIENT) ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER;
        set_status(mbedtls_ssl_config_defaults(ssl_config_.get(),
                                               endpoint,
                                               MBEDTLS_SSL_TRANSPORT_STREAM,
                                               MBEDTLS_SSL_PRESET_DEFAULT));
        if (status() != 0)
            return;

        auto roots = get_system_root_certs();
        if (roots)
            mbedtls_ssl_conf_ca_chain(ssl_config_.get(), roots, nullptr);

        // Install a custom verification callback:
        mbedtls_ssl_conf_verify(
                        ssl_config_.get(),
                        [](void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
                            return ((mbedtls_context*)ctx)->verify_callback(crt,depth,flags);
                        },
                        this);
    }


    mbedtls_context::~mbedtls_context() {
        mbedtls_ssl_config_free(ssl_config_.get());
    }


    void mbedtls_context::set_logger(int threshold, Logger logger) {
        if (!logger_) {
            mbedtls_ssl_conf_dbg(ssl_config_.get(), [](void *ctx, int level, const char *file, int line,
                                                       const char *msg) {
                auto &logger = ((mbedtls_context*)ctx)->logger_;
                if (logger)
                    logger(level, file, line, msg);
            }, this);
        }
        logger_ = logger;
        mbedtls_debug_set_threshold(threshold);
    }


    void mbedtls_context::require_peer_cert(role_t forRole, bool require) {
        if (forRole != role())
            return;
        int authMode = (require ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_authmode(ssl_config_.get(), authMode);
    }


    void mbedtls_context::allow_only_certificate(const std::string &cert_data) {
        if (cert_data.empty()) {
            pinned_cert_.reset();
        } else {
            pinned_cert_ = parse_cert(cert_data, false);
        }
    }


    void mbedtls_context::allow_only_certificate(mbedtls_x509_crt *certificate) {
        string cert_data;
        if (certificate) {
            cert_data = string((const char*)certificate->raw.p, certificate->raw.len);
        }
        allow_only_certificate(cert_data);
    }


    // callback from mbedTLS cert validation (see above)
    int mbedtls_context::verify_callback(mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
        if (depth != 0)
            return 0;

        int status = -1;
        if (pinned_cert_) {
            status = (crt->raw.len == pinned_cert_->raw.len
                      && 0 == memcmp(crt->raw.p, pinned_cert_->raw.p, crt->raw.len));
        } else if (auto &callback = get_auth_callback(); callback) {
            string certData((const char*)crt->raw.p, crt->raw.len);
            status = callback(certData);
        }
        
        if (status > 0) {
            *flags &= ~(MBEDTLS_X509_BADCERT_NOT_TRUSTED | MBEDTLS_X509_BADCERT_CN_MISMATCH);
        } else if (status == 0) {
            *flags |= MBEDTLS_X509_BADCERT_NOT_TRUSTED;
        }
        return 0;
    }


    void mbedtls_context::set_identity(const std::string &certificate_data,
                                       const std::string &private_key_data)
    {
        auto ident_cert = parse_cert(certificate_data, false);

        unique_ptr<key> ident_key(new key);
        int err = mbedtls_pk_parse_key(ident_key.get(),
                                       (const uint8_t*) private_key_data.data(),
                                       private_key_data.size(), NULL, 0);
        if( err != 0 ) {
            log_mbed_ret(err, "mbedtls_pk_parse_key");
            throw sys_error(err);
        }

        set_identity(ident_cert.get(), ident_key.get());
        identity_cert_ = move(ident_cert);
        identity_key_  = move(ident_key);
    }


    void mbedtls_context::set_identity(mbedtls_x509_crt *certificate,
                                       mbedtls_pk_context *private_key)
    {
        mbedtls_ssl_conf_own_cert(ssl_config_.get(), certificate, private_key);
    }


    mbedtls_context::role_t mbedtls_context::role() {
        return (ssl_config_->endpoint == MBEDTLS_SSL_IS_CLIENT) ? CLIENT : SERVER;
    }


    unique_ptr<tls_socket> mbedtls_context::wrap_socket(std::unique_ptr<stream_socket> socket,
                                                        role_t socketRole,
                                                        const std::string &peer_name)
    {
        assert(socketRole == role());
        return unique_ptr<tls_socket>(new mbedtls_socket(move(socket), *this, peer_name));
    }


#pragma mark - PLATFORM SPECIFIC:


    // mbedTLS does not have built-in support for reading the OS's trusted root certs.

#ifdef __APPLE__
    // Read system root CA certs on macOS.
    // (Sadly, SecTrustCopyAnchorCertificates() is not available on iOS)
    static string read_system_root_certs() {
    #if TARGET_OS_OSX
        CFArrayRef roots;
        OSStatus err = SecTrustCopyAnchorCertificates(&roots);
        if (err)
            return {};
        CFDataRef pemData = nullptr;
        err =  SecItemExport(roots, kSecFormatPEMSequence, kSecItemPemArmour, nullptr, &pemData);
        CFRelease(roots);
        if (err)
            return {};
        string pem((const char*)CFDataGetBytePtr(pemData), CFDataGetLength(pemData));
        CFRelease(pemData);
        return pem;
    #else
        // fallback -- no certs
        return "";
    #endif
    }

#elif defined(_WIN32)
    // Windows:
    static string read_system_root_certs() {
	    PCCERT_CONTEXT pContext = nullptr;
	    HCERTSTORE hStore = CertOpenSystemStore(NULL, "ROOT");
		if(hStore == nullptr) {
			return "";
		}

		stringstream certs;
		while ((pContext = CertEnumCertificatesInStore(hStore, pContext))) {
			certs.write((const char*)pContext->pbCertEncoded, pContext->cbCertEncoded);
		}

		CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
		return certs.str();
    }

#else
    // Read system root CA certs on Linux using OpenSSL's cert directory
    static string read_system_root_certs() {
        static constexpr const char* CERTS_DIR  = "/etc/ssl/certs/";
        static constexpr const char* CERTS_FILE = "ca-certificates.crt";

        stringstream certs;
        char buf[1024];
        // Subroutine to append a file to the `certs` stream:
        auto read_file = [&](const string &file) {
            ifstream in(file);
            char last_char = '\n';
            while (in) {
                in.read(buf, sizeof(buf));
                auto n = in.gcount();
                if (n > 0) {
                    certs.write(buf, n);
                    last_char = buf[n-1];
                }
            }
            if (last_char != '\n')
                certs << '\n';
        };

        struct stat s;
        if (stat(CERTS_DIR, &s) == 0 && S_ISDIR(s.st_mode)) {
            string certs_file = string(CERTS_DIR) + CERTS_FILE;
            if (stat(certs_file.c_str(), &s) == 0) {
                // If there is a file containing all the certs, just read it:
                read_file(certs_file);
            } else {
                // Otherwise concatenate all the certs found in the dir:
                auto dir = opendir(CERTS_DIR);
                if (dir) {
                    struct dirent *ent;
                    while (nullptr != (ent = readdir(dir))) {
                        if (fnmatch("?*.pem", ent->d_name, FNM_PERIOD) == 0
                                    || fnmatch("?*.crt", ent->d_name, FNM_PERIOD) == 0)
                            read_file(string(CERTS_DIR) + ent->d_name);
                    }
                    closedir(dir);
                }
            }
        }
        return certs.str();
    }

#endif


}
