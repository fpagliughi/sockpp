# TCP and Stream Sockets

## TCP Sockets

TCP and other "streaming" network applications are usually set up as either servers or clients. An acceptor is used to create a TCP/streaming server. It binds an address and listens on a known port to accept incoming connections. When a connection is accepted, a new, streaming socket is created. That new socket can be handled directly or moved to a thread (or thread pool) for processing.

Conversely, to create a TCP client, a connector object is created and connected to a server at a known address (typically host and socket). When connected, the socket is a streaming one which can be used to read and write, directly.

For IPv4 the `tcp_acceptor` and `tcp_connector` classes are used to create servers and clients, respectively. These use the `inet_address` class to specify endpoint addresses composed of a 32-bit host address and a 16-bit port number.

### TCP Server: `tcp_acceptor`

The `tcp_acceptor` is used to set up a server and listen for incoming connections.

```cpp
int16_t port = 12345;
sockpp::tcp_acceptor acc(port);

if (!acc)
    report_error(acc.last_error_str());

// Accept a new client connection
sockpp::tcp_socket sock = acc.accept();
```

The acceptor normally sits in a loop accepting new connections, and passes them off to another process, thread, or thread pool to interact with the client. In standard C++, this could look like:

```cpp
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
```

The hazards of a thread-per-connection design is well documented, but the same technique can be used to pass the socket into a thread pool, if one is available.

See the [tcpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpechosvr.cpp) example.

### TCP Client: `tcp_connector`

The TCP client is somewhat simpler in that a `tcp_connector` object is created and connected, then can be used to read and write data directly.

```cpp
sockpp::tcp_connector conn;
int16_t port = 12345;

if (!conn.connect(sockpp::inet_address("localhost", port)))
    report_error(conn.last_error_str());

conn.write_n("Hello", 5);

char buf[16];
ssize_t n = conn.read(buf, sizeof(buf));
```

See the [tcpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/tcp/tcpecho.cpp) example.

### IPv6

The same style of  connectors and acceptors can be used for TCP connections over IPv6 using the classes:

```cpp
inet6_address
tcp6_connector
tcp6_acceptor
tcp6_socket
udp6_socket
```

Examples are in the [examples/tcp](https://github.com/fpagliughi/sockpp/tree/master/examples/tcp) directory.
