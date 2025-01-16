/**
 * @file socket.h
 *
 * The base `socket` and library initialization classes.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date December 2014
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

#ifndef __sockpp_socket_h
#define __sockpp_socket_h

#include <chrono>
#include <string>
#include <tuple>

#include "sockpp/result.h"
#include "sockpp/sock_address.h"
#include "sockpp/types.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

#if !defined(SOCKPP_SOCKET_T_DEFINED)
typedef int socket_t;                ///< The OS socket handle
const socket_t INVALID_SOCKET = -1;  ///< Invalid socket descriptor
    #define SOCKPP_SOCKET_T_DEFINED
#endif

/**
 * Converts a number of microseconds to a relative timeval.
 * @param dur A chrono duration of microseconds.
 * @return A timeval
 */
timeval to_timeval(const microseconds& dur);

/**
 * Converts a chrono duration to a relative timeval.
 * @param dur A chrono duration.
 * @return A timeval.
 */
template <class Rep, class Period>
timeval to_timeval(const duration<Rep, Period>& dur) {
    return to_timeval(std::chrono::duration_cast<microseconds>(dur));
}

/**
 * Converts a relative timeval to a chrono duration.
 * @param tv A timeval.
 * @return A chrono duration.
 */
inline microseconds to_duration(const timeval& tv) {
    auto dur = seconds{tv.tv_sec} + microseconds{tv.tv_usec};
    return std::chrono::duration_cast<microseconds>(dur);
}

/**
 * Converts an absolute timeval to a chrono time_point.
 * @param tv A timeval.
 * @return A chrono time_point.
 */
inline std::chrono::system_clock::time_point to_timepoint(const timeval& tv) {
    return std::chrono::system_clock::time_point{
        std::chrono::duration_cast<std::chrono::system_clock::duration>(to_duration(tv))
    };
}

/////////////////////////////////////////////////////////////////////////////
//                              socket_initializer
/////////////////////////////////////////////////////////////////////////////

/**
 * RAII singleton class to initialize and then shut down the library.
 *
 * The singleton object of this class should be created before any other
 * classes in the library are used.
 *
 * This is only required on some platforms, particularly Windows, but is
 * harmless on other platforms. On some, such as POSIX, the initializer sets
 * optional parameters for the library, and the destructor does nothing.
 */
class socket_initializer
{
    /** Initializes the sockpp library */
    socket_initializer();

    socket_initializer(const socket_initializer&) = delete;
    socket_initializer& operator=(const socket_initializer&) = delete;

    friend class socket;

public:
    /**
     * Creates the initializer singleton on the first call as a static
     * object which will get destructed on program termination with the
     * other static objects in reverse order as they were created,
     */
    static void initialize() { static socket_initializer sock_init; }
    /**
     * Destructor shuts down the sockpp library.
     */
    ~socket_initializer();
};

/**
 * Initializes the sockpp library.
 *
 * This initializes the library by creating a static singleton
 * RAII object which does any platform-specific initialization the first
 * time it's call in a process, and then performs cleanup automatically
 * when the object is destroyed at program termination.
 *
 * This should be call at least once before any other sockpp objects are
 * created or used. It can be called repeatedly with subsequent calls
 * having no effect
 *
 * This is primarily required for Win32, to startup the WinSock DLL.
 *
 * On Unix-style platforms it disables SIGPIPE signals. Errors are handled
 * by function return values and exceptions.
 */
void initialize();

/////////////////////////////////////////////////////////////////////////////
//									socket
/////////////////////////////////////////////////////////////////////////////

/**
 * Base class for socket objects.
 *
 * This class wraps an OS socket handle and maintains strict ownership
 * semantics. If a socket object has a valid, open socket, then it owns the
 * handle and will close it when the object is destroyed.
 *
 * Objects of this class are not copyable, but they are moveable.
 */
class socket
{
    /** The OS integer socket handle */
    socket_t handle_{INVALID_SOCKET};

    /**
     * The OS-specific function to close a socket handle/
     * @param h The OS socket handle.
     * @return An error code on failure
     */
    result<> close(socket_t h) noexcept;

    // Non-copyable.
    socket(const socket&) = delete;
    socket& operator=(const socket&) = delete;

protected:
    /**
     * Checks the value and if less than zero, returns the error.
     * @tparam T A signed integer type of any size
     * @param ret The return value from a library or system call.
     * @return The error code from the last error, if any.
     */
    template <typename T, typename Tout = T>
    static result<Tout> check_res(T ret) {
        return (ret < 0) ? result<Tout>::from_last_error() : result<Tout>{Tout(ret)};
    }
    /**
     * Checks the value and if less than zero, returns the error.
     * @tparam T A signed integer type of any size
     * @param ret The return value from a library or system call.
     * @return The error code from the last error, if any.
     */
    template <typename T>
    static result<> check_res_none(T ret) {
        return (ret < 0) ? result<>::from_last_error() : result<>{none{}};
    }

    /**
     * Checks the value of the socket handle, and if invalid, returns the
     * error.
     * @param s The socket return value from a library or system call.
     * @return The error code from the last error, if any.
     */
    static result<socket_t> check_socket(socket_t s) {
        return (s == INVALID_SOCKET) ? result<socket_t>::from_last_error()
                                     : result<socket_t>{s};
    }

// For non-Windows systems, routines to manipulate flags on the socket
// handle.
#if !defined(_WIN32)
    /** Gets the flags on the socket handle. */
    result<int> get_flags() const;
    /** Sets the flags on the socket handle. */
    result<> set_flags(int flags);
    /** Sets a single flag on or off */
    result<> set_flag(int flag, bool on = true);
#endif

public:
    /** A pair of base sockets */
    using socket_pair = std::tuple<socket, socket>;
    /**
     * Creates an unconnected (invalid) socket
     */
    socket() = default;
    /**
     * Creates a socket from an existing OS socket handle.
     * The object takes ownership of the handle and will close it when
     * destroyed.
     * @param h An OS socket handle.
     */
    explicit socket(socket_t h) noexcept : handle_{h} {}
    /**
     * Move constructor.
     * This takes ownership of the underlying handle in sock.
     * @param sock An rvalue reference to a socket object.
     */
    socket(socket&& sock) noexcept : handle_{std::move(sock.handle_)} {
        sock.handle_ = INVALID_SOCKET;
    }
    /**
     * Destructor closes the socket.
     */
    virtual ~socket() { close(); }
    /**
     * Creates an OS handle to a socket.
     *
     * This is normally only required for internal or diagnostics code.
     *
     * @param domain The communications domain for the sockets (i.e. the
     *  			 address family)
     * @param type The communication semantics for the sockets (SOCK_STREAM,
     *  		   SOCK_DGRAM, etc).
     * @param protocol The particular protocol to be used with the sockets
     *
     * @return An OS socket handle with the requested communications
     *  	   characteristics on success, or error code on failure.
     */
    static result<socket_t> create_handle(int domain, int type, int protocol = 0) noexcept {
        return check_socket(socket_t(::socket(domain, type, protocol)));
    }
    /**
     * Creates a socket with the specified communications characterics.
     * Not that this is not normally how a socket is creates in the sockpp
     * library. Applications would typically create a connector (client) or
     * acceptor (server) socket which would take care of the details.
     *
     * This is included for completeness or for creating different types of
     * sockets than are supported by the library.
     *
     * @param domain The communications domain for the sockets (i.e. the
     *  			 address family)
     * @param type The communication semantics for the sockets (SOCK_STREAM,
     *  		   SOCK_DGRAM, etc).
     * @param protocol The particular protocol to be used with the sockets
     *
     * @return A socket with the requested communications characteristics.
     */
    static result<socket> create(int domain, int type, int protocol = 0) noexcept;
    /**
     * Determines if the socket is open (valid).
     * @return @em true if the socket is open, @em false otherwise.
     */
    bool is_open() const { return handle_ != INVALID_SOCKET; }
    /**
     * Determines if the socket is closed or in an error state.
     * @return @em true if the socket is closed or in an error state, @em
     *  	   false otherwise.
     */
    bool operator!() const { return handle_ == INVALID_SOCKET; }
    /**
     * Determines if the socket is open and in an error-free state.
     * @return @em true if the socket is open and in an error-free state,
     *  	   @em false otherwise.
     */
    explicit operator bool() const { return handle_ != INVALID_SOCKET; }
    /**
     * Get the underlying OS socket handle.
     * @return The underlying OS socket handle.
     */
    socket_t handle() const { return handle_; }
    /**
     * Gets the network family of the address to which the socket is bound,
     * if it is bound.
     * @return The network family of the address (AF_INET, etc) to which the
     *             socket is bound. If the socket is not bound or the
     *             address is not known, returns AF_UNSPEC.
     */
    virtual sa_family_t family() const { return address().family(); }
    /**
     * Make a copy of this socket.
     * This creates a new object with an independent lifetime, but refers
     * back to this same socket. On most systems, this duplicates the file
     * handle using the dup() call. A typical use of this is to have
     * separate threads for reading and writing the socket. One thread would
     * get the original socket and the other would get the cloned one.
     * @return A new socket object that refers to the same socket as this
     *  	   one.
     */
    result<socket> clone() const;
    /**
     * Creates a pair of connected sockets.
     *
     * Whether this will work at all is highly system and domain dependent.
     * Currently it is only known to work for Unix-domain sockets on *nix
     * systems.
     *
     * Note that applications would normally call this from a derived socket
     * class which would return the properly type-cast sockets to match the
     * `domain` and `type`.
     *
     * @param domain The communications domain for the sockets (i.e. the
     *  			 address family)
     * @param type The communication semantics for the sockets (SOCK_STREAM,
     *  		   SOCK_DGRAM, etc).
     * @param protocol The particular protocol to be used with the sockets
     *
     * @return A pair (std::tuple) of sockets on success, or an error code
     *         on failure.
     */
    static result<socket_pair> pair(int domain, int type, int protocol = 0) noexcept;
    /**
     * Releases ownership of the underlying socket object.
     * @return The OS socket handle.
     */
    socket_t release() noexcept {
        socket_t h = handle_;
        handle_ = INVALID_SOCKET;
        return h;
    }
    /**
     * Replaces the underlying managed socket object.
     * This closes the existing socket before replacing the handle.
     * @param h The new socket handle to manage.
     */
    void reset(socket_t h = INVALID_SOCKET) noexcept;
    /**
     * Move assignment.
     * This assigns ownership of the socket from the other object to this
     * one.
     * @return A reference to this object.
     */
    socket& operator=(socket&& sock) noexcept {
        // Give our handle to the other to close.
        std::swap(handle_, sock.handle_);
        return *this;
    }
    /**
     * Binds the socket to the specified address.
     * @param addr The address to which we get bound.
     * @param reuse A reuse option for the socket. This can be SO_REUSEADDR
     *              or SO_REUSEPORT if supported on the platform, and is set
     *              before it tries to bind. A value of zero doesn't set any
     *              option on the socket.
     * @return @em true on success, @em false on error
     */
    result<> bind(const sock_address& addr, int reuse = 0) noexcept;
    /**
     * Gets the local address to which the socket is bound.
     * @return The local address to which the socket is bound.
     */
    sock_address_any address() const;
    /**
     * Gets the address of the remote peer, if this socket is connected.
     * @return The address of the remote peer, if this socket is connected.
     */
    sock_address_any peer_address() const;
    /**
     * Gets the value of a socket option.
     *
     * This is a thin wrapper for the system `getsockopt`.
     *
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param optval The buffer for the value to retrieve
     * @param optlen Initially contains the length of the buffer, and on
     *               return contains the length of the value retrieved.
     *
     * @return bool @em true if the value was retrieved, @em false if an error
     *  	   occurred.
     */
    result<> get_option(
        int level, int optname, void* optval, socklen_t* optlen
    ) const noexcept;
    /**
     * Gets the value of a socket option.
     *
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param val The value to retrieve
     * @return An error code on failure.
     */
    template <typename T>
    result<> get_option(int level, int optname, T* val) const noexcept {
        socklen_t len = sizeof(T);
        return get_option(level, optname, (void*)val, &len);
    }
    /**
     * Gets the value of a socket option.
     *
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @return The option value on success, an error code on failure.
     */
    template <typename T>
    result<T> get_option(int level, int optname) const noexcept {
        T val{};
        auto res = get_option(level, optname, &val);
        return (res) ? result<T>{val} : result<T>{res.error()};
    }
    /**
     * Gets the value of a boolean socket option.
     * This maps the OS options that use an C-style integer value where 0 is
     * false, and non-zero is true.
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @return The option's value on success, an error code on failure.
     */
    result<bool> get_option(int level, int optname) const noexcept {
        int val{};
        auto res = get_option(level, optname, &val);
        return (res) ? result<bool>{val != 0} : result<bool>{res.error()};
    }
    /**
     * Sets the value of a socket option.
     *
     * This is a thin wrapper for the system `setsockopt`.
     *
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param optval The buffer with the value to set.
     * @param optlen Contains the length of the value buffer.
     *
     * @return bool @em true if the value was set, @em false if an error
     *  	   occurred.
     */
    result<> set_option(
        int level, int optname, const void* optval, socklen_t optlen
    ) noexcept;
    /**
     * Sets the value of a socket option.
     *
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param val The value to set.
     *
     * @return bool @em true if the value was set, @em false if an error
     *  	   occurred.
     */
    template <typename T>
    result<> set_option(int level, int optname, const T& val) noexcept {
        return set_option(level, optname, &val, sizeof(T));
    }
    /**
     * Sets the value of a boolean socket option.
     * This maps the OS options that use an C-style integer value where 0 is
     * false, and non-zero is true.
     * @param level The protocol level at which the option resides, such as
     *  			SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param val The value to set.
     *
     * @return An error code on failure.
     */
    result<> set_option(int level, int optname, const bool& val) noexcept {
        return set_option(level, optname, int(val));
    }

    // ----- Specific socket options -----

    /**
     * Places the socket into or out of non-blocking mode.
     * When in non-blocking mode, a call that is not immediately ready to
     * complete (read, write, accept, etc) will return immediately with the
     * error errc::operation_would_block.
     * @param on Whether to turn non-blocking mode on or off.
     * @return @em true on success, @em false on failure.
     */
    virtual result<> set_non_blocking(bool on = true);

#if !defined(_WIN32)
    /**
     * Determines if the socket is non-blocking
     */
    virtual bool is_non_blocking() const;
#endif
    /**
     * Gets the value of the `SO_REUSEADDR` option on the socket.
     * @return The value of the `SO_REUSEADDR` option on the socket on
     *         success, an error code on failure.
     */
    result<bool> reuse_address() const noexcept {
        return get_option<bool>(SOL_SOCKET, SO_REUSEADDR);
    }
    /**
     * Sets the value of the `SO_REUSEADDR` option on the socket.
     * @param on The desired value of the `SO_REUSEADDR` option
     * @return An error code on failure.
     */
    result<> reuse_address(bool on) noexcept {
        return set_option(SOL_SOCKET, SO_REUSEADDR, on);
    }
#if !defined(_WIN32) && !defined(__CYGWIN__)
    /**
     * Gets the value of the `SO_REUSEPORT` option on the socket.
     * @return The value of the `SO_REUSEPORT` option on the socket on
     *         success, an error code on failure.
     */
    result<bool> reuse_port() const noexcept {
        return get_option<bool>(SOL_SOCKET, SO_REUSEPORT);
    }
    /**
     * Sets the value of the `SO_REUSEPORT` option on the socket.
     * @param on The desired value of the `SO_REUSEPORT` option
     * @return An error code on failure.
     */
    result<> reuse_port(bool on) noexcept { return set_option(SOL_SOCKET, SO_REUSEPORT, on); }
#endif
    /**
     * Gets the value of the `SO_RCVBUF` option on the socket.
     * This is the size of the OS receive buffer for the socket.
     * @return The size of the OS receive buffer.
     */
    result<unsigned int> recv_buffer_size() const noexcept {
        return get_option<unsigned int>(SOL_SOCKET, SO_RCVBUF);
    }
    /**
     * Sets the value of the `SO_RCVBUF` option on the socket.
     * This is the size of the OS receive buffer for the socket.
     * @param sz The desired size of the OS receive buffer.
     * @return The error code on failure.
     */
    result<> recv_buffer_size(unsigned int sz) noexcept {
        return set_option<unsigned int>(SOL_SOCKET, SO_RCVBUF, sz);
    }
    /**
     * Gets the value of the `SO_SNDBUF` option on the socket.
     * This is the size of the OS send buffer for the socket.
     * @return The size of the OS send buffer.
     */
    result<unsigned int> send_buffer_size() const noexcept {
        return get_option<unsigned int>(SOL_SOCKET, SO_SNDBUF);
    }
    /**
     * Sets the value of the `SO_SNDBUF` option on the socket.
     * This is the size of the OS send buffer for the socket.
     * @param sz The desired size of the OS send buffer.
     * @return The error code on failure.
     */
    result<> send_buffer_size(unsigned int sz) noexcept {
        return set_option<unsigned int>(SOL_SOCKET, SO_SNDBUF, sz);
    }
    /**
     * Shuts down all or part of the full-duplex connection.
     * @param how Which part of the connection should be shut:
     *  	@li SHUT_RD   (0) Further reads disallowed.
     *  	@li SHUT_WR   (1) Further writes disallowed
     *  	@li SHUT_RDWR (2) Further reads and writes disallowed.
     * @return @em true on success, @em false on error.
     */
    result<> shutdown(int how = SHUT_RDWR);
    /**
     * Closes the socket.
     * After closing the socket, the handle is @em invalid, and can not be
     * used again until reassigned.
     * @return @em true if the sock is closed, @em false on error.
     */
    virtual result<> close();

    // ----- I/O -----

    /**
     * Sends a message to the socket at the specified address.
     * @param buf The data to send.
     * @param n The number of bytes in the data buffer.
     * @param flags The flags. See send(2).
     * @param addr The remote destination of the data.
     * @return The number of bytes sent on success or, the error code on
     *         failure.
     */
    result<size_t> send_to(const void* buf, size_t n, int flags, const sock_address& addr) {
#if defined(_WIN32)
        auto cbuf = reinterpret_cast<const char*>(buf);
        return check_res<size_t>(
            ::sendto(handle(), cbuf, int(n), flags, addr.sockaddr_ptr(), addr.size())
        );
#else
        return check_res<ssize_t, size_t>(
            ::sendto(handle(), buf, n, flags, addr.sockaddr_ptr(), addr.size())
        );
#endif
    }
    /**
     * Sends a message to another socket.
     * @param buf The data to send.
     * @param n The number of bytes in the data buffer.
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const void* buf, size_t n, const sock_address& addr) {
        return send_to(buf, n, 0, addr);
    }
    /**
     * Sends a string to another socket.
     * @param s The string to send.
     * @param addr The remote destination of the data.
     * @return the number of bytes sent on success or, @em -1 on failure.
     */
    result<size_t> send_to(const string& s, const sock_address& addr) {
        return send_to(s.data(), s.length(), 0, addr);
    }
    /**
     * Sends a message to the socket at the default address.
     * The socket should be connected before calling this.
     * @param buf The date to send.
     * @param n The number of bytes in the data buffer.
     * @param flags The option bit flags. See send(2).
     * @return @em zero on success, @em -1 on failure.
     */
    result<size_t> send(const void* buf, size_t n, int flags = 0) {
#if defined(_WIN32)
        return check_res<ssize_t, size_t>(
            ::send(handle(), reinterpret_cast<const char*>(buf), int(n), flags)
        );
#else
        return check_res<ssize_t, size_t>(::send(handle(), buf, n, flags));
#endif
    }
    /**
     * Sends a string to the socket at the default address.
     * The socket should be connected before calling this
     * @param s The string to send.
     * @param flags The option bit flags. See send(2).
     * @return @em zero on success, @em -1 on failure.
     */
    result<size_t> send(const string& s, int flags = 0) {
        return send(s.data(), s.length(), flags);
    }
    /**
     * Receives a message on the socket.
     * @param buf Buffer to get the incoming data.
     * @param n The number of bytes to read.
     * @param flags The option bit flags. See send(2).
     * @param srcAddr Receives the address of the peer that sent the
     *  			   message
     * @return The number of bytes read or @em -1 on error.
     */
    result<size_t> recv_from(void* buf, size_t n, int flags, sock_address* srcAddr = nullptr);
    /**
     * Receives a message on the socket.
     * @param buf Buffer to get the incoming data.
     * @param n The number of bytes to read.
     * @param srcAddr Receives the address of the peer that sent the
     *  			   message
     * @return The number of bytes read or @em -1 on error.
     */
    result<size_t> recv_from(void* buf, size_t n, sock_address* srcAddr = nullptr) {
        return recv_from(buf, n, 0, srcAddr);
    }
    /**
     * Receives a message on the socket.
     * @param buf Buffer to get the incoming data.
     * @param n The number of bytes to read.
     * @param flags The option bit flags. See send(2).
     * @return The number of bytes read or @em -1 on error.
     */
    result<size_t> recv(void* buf, size_t n, int flags = 0) {
#if defined(_WIN32)
        return check_res<ssize_t, size_t>(
            ::recv(handle(), reinterpret_cast<char*>(buf), int(n), flags)
        );
#else
        return check_res<ssize_t, size_t>(::recv(handle(), buf, n, flags));
#endif
    }
};

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_socket_h
