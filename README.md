# sockpp

[![Build Status](https://travis-ci.org/fpagliughi/sockpp.svg?branch=master)](https://travis-ci.org/fpagliughi/sockpp)

Simple, modern, C++ socket library.

This is a fairly low-level C++ wrapper around the Berkley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` wraps a system integer socket handle, and maintains its lifetime. When the object goes out of scope, it closes the underlying socket handle. Socket objects are generally _moveable_ but not _copyable_. A socket object can be transferred from one scope (or thread) to another using `std::move()`.

All code in the library lives within the `sockpp` C++ namespace.

## Unreleased Features (in this branch)

The work in this branch is proceeding to add support for IPv6 and refactor the class hierarchies to better support the different address families without so much redundant code.

 - IPv6 support: _inet6_address_, _tcp6_acceptor_, _tcp_connector_, etc.
 - (Breaking change) The _sock_address_ class is now contains storage for any type of address and follows copy semantics. Previously it was a non-owning reference class. That reference class now exists as _sock_addresss_ref_.
 - Generic base classses are being re-implemented to use _sock_address_ and _sock_address_ref_ as generic addresses.
 
## What's new in v0.3

Version 0.3 brings several new features:

 - Socket class hierarcy now splits out for streaming and datagram sockets.
 - Support for UNIX-domain sockets.
 - New modern CMake build system.
 - GNU Make system marked for deprecation.
 
## Building the Library

CMake is now the supported build system. 

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

 
## TCP Sockets

TCP applications are usually set up as either servers or clients. The `tcp_acceptor` is used to create a TCP server. It binds an address and listens on a known port to accept incoming connections. When a connection is accepted, a new, streaming `tcp_socket` is created. That new socket can be handled directly or moved to a thread (or thread pool) for processing.

Conversely, to create a TCP client, a `tcp_connector` object is created and connected to a server at a known address (host and socket). When connected, the socket is a streaming one which can be used to read and write, directly.

### TCP server: `tcp_acceptor`

The `tcp_acceptor` is used to set up a server and listen for incoming connections.

    int16_t port = 12345;
    sockpp::tcp_acceptor acc(port);

    if (!acc)
        report_error(strerror(acc.last_error()));

    // Accept a new client connection
    sockpp::tcp_socket sock = acc.accept();

The acceptor normally sits in a loop accepting new connections, and passes them off to another process, thread, or thread pool to interact with the client. In standard C++, this could look like:

    while (true) {
        // Accept a new client connection
        sockpp::tcp_socket sock = acc.accept();

        if (!sock) {
            cerr << "Error accepting incoming connection: " 
                << ::strerror(acc.last_error()) << endl;
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
        report_error(strerror(acc.last_error()));

    conn.write_n("Hello", 5);
	
    char buf[5];
    int n = conn.read(buf, 5);

