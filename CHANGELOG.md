# Change Log


## Version 0.5

- (Breaking change) Updated the hierarchy of network address classes, now derived from a common base class.
    - Removed `sock_address_ref` class. Now a C++ reference to `sock_address` will replace it (i.e. `sock_address&`).
    - `sock_address` is now an abstract base class.
    - All the network address classes now derive from `sock_address`
    - Consolidates a number of overloaded functions that took different forms of addresses to just take a `const sock_address&`
    - Adds a new `sock_address_any` class that can contain any address, and is used by base classes that need a generic address.
- The `acceptor` and `connector` classes are still concrete, generic classes, but now a template derives from each of them to specialize.
- The connector and acceptor classes for each address family (`tcp_connector`, `tcp_acceptor`, `tcp6_connector`, etc) are now typedef'ed to template specializations.
- The `acceptor::bind()` and `acceptor::listen()` methods are now public.
- CMake build now honors the `CMAKE_BUILD_TYPE` flag.

## Version 0.4

The work in this branch is proceeding to add support for IPv6 and refactor the class hierarchies to better support the different address families without so much redundant code.

 - IPv6 support: `inet6_address`, `tcp6_acceptor`, `tcp_connector`, etc.
 - (Breaking change) The `sock_address` class is now contains storage for any type of address and follows copy semantics. Previously it was a non-owning reference class. That reference class now exists as `sock_addresss_ref`.
 - Generic base classses are being re-implemented to use _sock_address_ and _sock_address_ref_ as generic addresses.
 - (Breaking change) In the `socket` class(es) the `bool address(address&)` and `bool peer_address(addr&)` forms of getting the socket addresses have been removed in favor of the ones that simply return the address.
 Added `get_option()` and `set_option()` methods to the base `socket`class.
 - The GNU Make build system (Makefile) was deprecated and removed.

## Version 0.3

 - Socket class hierarcy now splits out for streaming and datagram sockets.
 - Support for UNIX-domain sockets.
 - New modern CMake build system.
 - GNU Make system marked for deprecation.

## Version 0.2

 - Initial working version for IPv4.
 - API using boolean return values for pass/fail functions instead of syscall-style integers.