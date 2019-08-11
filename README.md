# sockpp

[![Build Status](https://travis-ci.org/fpagliughi/sockpp.svg?branch=master)](https://travis-ci.org/fpagliughi/sockpp)

Simple, modern, C++ socket library.

This is a fairly low-level C++ wrapper around the Berkeley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` wraps a system socket handle, and maintains its lifetime. When the C++ object goes out of scope, it closes the underlying socket handle. Socket objects are generally _moveable_ but not _copyable_. A socket object can be transferred from one scope (or thread) to another using `std::move()`.

All code in the library lives within the `sockpp` C++ namespace.

## Latest News

The library is reaching a stable API, and is on track for a 1.0 release in the near future. Until then, there may be a few more breaking changes, but hopefully those will be fewer than we have seen so far.

To keep up with the latest announcements for this project, follow me at:

**Twitter:** [@fmpagliughi](https://twitter.com/fmpagliughi)

## New in v0.6

- UDP support
    - The base `datagram_socket` added to the Windows build
    - The `datagram_socket` cleaned up for proper parameter and return types.
    - New `datagram_socket_tmpl` template class for defining UDP sockets for the different address families.
    - New datagram classes for IPv4 (`udp_socket`), IPv6 (`udp6_socket`), and Unix-domain (`unix_dgram_socket`)
- Windows support
    - Windows support was broken in release v0.5. It is now fixed, and includes the UDP features.
- Proper move semantics for stream sockets and connectors.
- Separate tcp socket header files for each address family (`tcp_socket.h`, `tcp6_socket.h`, etc).
- Proper implementation of Unix-domain streaming socket.
- CMake auto-generates a version header file, _version.h_
- CI dropped tests for gcc-4.9, and added support for clang-7 and 8.

## TODO

- **Unit Tests** - The framework for unit and regression tests is in place (using _Catch2_), along with the GitHub Travis CI integration. But the library could use a lot more tests.
- **Consolidate Header Files** - The last round of refactoring left a large number of header files with a single line of code in each. This may be OK, in that it separates all the protocols and families, but seems a waste of space.
- **Secure Sockets** - It would be extremely handy to have support for SSL/TLS built right into the library as an optional feature.
- **SCTP** - The _SCTP_ protocol never caught on, but it seems intriguing, and might be nice to have in the library for experimentation, if not for some internal applications.

## Building the Library

CMake is the supported build system.

### Requirements:

- A conforming C++-14 compiler.
    - _gcc_ v5.0 or later (or) _clang_ v3.8 or later.
    - _Visual Studio 2015_, or later on WIndows.
- _CMake_ v3.5 or newer.
- _Doxygen_ (optional) to generate API docs.
- _Catch2_ (optional) to build and run unit tests.

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
SOCKPP_BUILD_DOCUMENTATION | OFF | Create and install the HTML based API documentation (requires _Doxygen)_
SOCKPP_BUILD_EXAMPLES | OFF | Build example programs
SOCKPP_BUILD_TESTS | OFF | Build the unit tests (requires _Catch2_)


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

    char buf[16];
    ssize_t n = conn.read(buf, sizeof(buf));

### UDP Socket: `udp_socket`

UDP sockets can be used for connectionless communications:

    sockpp::udp_socket sock;
    sockpp::inet_address addr("localhost", 12345);

    std::string msg("Hello there!");
    sock.send_to(msg, addr);

    sockpp::inet_address srcAddr;

    char buf[16];
    ssize_t n = sock.recv(buf, sizeof(buf), &srcAddr);


### IPv6

The same style of  connectors and acceptors can be used for TCP connections over IPv6 using the classes:

    inet6_address
    tcp6_connector
    tcp6_acceptor
    tcp6_socket
    udp6_socket

### Unix Domain Sockets

The same is true for local connection on *nix systems that implement Unix Domain Sockets. For that use the classes:

    unix_address
    unix_connector
    unix_acceptor
    unix_socket  (unix_stream_socket)
    unix_dgram_socket
