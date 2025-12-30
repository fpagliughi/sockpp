# Socket Basics

The `socket` class forms the base of the class hierarchy for the socket types in the library. It is an RAII wrapper around an integer socket handle, with numerous methods for configuring and using the socket at a base level. An application would typically not use a base `socket` variable directly, but would create one from a derived class that is specific for clients or servers, or something specific to an address family.

## Socket Handle Lifetime

The `socket` object maintains ownership of a socket handle from the operating system. This is typically an `int` on *nix systems, and a Winsock `HANDLE` on Windows. The object maintains the lifetime of the handle and automatically closes it in from the destructor when it goes out of scope.

A socket object does not always contain a valid OS handle. The object can be constructed without a handle, then assigned one later. Similarly, a valid handle can be released from the socket, whereby the object reverts to to initial state without a handle. When a socket object does not contain a handle, it is considered to be in an _invalid_ state. This can be tested in a number of ways, typically with the boolean operators:

```
sockpp::socket sock;

if (!sock) {
    cout << "The socket does not contain an OS handle." << endl;
}
```

### Move Semantics

`socket` objects do not implement copy semantics by default, but _do_ implement move semantics. Therefore, unless the user does something unsafe with the underlying handle, no two socket objects should contain the same handle. The object has unique ownership.

So, to pass ownership of the socket to another context, such as into a function or to another thread, _move_ the socket to the other context using `std::move()`:

```
sockpp::socket sock = get_a_socket_from_somewhere();
send_the_socket_somewhere_else(std::move(sock));
```

### The OS Socket Handle

In the library, the OS socket handle is given the platform-agnostic type of `socket_t`. As previously stated, this is typically an `int` on *nix systems, and a Winsock `HANDLE` on Windows. Either way, the `socket_t` type can be used for portability if the application needs to access the value.

The _sockpp_ API does not fully cover the C socket API (or the Winsock API on Windows). An application is still able to call the C functions by using the object's `handle()`.

```
#include "sockpp/socket.h"
#include <sys/stat.h>

int main() {
    struct stat sb;

    auto sock = sockpp::socket::create(AF_INET, SOCK_STREAM);

    if (fstat(sock.handle(), &sb) == 0) {
        cout << "I-node number: " << sb.st_ino << endl;
    }
}
```