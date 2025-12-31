# Network Addresses

The _sockpp_ library provides wrapper classes for the socket address structures for the supported address families.

The address classes typically wrap the underlying C struct for the family that they represent, and simply provide constructors and field defaults to make for easier and more convenient usage. The address objects act as normal data, implementing copy and move semantics. For simplicity, all multi-byte integer values given or retrieved from the address objects are in native (host) byte order. The classes convert to and from network order as needed.

Note that the C++ classes are _specifically_ given similar, but different, names than their C counterparts to avoid confusion and name clashes, despite being in different namespaces.

## The C API

Before explaining the _sockpp_ C++ wrappers for the C API address structures, it would be good to review the C API for network addressing.

Many API functions that expect an address take a pointer and size to a generic, protocol-independent, structure called `sockaddr`, like this:

```c
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
```

Here, essentialy, the `bind()` call takes a socket descriptor and an address. The address is provided as a pointer to a struct and the length of the struct, noting that the addresses from different families can be of different length.

The `sockaddr` struct is usually defined something like this:

```c
struct sockaddr {
    sa_family_t sa_family;   // Address family (e.g., AF_INET, AF_INET6, AF_UNIX)
    char        sa_data[14]; // Protocol address data (variable length)
};
```

The contains a single, concrete member, `sa_family` as a short (2-byte) integer, providing the addess family, which defines the actual type that will follow. All addresses simply need this as the first field in their family-specific address structures to properly align with `sockaddr`.

Addresses for specific protocols have concrete structures defined in the API which overlap with this. For example the IPv4 family address structure often looks like this:

```c
struct sockaddr_in {
    sa_family_t      sin_family;  // Address family is always AF_INET
    unsigned short   sin_port;    // Port number (network byte order)
    struct in_addr   sin_addr;    // IP address (network byte order)
    char             sin_zero[8]; // Padding to match the size of struct sockaddr
};
```

Note that the size of `sockaddr` is 16 bytes, and as `sockaddr_in` requires less data, it pads the struct to be the same length. But many address structs that were defined later require more than 16 bytes, and thus the length is always required as they can be different sizes.

Since the actual addresses can be larger than `sockaddr` a new generic type was defined to have enough storage space to contain any address supported on the target platform. This is called `sockaddr_storage`:

```c
struct sockaddr_storage {
    sa_family_t ss_family;      // Address family (e.g., AF_INET, AF_INET6)
    char padding[SS_PADSIZE];   // Padding to contain any address type
};
```

This can be used to read in an address when the specific type is not knowd. After it is received, the `ss_family` member can be checked to see the type of address and determine the actual length.

## The Base Address Class: *sock_address*

In the _sockpp_ library, the `sock_address` is an abstract base class for all the addresses. It serves a similar use as the `sockaddr` in the C API, serving as a function parameter when the specific address can be more than one different type. It contains virtual methods to let the derived classes expose their family, and a pointer and length to an underlying C struct, allowing the C++ addresses to be passed to any of the C API functions, as needed.

It looks something like this:

```cpp
class sock_address
{
public:
    virtual socklen_t size() const = 0;
    virtual sockaddr* sockaddr_ptr() = 0;
    virtual const sockaddr* sockaddr_ptr() const = 0;
    virtual sa_family_t family() const { ... }
    // ...
};
```

Since the other addresses are derived from this, they can se sent to any function that takes a reference to a `sock_address`, like this:

```cpp
result<size_t> socket::send_to(const string& s, const sock_address& addr);
```

## IPv4 and IPv6 Addresses: *inet_address* and *inet6_address*

The `inet_address` is the address type for the **AF_INET** family, and wraps the C `sockaddr_in` struct. It is the IPv4 address type, consisting of a 32-bit address and 16-bit port number. All integer values use native (host) byte ordering going into or out of the objects.

Addresses can be created using the numeric values, like:

```cpp
inet_address addr{0x7F000001, 80};
```

They can also be created with string address names, in which case, name resolution will be applied to get the actual address. When done this way, though, the resolution can fail, and the error should be handled. The application can use the presentation form of an address, or the name for lookup:

```cpp
inet_address addr1{"192.168.1.1", 80};
inet_address addr2{"localhost", 502};
```

The `inet6_address` is the address type for the **AF_INET6** family, and wraps the C `sockaddr_in6` struct. It is the IPv6 address type, consisting of a 128-bit address and 16-bit port number.

## Additional Address Families

The _sockpp_ library has additional address types for other supported families, like `unix_address` for UNIX-domain sockets, `can_address` for Linux SocketCAN, etc. These are described in the sections about those protocols.

## The "Any" Address Type: *sock_address_any*

The library also provides the generic `sock_address_any` type, which contains enough storage space for any of the supported addresses on the platform. This wraps the `sockaddr_storage` type from the C API. Applications can use an address of this type when receiving addresses of an unknown family.

This is also the address type returned by the underlying (generic) `socket` class when queried for it's address or the address of its peer:

```cpp
class socket {
public:
    // ...

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

    // ...
};
```

Since it overrides the base address class, `sock_address_any` can be used whenever an address is required; it does not have to be converted to the specific type to be used. But it can be converted. The different family address types all have constructors that take a `sock_address_any`, but they are all failable. They only succeed if the address is compatible, which is usually confirmed by checking the family type and length of the source address.

```cpp
sock_address_any any_addr = sock.peer_address();

error_code ec;
inet_address addr{any_addr, ec};
if (!ec) {
    cout << "The peer is not using IPv4" << endl;
}
```


