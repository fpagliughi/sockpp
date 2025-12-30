# UDP and Datagram Sockets

### UDP Socket: `udp_socket`

UDP sockets can be used for connectionless communications:

```
sockpp::udp_socket sock;
sockpp::inet_address addr("localhost", 12345);

std::string msg("Hello there!");
sock.send_to(msg, addr);

sockpp::inet_address srcAddr;

char buf[16];
ssize_t n = sock.recv(buf, sizeof(buf), &srcAddr);
```

See the [udpecho.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpecho.cpp) and [udpechosvr.cpp](https://github.com/fpagliughi/sockpp/blob/master/examples/udp/udpechosvr.cpp) examples.

