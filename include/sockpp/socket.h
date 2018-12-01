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

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

#if !defined(SOCKPP_SOCKET_T_DEFINED)
	typedef int socket_t;				///< The OS socket handle
	const socket_t INVALID_SOCKET = -1;	///< Invalid socket descriptor
	#define SOCKPP_SOCKET_T_DEFINED
#endif

#if !defined(WIN32)
	timeval to_timeval(const std::chrono::microseconds& dur);
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
	socket(socket&& sock) noexcept : handle_(sock.handle_), lastErr_(0) {
		sock.handle_ = INVALID_SOCKET;
	}
	/**
	 * Destructor closes the socket.
	 */
	virtual ~socket() { close(); }
	/**
	 * Initializes the socket (sockpp) library.
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
	 * Creates a new socket that refers to this one.
	 * This creates a new object with an independent lifetime, but refers
	 * back to this same socket. On most systems, this duplicates the file
	 * handle using the dup() call.
	 * A typical use of this is to have separate threads for reading and
	 * writing the socket. One thread would get the original socket and the
	 * other would get the cloned one.
	 * @return A new socket object that refers to the same socket as this
	 *  	   one.
	 */
	socket clone();
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
	socket& operator=(socket&& sock) noexcept {
		// Give our handle to the other to close.
		std::swap(handle_, sock.handle_);
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
	 * Gets a string describing the last errror.
	 * This is typically the returned message from the system strerror()
	 * call.
	 * @return A string describing the last errror.
	 */
	std::string last_error_str() const;
	/**
	 * Closes the socket.
	 * After closing the socket, the handle is @em invalid, and can not be
	 * used again until reassigned.
	 */
	void close();
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

