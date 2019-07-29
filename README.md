# sockpp

[![Build Status](https://travis-ci.org/fpagliughi/sockpp.svg?branch=master)](https://travis-ci.org/fpagliughi/sockpp)

Simple, modern, C++ socket library.

This is a fairly low-level C++ wrapper around the Berkeley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` wraps a system integer socket handle, and maintains its lifetime. When the object goes out of scope, it closes the underlying socket handle. Socket objects are generally _moveable_ but not _copyable_. A socket object can be transferred from one scope (or thread) to another using `std::move()`.

All code in the library lives within the `sockpp` C++ namespace.

## Latest News

To keep up with the latest announcements for this project, follow me at:

**Twitter:** [@fmpagliughi](https://twitter.com/fmpagliughi)

## What's New in Version v0.4

Version 0.4 added support for IPv6 and refactored the class hierarchies to better support the different address families without so much redundant code.

 - IPv6 support: `inet6_address`, `tcp6_acceptor`, `tcp_connector`, etc.
 - **(Breaking change)** The `sock_address` class is now contains storage for any type of address and follows copy semantics. Previously it was a non-owning reference class. That reference class now exists as `sock_addresss_ref`.
 - Generic base classses are being re-implemented to use _sock_address_ and _sock_address_ref_ as generic addresses.
 - **(Breaking change)** In the `socket` class(es) the `bool address(address&)` and `bool peer_address(addr&)` forms of getting the socket addresses have been removed in favor of the ones that simply return the address.
 Added `get_option()` and `set_option()` methods to the base `socket`class.
 - The GNU Make build system (Makefile) was deprecated and removed.
 
## Coming Soon
 
 The following improvements are soon to follow:
 
  - **Proper UDP support.** The existing `datagram_socket` will serve as a base for UDP socket classes for all the families supported (IPv4, v6, and Unix-Domain).
  
  - **Better use of templates and generics.** Currently there is a lot of redundant code in the implementations for the different familiess. This can likely be replaced with some template classes, while keeping the public API compatible.
  
  - **SSL Sockets.** It might be nice to add optional support for secure sockets.
 
## Building the Library

CMake is the supported build system. 

### Requirements:

 - CMake v3.5 or newer.
 - Doxygen (optional) to generate API docs.

Build like this on Linux:

```
$ cd sockpp
$ mkdir build ; cd build
$ cmake ..
$ make
$ sudo make install
```

### Build Options

The library has several build options via CMake to choose between creating a static or shared (dynamic) library - or both. It also allows you to build the example options, and if Doxygen is

Variable | Default Value | Description
------------ | ------------- | -------------
SOCKPP_BUILD_SHARED | ON | Whether to build the shared library
SOCKPP_BUILD_STATIC | OFF | Whether to build the static library
SOCKPP_BUILD_DOCUMENTATION | OFF | Create and install the HTML based API documentation (requires Doxygen)
SOCKPP_BUILD_EXAMPLES | OFF | Build example programs
SOCKPP_BUILD_TESTS | OFF | Build the unit tests

 
## TCP Sockets

TCP and other "streaming" network applications are usually set up as either servers or clients. An acceptor is used to create a TCP/streaming server. It binds an address and listens on a known port to accept incoming connections. When a connection is accepted, a new, streaming socket is created. That new socket can be handled directly or moved to a thread (or thread pool) for processing.

Conversely, to create a TCP client, a connector object is created and connected to a server at a known address (typically host and socket). When connected, the socket is a streaming one which can be used to read and write, directly.

For IPv4 the `tcp_acceptor` and `tcp_connector` classes are used to create servers and clients, respectively. These use the `inet_address` class to specify endpoint addresses composed of a 32-bit host address and a 16-bit port number.

### TCP server: `tcp_acceptor`

The `tcp_acceptor` is used to set up a server and listen for incoming connections.

    int16_t port = 12345;
    sockpp::tcp_acceptor acc(port);

    if (!acc)
        report_error(acc.last_error_str());

    // Accept a new client connection
    sockpp::tcp_socket sock = acc.accept();

The acceptor normally sits in a loop accepting new connections, and passes them off to another process, thread, or thread pool to interact with the client. In standard C++, this could look like:

    while (true) {
        // Accept a new client connection
        sockpp::tcp_socket sock = acc.accept();

        if (!sock) {
            cerr << "Error accepting incoming connection: " 
                << acc.last_error_str() << endl;
        }
        else {
            // Create a thread and transfer the new stream to it.
            thread thr(run_echo, std::move(sock));
            thr.detach();
        }
    }

The hazards of a thread-per-connection design is well documented, but the same technique can be used to pass the socket into a thread pool, if one is available.

### TCP Client: `tcp_connector`

The TCP client is somewhat simpler in that a `tcp_connector` object is created and connected, then can be used to read and write data directly.

    sockpp::tcp_connector conn;
    int16_t port = 12345;

    if (!conn.connect(sockpp::inet_address("localhost", port)))
        report_error(acc.last_error_str());

    conn.write_n("Hello", 5);
	
    char buf[5];
    int n = conn.read(buf, 5);

### IPv6

The same style of  connectors and acceptors can be used for TCP connections over IPv6 using the classes:

    inet6_address
    tcp6_connector
    tcp6_acceptor
    
### Unix Domain Sockets

The same us true for local connection on *nix systems that implement Unix Domain Sockets. For that use the classes:

    unix_address
    unix_connector
    unix_acceptor
    
