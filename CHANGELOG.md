# Change Log for _sockpp_

## Version 2.0  - Unreleased

### Breaking Changes

- Minimum C++ standard is now **C++17**. C++20 required for `span<byte>` I/O overloads.
- `socket::last_error()` removed; all socket operations now return `result<>` instead of setting a cached error code.
- Exception support removed (`SOCKPP_WITH_EXCEPTIONS` gone). A small number of functions may still throw, but every throwing function now has a `noexcept` overload returning `result<>`.
- CAN bus classes renamed from `can_*` to `canbus_*` (e.g. `can_socket` → `canbus_socket`) to avoid collisions with C struct names.
- CAN `send_to()` / `recv_from()` removed — they are not meaningful for CAN sockets.
- `trust_store` renamed to `trust_locations` in TLS context.
- `sock_address` hierarchy: `remove exception from unix_address` ([#72](https://github.com/fpagliughi/sockpp/issues/72)) and `can_address` ([#72](https://github.com/fpagliughi/sockpp/issues/72)); `system_error` with `error_code` is used instead.
- UNIX-domain and CAN bus headers moved into `unix/` and `canbus/` subdirectories respectively.

### New Features

#### TLS / Secure Sockets (OpenSSL and mbedTLS backends)

- Full TLS client and server support via two compile-time-selectable backends: **OpenSSL** and **mbedTLS**.
- `tls_context` — configures certificates, private keys, trust locations, and TLS parameters.
- `tls_connector` / `tls_acceptor` — TLS-wrapped stream connector and acceptor.
- `tls_socket` — TLS stream socket with graceful shutdown, state queries, and peer certificate retrieval (`peer_certificate()`).
- `tls_certificate` — X.509 certificate wrapper with queries for subject/issuer names, SANs, serial number, fingerprint, validity window, and PEM export.
- `tls_certificate_chain` — ordered collection of `tls_certificate` objects (leaf first), with PEM import/export and structural validity checking.
- TLS connector can be constructed without an immediate connect, allowing TLS options to be configured before calling `connect()`.
- Added ALPN (Application-Layer Protocol Negotiation) support.
- Added PSK (Pre-Shared Key) support.
- Added TLS cipher suite control.
- Added `certinfo` example program to display peer certificate information.
- Added TLS certificate unit tests.

#### C++20 `span<byte>` I/O

- [#87](https://github.com/fpagliughi/sockpp/issues/87) Overloaded `read()`, `write()`, `send()`, `recv()`, etc. with `std::span<byte>` variants for zero-copy, bounds-safe I/O.
- Added unit tests for span I/O functions.

#### CAN FD Support

- Added initial CAN FD (flexible data-rate) support on Linux.
- CAN frames can be converted between classic CAN and CAN FD formats.
- `canbus_socket::recv()` now validates the received frame size and fails if an FD frame arrives on a classic socket.

#### Acceptor / Connector Improvements

- [#88](https://github.com/fpagliughi/sockpp/issues/88) Optional timeout for `acceptor::accept()`.
- [#97](https://github.com/fpagliughi/sockpp/issues/97) Acceptors can choose the socket reuse option (`SO_REUSEADDR` / `SO_REUSEPORT`) in the constructor.
- [#100](https://github.com/fpagliughi/sockpp/issues/100) `set_read_timeout()` / `set_write_timeout()` moved to the base `socket` class so all socket types (UDP, raw, etc.) can use them.
- [#108](https://github.com/fpagliughi/sockpp/issues/108) Protocol value is now customizable in `connector` and `acceptor` constructors.
- `connector::connect()` now closes the socket handle on connection failure.
- Added `connector::connect()` overloads taking host/port strings and returning `result<>`.
- Added cross-platform `poller` class; `connector::connect()` with timeout uses it on non-Windows systems.
- Added new socket constructors.

#### UNIX-Domain Sockets

- Added UNIX-domain socket support on **Windows**.
- UNIX-domain sockets are now a CMake option on all platforms.

#### `result<>` / Error Handling

- `result<>` type significantly improved: generic template base, `std::error_code` for the error variant, `!=` comparison against error types, `release()` member.
- `ioresult` rebuilt on top of the same generic template.
- `socket::clone()` now returns `result<>`.
- `create_handle()` for stream sockets now returns `result<>`.
- `result::last_error()` on Windows manually converts WinSock error codes.
- `error_code` support added for `getaddrinfo()` failures.
- `unique_ptr<>` and `vector<>` brought into the `sockpp` namespace.

### Bug Fixes

- [#106](https://github.com/fpagliughi/sockpp/pull/106) `inet6_address::resolve_name()` now correctly captures `getaddrinfo()`'s error code (previously always reported success).
- [#112](https://github.com/fpagliughi/sockpp/pull/112) Fixed MinGW-64 support; CI updated for MinGW. Minimum Windows target bumped to Windows 7.
- Fixed OpenSSL build issue caused by `::` qualifiers on library macros.
- Fixed copy semantics for mbedTLS certificate objects.
- Resolved compatibility issues between mbedTLS and OpenSSL implementations.

### Build / CI

- CMake minimum required version updated; C++ standard version is now configurable (`SOCKPP_CXX_STANDARD`).
- Added namespace CMake targets for subdirectory builds.
- GitHub Actions CI updated: added macOS builds, Debug and shared-library configurations, and CI for pushes/PRs to the `develop` branch.
- Removed Travis CI configuration.
- Doxygen warnings cleaned up.

---

## [Version 0.8.2](https://github.com/fpagliughi/sockpp/compare/v0.8.1..v0.8.2) - (2023-12-05)

- [#89](https://github.com/fpagliughi/sockpp/issue/89) Fixed generator expression for older CMake
- [#91](https://github.com/fpagliughi/sockpp/issue/91) Fixed uniform_int_distribution<> in UNIX socket example


## [Version 0.8.1](https://github.com/fpagliughi/sockpp/compare/v0.8.0..v0.8.1)

Released: 2023-01-30

- Cherry picked most of the non-TLS commits in PR [#17](https://github.com/fpagliughi/sockpp/pull/17)
    - Connector timeouts
    - Stateless reads & writes for streaming sockets w/ functions returning `ioresult`
    - Some small bug fixes
    - No shutdown on invalid sockets
- [#38](https://github.com/fpagliughi/sockpp/issues/38) Made system libs public for static builds to fix Windows
- [#73](https://github.com/fpagliughi/sockpp/issue/73) Clone a datagram (UDP) socket
- [#74](https://github.com/fpagliughi/sockpp/issue/74) Added `<sys/time.h>` to properly get `timeval` in *nix builds.
- [#56](https://github.com/fpagliughi/sockpp/issue/56) handling unix paths with maximum length (no NUL term)
- Fixed outstanding build warnings on Windows when using MSVC

## [Version 0.8.0](https://github.com/fpagliughi/sockpp/compare/v0.7.1..v0.8.0)

Released: 2023-01-17

- [Breaking] Library initializer now uses a static singleton created via `socket_initializer::initialize()` call, which can be called repeatedly with no ill effect. Also added global `socketpp::initialize()` function as shortcut.
- Improvements to CMake to better follow modern standards.
    - CMake required version bumped up to 3.12
    - Generating CMake files for downstream projects (config, target, version)
    - Windows builds default to shared DLL, not static library
    - Lots of cleanup

## [Version 0.7.1](https://github.com/fpagliughi/sockpp/compare/v0.7..v0.7.1)

Released: 2022-01-24

- [Experimental] **SocketCAN**, CAN bus support on Linux
- [#37](https://github.com/fpagliughi/sockpp/pull/37) socket::get_option() not returning length on Windows
- [#39](https://github.com/fpagliughi/sockpp/pull/39) Using *SSIZE_T* for *ssize_t* in Windows
- [#53](https://github.com/fpagliughi/sockpp/pull/53) Add Conan support
- [#55](https://github.com/fpagliughi/sockpp/pull/55) Fix Android strerror
- [#60](https://github.com/fpagliughi/sockpp/pull/60) Add missing move constructor for connector template.
- Now `acceptor::open()` uses the *SO_REUSEPORT* option instead of *SO_REUSEADDR* on non-Windows systems. Also made reuse optional.

## Version 0.7

- Base `socket` class
    - `shutdown()` added
    - `create()` added
    - `bind()` moved into base socket (from `acceptor`)
- Unix-domain socket pairs (stream and datagram)
- Non-blocking I/O
- Scatter/Gather I/O
- `stream_socket` cloning.
- Set and get socket options using template types.
- `stream_socket::read_n()` and `write_n()` now properly handle EINTR return.
- `to_timeval()` can convert from any `std::chrono::duration` type.
- `socket::close()` and `shutdown()` check for errors, set last error, and return a bool.
- _tcpechomt.cpp_: Example of a client sharing a socket between read and write threads - using `clone()`.
- Windows enhancements:
    - Implemented socket timeouts on Windows
    - Fixed bug in Windows socket cloning.
    - Fixed bug in Windows `socket::last_error_string`.
    - Unit tests working on Windows
- More unit tests

##  Version 0.6

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
 - (Breaking change) The `sock_address` class is now contains storage for any type of address and follows copy semantics. Previously it was a non-owning reference class. That reference class now exists as `sock_address_ref`.
 - Generic base classes are being re-implemented to use _sock_address_ and _sock_address_ref_ as generic addresses.
 - (Breaking change) In the `socket` class(es) the `bool address(address&)` and `bool peer_address(addr&)` forms of getting the socket addresses have been removed in favor of the ones that simply return the address.
 Added `get_option()` and `set_option()` methods to the base `socket`class.
 - The GNU Make build system (Makefile) was deprecated and removed.

## Version 0.3

 - Socket class hierarchy now splits out for streaming and datagram sockets.
 - Support for UNIX-domain sockets.
 - New modern CMake build system.
 - GNU Make system marked for deprecation.

## Version 0.2

 - Initial working version for IPv4.
 - API using boolean return values for pass/fail functions instead of syscall-style integers.
