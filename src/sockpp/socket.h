/**
 * @file socket.h
 *
 * Classes for TCP & UDP socket.
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

#ifndef __sockpp_socket_h
#define __sockpp_socket_h

#include "sockpp/inet_address.h"
#include <chrono>
#include <string>
#include <algorithm>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

#if !defined(SOCKPP_SOCKET_T_DEFINED)
	typedef int socket_t;				///< The OS socket handle
	const socket_t INVALID_SOCKET = -1;	///< Invalid socket descriptor
	#define SOCKPP_SOCKET_T_DEFINED
#endif

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
	socket_t handle_;
	/** Cache of the last error (errno) */
	mutable int lastErr_;
	/**
	 * The OS-specific socket close function
	 * @param h The integer socket handle.
	 */
	void close(socket_t h);

	// Non-copyable.
	socket(const socket&) =delete;
	socket& operator=(const socket&) =delete;

protected:
	/**
	 * OS-specific means to retrieve the last error from an operation.
	 * This should be called after a failed system call to set the 
	 * lastErr_ member variable. Normally this would be called from 
	 * @ref check_ret. 
	 */
	static int get_last_error();
	/**
	 * Cache the last system error code into this object. 
	 * This should be called after a failed system call to store the error 
	 * value.
	 */
	void set_last_error() {
		lastErr_ = get_last_error();
	}
	/**
	 * Checks the value and if less than zero, sets last error.
	 * @param ret The return value from a library or system call.
	 * @return Return the value sent to it, ret.
	 */
	int check_ret(int ret) const{
		lastErr_ = (ret < 0) ? get_last_error() : 0;
		return ret;
	}
	/**
	 * Checks the value and if less than zero, sets last error.
	 * @param ret The return value from a library or system call.
	 * @return @em true if the value is a typical system success value (>=0) 
	 *  	   or @em false is is an error (<0)
	 */
	bool check_ret_bool(int ret) const{
		lastErr_ = (ret < 0) ? get_last_error() : 0;
		return ret >= 0;
	}

public:
	/**
	 * Creates an unconnected (invalid) socket
	 */
	socket() : handle_(INVALID_SOCKET), lastErr_(0) {}
	/**
	 * Creates a socket from an OS socket handle.
	 * The object takes ownership of the handle and will close it when
	 * destroyed.
	 * @param h An OS socket handle.
	 */
	explicit socket(socket_t h) : handle_(h), lastErr_(0) {}
	/**
	 * Move constructor.
	 * This takes ownership of the underlying handle in sock.
	 * @param An rvalue reference to a socket object.
	 */
	socket(socket&& sock) : handle_(sock.handle_), lastErr_(0) {
		sock.handle_ = INVALID_SOCKET;
	}
	/**
	 * Destructor closes the socket.
	 */
	virtual ~socket() { close(); }
	/**
	 * Initializes the Socket library.
	 * This is only required for Win32. On platforms that use a standard
	 * socket implementation this is an empty call.
	 */
	static void initialize();
	/**
	 * Shuts down the socket library.
	 * This is only required for Win32. On platforms that use a standard
	 * socket implementation this is an empty call.
	 */
	static void destroy();
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
	bool operator!() const {
		return handle_ == INVALID_SOCKET || lastErr_ != 0;
	}
	/**
	 * Determines if the socket is open and in an error-free state.
	 * @return @em true if the socket is open and in an error-free state,
	 *  	   @em false otherwise.
	 */
	explicit operator bool() const {
		return handle_ != INVALID_SOCKET && lastErr_ == 0;
	}
	/**
	 * Get the underlying OS socket handle.
	 * @return The underlying OS socket handle.
	 */
	socket_t handle() const { return handle_; }
	/**
	 * Clears the error flag for the object.
	 * @param val The value to set the flag, normally zero.
	 */
	void clear(int val=0) { lastErr_ = val; }
	/**
	 * Releases ownership of the underlying socket object.
	 * @return The OS socket handle.
	 */
	socket_t release() {
		socket_t h = handle_;
		handle_ = INVALID_SOCKET;
		return h;
	}
	/**
	 * Replaces the underlying managed socket object.
	 * @param h The new socket handle to manage.
	 */
	void reset(socket_t h=INVALID_SOCKET);
	/**
	 * Move assignment.
	 * This assigns ownership of the socket from the other object to this
	 * one.
	 * @return A reference to this object.
	 */
	socket& operator=(socket&& sock) {
		handle_ = sock.handle_;
		sock.handle_ = INVALID_SOCKET;
		return *this;
	}
	/**
	 * Gets the local address to which the socket is bound.
	 * @param addr Gets the local address to which the socket is bound.
	 * @return @em true on success, @em false on error
	 */
	bool address(inet_address& addr) const;
	/**
	 * Gets the local address to which the socket is bound.
	 * @return The local address to which the socket is bound.
	 * @throw sys_error on error
	 */
	inet_address address() const;
	/**
	 * Gets the address of the remote peer, if this socket is connected.
	 * @param The address of the remote peer, if this socket is connected.
	 * @return @em true on success, @em false on error
	 */
	bool peer_address(inet_address& addr) const;
	/**
	 * Gets the address of the remote peer, if this socket is connected.
	 * @return The address of the remote peer, if this socket is connected.
	 * @throw sys_error on error
	 */
	inet_address peer_address() const;
	/**
	 * Gets the code for the last errror.
	 * This is typically the code from the underlying OS operation.
	 * @return The code for the last errror.
	 */
	int last_error() const { return lastErr_; }
	/**
	 * Closes the socket.
	 * After closing the socket, the handle is @em invalid, and can not be
	 * used again until reassigned.
	 */
	void close();
};

/////////////////////////////////////////////////////////////////////////////
/// Class that wraps UDP sockets

#if !defined(WIN32)

class udp_socket : public socket
{
protected:
	static socket_t create() {
		return (socket_t) ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

public:
	/**
	 * Creates an unbound UDP socket.
	 * This can be used as a client or later bound as a server socket.
	 */
	udp_socket() : socket(create()) {}
	/**
	 * Creates a UDP socket and binds it to the specified port.
	 * @param port The port to bind.
	 */
	udp_socket(in_port_t port);
	/**
	 * Creates a UDP socket and binds it to the address.
	 * @param addr The address to bind.
	 */
	udp_socket(const inet_address& addr);
	/**
	 * Binds the socket to the local address.
	 * UDP sockets can bind to a local address/adapter to filter which
	 * incoming packets to receive.
	 * @param addr
	 * @return @em true on success, @em false on failure
	 */
	bool bind(const inet_address& addr) {
		return check_ret_bool(::bind(handle(), addr.sockaddr_ptr(),
									 sizeof(sockaddr_in)));
	}
	/**
	 * Connects the socket to the remote address.
	 * In the case of UDP sockets, this does not create an actual
	 * connection, but rather specified the address to which datagrams are
	 * sent by default, and the only address from which datagrams are
	 * received.
	 * @param addr
	 * @return @em true on success, @em false on failure
	 */
	bool connect(const inet_address& addr) {
		return check_ret_bool(::connect(handle(), addr.sockaddr_ptr(),
										sizeof(sockaddr_in)));
	}

	// ----- I/O -----

	/**
	 * Sends a UDP packet to the specified internet address.
	 * @param buf The date to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	int	sendto(const void* buf, size_t n, const inet_address& addr) {
		return check_ret(::sendto(handle(), buf, n, 0,
								  addr.sockaddr_ptr(), addr.size()));
	}
	int sendto(const std::string& s, const inet_address& addr) {
		return sendto(s.data(), s.length(), addr);
	}
	/**
	 * Sends a UDP packet to the specified internet address.
	 * @param buf The date to send.
	 * @param n The number of bytes in the data buffer.
	 * @param flags
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	int	sendto(const void* buf, size_t n, int flags, const inet_address& addr) {
		return check_ret(::sendto(handle(), buf, n, flags,
								  addr.sockaddr_ptr(), addr.size()));
	}
	int	sendto(const std::string& s, int flags, const inet_address& addr) {
		return sendto(s.data(), s.length(), flags, addr);
	}
	/**
	 * Sends a UDP packet to the default address.
	 * The socket should be connected before calling this
	 * @param buf The date to send.
	 * @param n The number of bytes in the data buffer.
	 * @param addr The remote destination of the data.
	 * @return @em zero on success, @em -1 on failure.
	 */
	int	send(const void* buf, size_t n, int flags=0) {
		return check_ret(::send(handle(), buf, n, flags));
	}
	int	send(const std::string& s, int flags=0) {
		return send(s.data(), s.length(), flags);
	}
	/**
	 * Receives a UDP packet.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Gets the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recvfrom(void* buf, size_t n, int flags, inet_address& addr);
	/**
	 * Receives a UDP packet.
	 * @sa recvfrom
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @param addr Gets the address of the peer that sent the message
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recvfrom(void* buf, size_t n, inet_address& addr) {
		return recvfrom(buf, n, 0, addr);
	}
	/**
	 * Receives a UDP packet.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to read.
	 * @return The number of bytes read or @em -1 on error.
	 */
	int	recv(void* buf, size_t n, int flags=0) {
		return check_ret(::recv(handle(), buf, n, flags));
	}
};

#endif	// !WIN32

/////////////////////////////////////////////////////////////////////////////

/**
 * Class for TCP sockets.
 * This is the streaming connection between two TCP peers. It looks like a
 * readable/writeable device.
 */
class tcp_socket : public socket
{
protected:
	friend class tcp_acceptor;
	/**
	 * Creates a TCP socket.
	 * @return An OS handle to a TCP socket.
	 */
	static socket_t create() {
		return (socket_t) ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

public:
	/**
	 * Creates an unconnected TCP socket.
	 */
	tcp_socket() {}
	/**
	 * Creates a TCP socket from an existing OS socket handle and claims
	 * ownership of the handle.
	 * @param sock
	 */
	explicit tcp_socket(socket_t sock) : socket(sock) {}
	/**
	 * Creates a TCP socket by copying the socket handle from the specified
	 * socket object and transfers ownership of the socket.
	 */
	tcp_socket(tcp_socket&& sock) : socket(std::move(sock)) {}

	/**
	 * Open the socket.
	 * @return @em true on success, @em false on failure.
	 */
	bool open();

	// ----- IDevice Interface -----

	/**
	 * Reads from the port
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to try to read.
	 * @return The number of bytes read on success, or @em -1 on error.
	 */
	virtual ssize_t read(void *buf, size_t n);
	/**
	 * Best effort attempts to read the specified number of bytes.
	 * This will make repeated read attempts until all the bytes are read in
	 * or until an error occurs.
	 * @param buf Buffer to get the incoming data.
	 * @param n The number of bytes to try to read.
	 * @return The number of bytes read on success, or @em -1 on error. If
	 *  	   successful, the number of bytes read should always be 'n'.
	 */
	virtual ssize_t read_n(void *buf, size_t n);
	/**
	 * Set a timeout for read operations.
	 * Sets the timout that the device uses for read operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
	 */
	virtual bool read_timeout(const std::chrono::microseconds& to);
	/**
	 * Set a timeout for read operations.
	 * Sets the timout that the device uses for read operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
	 */
	template<class Rep, class Period>
	bool read_timeout(const std::chrono::duration<Rep,Period>& to) {
		return read_timeout(std::chrono::duration_cast<std::chrono::microseconds>(to));
	}
	/**
	 * Writes the buffer to the socket.
	 * @param buf The buffer to write
	 * @param n The number of bytes in the buffer.
	 * @return The number of bytes written, or @em -1 on error.
	 */
	virtual ssize_t write(const void *buf, size_t n);
	/**
	 * Best effort attempt to write the whole buffer to the socket.
	 * @param buf The buffer to write
	 * @param n The number of bytes in the buffer.
	 * @return The number of bytes written, or @em -1 on error. If
	 *  	   successful, the number of bytes written should always be 'n'.
	 */
	virtual ssize_t write_n(const void *buf, size_t n);
	/**
	 * Best effort attempt to write a string to the socket.
	 * @param s The string to write.
	 * @return The number of bytes written, or @em 1 on error. On success,
	 *  	   the number of bytes written should always be the length of
	 *  	   the string.
	 */
	virtual int write(const std::string& s) {
		return write_n(s.data(), s.size());
	}
	/**
	 * Set a timeout for write operations.
	 * Sets the timout that the device uses for write operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
	 */
	virtual bool write_timeout(const std::chrono::microseconds& to);
	/**
	 * Set a timeout for write operations.
	 * Sets the timout that the device uses for write operations. Not all
	 * devices support timouts, so the caller should prepare for failure.
	 * @param to The amount of time to wait for the operation to complete.
	 * @return @em true on success, @em false on failure.
	 */
	template<class Rep, class Period>
	bool write_timeout(const std::chrono::duration<Rep,Period>& to) {
		return write_timeout(std::chrono::duration_cast<std::chrono::microseconds>(to));
	}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * RAII class to initialize and then shut down the library.
 * A single object of this class can be declared before any other classes in
 * the library are used. The lifetime of the object should span the use of
 * the other classes in the library, so declaring an object at the top of
 * main() is usually the best choice.
 * This is only required on some platforms, particularly Windows, but is
 * harmless on other platforms. On some, such as POSIX, the initializer sets
 * optional parameters for the library, and the destructor does nothing.
 */
class socket_initializer
{
public:
	socket_initializer() { socket::initialize(); }
	~socket_initializer() { socket::destroy(); }
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_socket_h

