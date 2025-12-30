# Introduction

_sockpp_ is a simple, modern, C++ network socket library.

It is a fairly low-level C++ wrapper around the Berkeley sockets library using `socket`, `acceptor,` and `connector` classes that are familiar concepts from other languages.

The base `socket` class wraps a system socket handle and maintains its lifetime using the familiar [RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) pattern. When the C++ object goes out of scope, it closes the underlying socket handle. Socket objects are generally _movable_ but not _copyable_. A socket can be transferred from one scope (or thread) to another using `std::move()`.

## Features

The library currently supports:

- Portable socket classes across Linux, Mac, and Windows.
    - Other *nix and POSIX systems are untested, but should work with little or no modification.
- IPv4 and IPv6 on all supported platforms.
- Unix-Domain Sockets on *nix systems that have an OS implementation for them.
    - Windows support might be possible, if there was any interest.
- `connector` classes for easy client creation.
- `acceptor` classes for easy server creation.
- `address` classes for easy network address manipulation.
- [Experimental] CANbus raw sockets on Linux for SocketCAN.
- [Experimental, Coming Soon] Secure TLS sockets with OpenSSL and MbedTLS

Support for secure sockets using either the OpenSSL or MbedTLS libraries was recently started with basic coverage. This will continue to be expanded in the near future.

There is also some experimental support for CAN bus programming on Linux using the SocketCAN subsystem. SocketCAN creates virtual network interfaces to read and write packets onto a physical CAN  bus using raw sockets.

Note that _sockpp_ releases observe semantic versioning, but any features marked as experimental are being actively developed, and are not bound by those constraints. Experimental features may have breaking changes within the same major version.

## C++ Language Details

_sockpp_ v2.x targets C++17, and makes use of many of the language features from this version, so a compatible compiler is required. Recent versions of _gcc_, _clang_, and MSVC have all been tested and should suffice.

The library provides C++ classes that wrap the Berkeley Socket C API, creating different socket types for the different and protocol amd address families. This makes for easy socket creation depending on the required uses.

All code in the library lives within the `sockpp` C++ namespace. A number of common collection types from the standard library are imported from the `std` namespace into the `sockpp` namespace. Thus, for example, the `string` type in _sockpp_ is the `std::string` type. Thus a `std::string` _is a_ `sockpp::string`.

The _sockpp_ API generally avoids throwing exceptions, prefering instead to return a generic result type from functions. This is a type of union or variant that can contain the generic return type on success, or an error type on failure. The error is based on a C++ `error_code`. For functions that can not return a value, such as constructors, there are typically two matching functions: one with the required parameters, and another with the same parameters and an additional parameter which is a reference to an `error_code`. The first will throw an exception on error. The second is marked as `noexcept` and sets the error code on failure.


