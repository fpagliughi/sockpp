# sockpp

Simple, modern, C++ network socket library.

_sockpp_ is a fairly low-level C++ wrapper around the Berkeley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` class wraps a system socket handle and maintains its lifetime using the familiar [RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) pattern. When the C++ object goes out of scope, it closes the underlying socket handle. Socket C++ objects are generally _moveable_ but not _copyable_. A socket can be transferred from one scope (or thread) to another using `std::move()`.

The library currently supports: IPv4 and IPv6 on Linux, Mac, and Windows. Other *nix and POSIX systems should work with little or no modification.

Unix-Domain Sockets are available on *nix systems that have an OS implementation for them. There's experimental support for them on recent versions of Windows, although only UNIX stream sockets are supported on that platform.

Experimental support for secure sockets using either the OpenSSL or MbedTLS libraries was started with basic coverage. This will continue to be expanded in the near future.

There is also some experimental support for CAN bus programming on Linux using the SocketCAN package. This gives CAN bus adapters a network interface, with limitations dictated by the CAN message protocol.

All code in the library lives within the `sockpp` C++ namespace.

**The 'master' branch is moving toward the v2.0 API, and is particularly unstable at the moment. You're advised to download the latest release for production use.**

## Latest News

Work is proceeding toward Version 2.0 with the following goals:

- Move the library to C++17
- Do a major refactor of error handling, to allow apps to use the library without exceptions.
- Achieve better thread safety.
- Add a portable socket `poller` class
- MinGW-w64 support on Windows
- Fix a number of known bugs and build issues.
- Start some better documentation, particularly for new users.
- Start experimental support for:
  - Secure TLS sockets.
  - SocketCAN (CANbus) on Linux
  - [Optional] Start support for Bluetooth on Linux


The strategy for experimental features is to allow them to be optionally built into the library, but that they are not production-ready and will likely experience breaking changes within the major version as they evolve.

### Get Updates

To keep up with the latest announcements for this project, follow me at:

**Mastodon:** [@fpagliughi@fosstodon.org](https://fosstodon.org/@fpagliughi)

If you're using this library, send me a message and let me know how you're using it.  I'm always curious to see where it winds up!

## Documentation

Documentation is live here in the form of an mdBook:

https://fpagliughi.github.io/sockpp/

It's still in its infancy, but may be helpful.

## Building your app with CMake

The library, when installed can normally be discovered with `find_package(sockpp)`. It uses the namespace `Sockpp` and the library name `sockpp`.

A simple _CMakeLists.txt_ file might look like this:

```
cmake_minimum_required(VERSION 3.15)
project(mysock VERSION 1.0.0)

find_package(sockpp REQUIRED)

add_executable(mysock mysock.cpp)
target_link_libraries(mysock Sockpp::sockpp)
```

## Contributing

Contributions are accepted and appreciated. New and unstable work is done in the `develop` branch Please submit all pull requests against that branch, not _master_.

For more information, refer to: [CONTRIBUTING.md](https://github.com/fpagliughi/sockpp/blob/master/CONTRIBUTING.md)


## Building the Library

CMake is the supported build system.

### Requirements:

- A conforming C++-17 compiler.
    - _gcc_ v8.0 or later (or) _clang_ v5.0 or later.
    - _Visual Studio 2019_, or later on Windows.
- _CMake_ v3.15 or newer.
- _Doxygen_ (optional) to generate API docs.
- _Catch2_ (optional) v2.x or v3.x to build and run unit tests.

To build with default options:

```
$ cd sockpp
$ cmake -Bbuild .
$ cmake --build build/
```

To install:

```
$ cmake --build build/ --target install
```

### Build Options

The library has several build options via CMake to choose between creating a static or shared (dynamic) library - or both. It also allows you to build the example options, and if Doxygen is installed, it can be used to create documentation.

Variable | Default Value | Description
------------ | ------------- | -------------
SOCKPP_BUILD_SHARED | ON | Whether to build the shared library
SOCKPP_BUILD_STATIC | OFF | Whether to build the static library
SOCKPP_BUILD_DOCUMENTATION | OFF | Create and install the HTML based API documentation (requires _Doxygen)_
SOCKPP_BUILD_EXAMPLES | OFF | Build example programs
SOCKPP_BUILD_TESTS | OFF | Build the unit tests (requires _Catch2_)
SOCKPP_WITH_UNIX_SOCKETS | ON (*nix), OFF (Win) | Include support for UNIX-domain sockets. Windows support it experimental.

**Experimental Features**
Variable | Default Value | Description
------------ | ------------- | -------------
SOCKPP_WITH_OPENSSL | OFF | TLS support with OpenSSL
SOCKPP_WITH_MBEDTLS | OFF | TLS support with MbedTLS
SOCKPP_WITH_CAN | OFF | Include SocketCAN support. (Linux only)

Set these using the '-D' switch in the CMake configuration command. For example, to build documentation and example apps:

```
$ cd sockpp
$ cmake -Bbuild -DSOCKPP_BUILD_DOCUMENTATION=ON -DSOCKPP_BUILD_EXAMPLES=ON .
$ cmake --build build/
```

### Error Handling

Version 2.0 substantially changed the way the errors are reported by the library.

The initial versions, up to v1.0, stayed close to the original C-style of error reporting. Most class functions returned an integer value where 0 indicated success, and -1 indicated failure. Behind the scenes, the object grabbed the _errno_ value for the failure and the application could query it by calling `socket::last_error()`. But having a cached error return made it difficult to share the socket across threads, and to reliably tie the error with the exact operation.

The idea of having "stateless" I/O operations which was introduced in [PR #17](https://github.com/fpagliughi/sockpp/pull/17), but the PR was never fully merged. The idea was generalized and arrived in the 2.0 API with a `result<T>` class. The result is generic over the "success" type, but errors are always represented by a C++ `std::error_code`. This fixes the problem of the cached error state in socket objects, but also helps to significantly reduce platform issues for tracking and reporting errors, since the error codes are more portable.

In addition, using a uniform result type removes the need for exceptions in most functions - except for constructors which can't return a result. In those cases where the constructor might throw, a comparable `noexcept` function is also provided which sets an error code parameter instead of throwing. So the library can be used without any exceptions if so desired by the application.

For example, to create a `socket` object, you might see these two similar constructors:
```
socket(int domain, int type);   // This might throw an error
socket(int domain, int type, error_code& ec) noexcept;  // This will never throw. 'ec' gets any error.
```

Now, without the cached error value, the basic socket classes only wrap the socket handle, making them safer to share across threads in the same way a handle can be shared - typically with one thread for reading and another for writing.

## TCP Sockets

TCP and other "streaming" network applications are usually set up as either servers or clients. An acceptor is used to create a TCP/streaming server. It binds an address and listens on a known port to accept incoming connections. When a connection is accepted, a new, streaming socket is created. That new socket can be handled directly or moved to a thread (or thread pool) for processing.

Conversely, to create a TCP client, a connector object is created and connected to a server at a known address (typically host and port). When connected, the socket is a streaming one which can be used to read and write, directly.

For IPv4 the `tcp_acceptor` and `tcp_connector` classes are used to create servers and clients, respectively. These use the `inet_address` class to specify endpoint addresses composed of a 32-bit host address and a 16-bit port number.

### TCP Server: `tcp_acceptor`

The `tcp_acceptor` is used to set up a server and listen for incoming connections.

    in_port_t port = 12345;
    error_code ec;
    sockpp::tcp_acceptor acc{port, ec};

    if (ec)
        report_error(ec.message());

    // Accept a new client connection
    if (auto res = acc.accept(); !res)
        report_error(res.error_message());
    else {
        sockpp::tcp_socket sock = res.release();
        // use sock...
    }

The acceptor normally sits in a loop accepting new connections, and passes them off to another process, thread, or thread pool to interact with the client. In standard C++, this could look like:

    while (true) {
        // Accept a new client connection
        if (auto res = acc.accept(); !res) {
            cerr << "Error accepting incoming connection: "
                << res.error_message() << endl;
        }
        else {
            // Create a thread and transfer the new stream to it.
            thread thr(run_echo, res.release());
            thr.detach();
        }
    }

The hazards of a thread-per-connection design is well documented, but the same technique can be used to pass the socket into a thread pool, if one is available.

See the [tcpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpechosvr.cpp) example.

### TCP Client: `tcp_connector`

The TCP client is somewhat simpler in that a `tcp_connector` object is created and connected, then can be used to read and write data directly.

    in_port_t port = 12345;
    sockpp::tcp_connector conn;

    if (auto res = conn.connect(sockpp::inet_address("localhost", port)); !res)
        report_error(res.error_message());

    conn.write_n("Hello", 5);

    char buf[16];
    if (auto res = conn.read(buf, sizeof(buf)); !res)
        report_error(res.error_message());
    else {
        size_t n = res.value();
        // use buf[0..n-1]...
    }

See the [tcpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpecho.cpp) example.

### UDP Socket: `udp_socket`

UDP sockets can be used for connectionless communications:

    sockpp::udp_socket sock;
    sockpp::inet_address addr("localhost", 12345);

    std::string msg("Hello there!");
    sock.send_to(msg, addr);

    sockpp::inet_address srcAddr;
    char buf[16];
    if (auto res = sock.recv_from(buf, sizeof(buf), &srcAddr); !res)
        report_error(res.error_message());
    else {
        size_t n = res.value();
        // use buf[0..n-1]...
    }

See the [udpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpecho.cpp) and [udpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpechosvr.cpp) examples.

### IPv6

The same style of  connectors and acceptors can be used for TCP connections over IPv6 using the classes:

    inet6_address
    tcp6_connector
    tcp6_acceptor
    tcp6_socket
    udp6_socket

Examples are in the [examples/tcp](https://github.com/fpagliughi/sockpp/tree/master/examples/tcp) directory.

### Unix Domain Sockets

The same is true for local connection on *nix systems that implement Unix Domain Sockets. For that use the classes:

    unix_address
    unix_connector
    unix_acceptor
    unix_socket
    unix_stream_socket
    unix_dgram_socket

Examples are in the [examples/unix](https://github.com/fpagliughi/sockpp/tree/master/examples/unix) directory.

#### UNIX Socket Support in Windows! [Very Experimental]

Later versions of Windows 10 (starting with April 2018 update from Insider Build 17063) and all versions of Windows 11 implement support for UNIX sockets. Initial support was added to this library as a CMake option (opt-in), but has not been thoroughly tested, so is still considered experimental in this library.

More information can be found in these posts from Microsoft:

[AF_UNIX comes to Windows](https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/)
[Windows/WSL Interop with AF_UNIX](https://devblogs.microsoft.com/commandline/windowswsl-interop-with-af_unix/)

Some key points for UNIX sockets on Win32/64:

- Only stream sockets are supported, not dgram.
- socketpair is not supported.
- Windows file and directory permissions determine who can create and connect to UNIX sockets (as expected).
- You can check if UNIX sockets are supported on a target by running the command `sc query afunix` from a Windows admin command prompt.


### Secure Sockets [Experimental]

Support for secure sockets is being added using a number of possible TLS libraries, although support is incomplete and the API is still changing. The idea is that one (and _only_ one) supported TLS library can be built into _sockpp_ at a time.

To build the library with secure socket support, a TLS library needs to be chosen to provide the encryption implementation. Currently _OpenSSL_ v3.x or _MbedTLS_ can be used. When one is chosen, a `tls_context` and `tls_socket` are included that wrap the functionality of the target library.

Choose _one_ of the following when configuring the build:

Variable | Default Value | Description
------------ | ------------- | -------------
SOCKPP_WITH_OPENSSL | OFF | Secure Sockets with OpenSSL
SOCKPP_WITH_MBEDTLS | OFF | Secure Sockets with MbedTLS

#### OpenSSL

The `sockpp` OpenSSL wrapper is currently being built and tested with OpenSSL v3.x.


#### MbedTLS

The `sockpp` library currently supports MbedTLS v3.3. When building that library, the following configuration options should be defined in the config file, _include/mbedtls/mbedtls_config.h_

```
#define MBEDTLS_X509_TRUSTED_CERTIFICATE_CALLBACK
```
To support threading:

```
#define MBEDTLS_THREADING_PTHREAD
#define MBEDTLS_THREADING_C
```

and set the CMake build option:

```
LINK_WITH_PTHREAD:BOOL=ON
```

Note that the options in the config file should already be present in the file but commented out by default. Simply uncomment them, save, and build.

### CANbus on Linux with SocketCAN [Experimental]

The Controller Area Network (CAN bus) is a relatively simple protocol typically used by microcontrollers to communicate inside an automobile or industrial machine over a twisted pair of wires. Linux has the _SocketCAN_ package which allows processes to share access to a physical CAN bus interface using raw sockets in user space. See: [Linux SocketCAN](https://www.kernel.org/doc/html/latest/networking/can.html)

At the lowest level, CAN devices write individual packets, called "frames", to a specific numeric ID (addresses) for each frame. There is no master on the bus; all nodes can read and write at will. In the event that multiple nodes transmit at the same time, the collision is won by the frame with the highest priority, determined as the one with the lowest ID. The other nodes back-off and can retry later.

As an example, consider a device with a temperature sensor. The device might read the temperature periodically and write it to the bus as a raw 32-bit integer, like:

```
canbus_address addr("CAN0");
canbus_socket sock(addr);

// The agreed ID to broadcast temperature on the bus
canid_t canID = 0x40;

while (true) {
    this_thread::sleep_for(1s);

    // Write the temperature to the CAN bus as a 32-bit int
    int32_t t = read_temperature();

    canbus_frame frame { canID, &t, sizeof(t) };
    sock.send(frame);
}
```

A receiver to get a frame might look like this:

```
canbus_address addr("CAN0");
canbus_socket sock(addr);

canbus_frame frame;
sock.recv(&frame);
```

## Implementation Details

The socket class hierarchy is built upon a base `socket` class. Most simple applications will probably not use `socket` directly, but rather use derived classes defined for a specific address family like `tcp_connector` and `tcp_acceptor`.

The socket objects keep a handle to an underlying OS socket handle which is typically an integer file descriptor, with values >=0 for open sockets, and -1 for an unopened or invalid socket, with the exception of Win32 which uses a `HANDLE` object for the sockets. The value used for unopened sockets is defined as a constant, `INVALID_SOCKET`, which is true for all platforms, including Windows. That value doesn't need to be tested directly, as the object itself will evaluate to _false_ if it's uninitialized or in an error state.

The default constructors for each of the socket classes do nothing, and simply set the underlying handle to `INVALID_SOCKET`. They do not create a socket object. The call to actively connect a `connector` object or open an `acceptor` object will create an underlying OS socket and then perform the requested operation, reporting an appropriate error if either the socket could not be created or the operation failed.

An application can generally perform most low-level operations with the library. Unconnected and unbound sockets can be created with the static `create()` function in most of the classes, and then manually bind and listen on those sockets.

The `socket::handle()` method exposes the underlying OS handle which can be sent to any platform API calls that are not exposed by the library.

### Thread Safety

A socket object is not thread-safe. Applications that want to have multiple threads reading from a socket or writing to a socket should use some form of serialization, such as a `std::mutex` to protect access. As of Version 2.0 of the library, nearly all of the socket objects only contain the socket handle in the object, and do not carry any other state than what the OS keeps behind the handle.

It is a common pattern, especially in client applications, to have one thread to read from a socket and another thread to write to the same socket. In this case the underlying socket handle can be considered thread safe (one read thread and one write thread). But a socket object can not be copied as is common in C++ with an assignment or pass-by-value.

The solution for this case is to use the `socket::clone()` method to make a copy of the socket. This will use the system's `dup()` function or similar to create another socket object with a duplicated copy of the socket handle. This has the added benefit that each copy of the socket can maintain an independent lifetime. The underlying socket will not be closed until both objects go out of scope. This work well with the library's RAII philosophy.

    sockpp::tcp_connector conn({host, port});

    if (auto res = conn.clone(); !res) {
        report_error(res.error_message());
    }
    else {
        auto conn_clone = res.release();
        std::thread rdThr(read_thread_func, std::move(conn_clone));
    }

When using a pair of cloned sockets like this, the `socket::shutdown()` method can be used to communicate the intent to close the socket from one of these objects to the other without needing a separate thread signaling mechanism.

As stated, a `socket` can not be copied, but it can be _moved_ from one thread to another safely. This is a common pattern for a server which uses one thread to accept incoming connections and then passes off the new socket to another thread or thread pool for handling. For example with an `acceptor` variable, _acc_, this can be done like:

    if (auto res = acc.accept(); res) {
        // Create a thread and transfer the new socket to it.
        std::thread thr(handle_connection, std::move(res.release()));
        thr.detach();
    }

In this case, _handle_connection_ would be a function that takes a socket by value, like:

    void handle_connection(sockpp::tcp_socket sock) { ... }

Since a `socket` can not be copied, the only choice would be to move the socket to a function like this.


See the [tcpechomt.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpechomt.cpp) example.