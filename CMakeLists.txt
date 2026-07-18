# CMakeLists.txt
#
# Top-level CMake build file for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2026 Frank Pagliughi
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ---------------------------------------------------------------------------

# --- CMake required version ---

cmake_minimum_required(VERSION 3.15)

# --- Project setup ---

project(sockpp VERSION "2.0.0")

# --- Build Options ---

option(SOCKPP_BUILD_SHARED "Build shared library" ON)
option(SOCKPP_BUILD_STATIC "Build static library" OFF)
option(SOCKPP_BUILD_EXAMPLES "Build example applications" OFF)
option(SOCKPP_BUILD_TESTS "Build unit tests" OFF)
option(SOCKPP_BUILD_DOCUMENTATION "Create Doxygen reference documentation" OFF)
option(SOCKPP_WITH_OPENSSL "[EXPERIMENTAL] TLS Secure Sockets with OpenSSL" OFF)
option(SOCKPP_WITH_MBEDTLS "[EXPERIMENTAL] TLS Secure Sockets with MbedTLS" OFF)

if(WIN32)
    option(SOCKPP_WITH_UNIX_SOCKETS "[EXPERIMENTAL] Support for UNIX-domain sockets on Win32" OFF)
else()
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        option(SOCKPP_WITH_CAN "[EXPERIMENTAL] Support for Linux SocketCAN" OFF)
    endif()
    option(SOCKPP_WITH_UNIX_SOCKETS "Support for UNIX-domain sockets" ON)
endif()

# ----- Find any dependencies -----

if(SOCKPP_WITH_OPENSSL)
	set(SOCKPP_WITH_TLS ON)
	find_package(OpenSSL REQUIRED)
elseif(SOCKPP_WITH_MBEDTLS)
	set(SOCKPP_WITH_TLS ON)
	find_package(MbedTLS REQUIRED)
endif()

# --- Setting naming variables ---

set(SOCKPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SOCKPP_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# --- Collect the targets names ---

if(${SOCKPP_BUILD_SHARED})
    list(APPEND SOCKPP_TARGETS sockpp-shared)
endif()

if(${SOCKPP_BUILD_STATIC})
    list(APPEND SOCKPP_TARGETS sockpp-static)
endif()

if(NOT SOCKPP_TARGETS)
    message(FATAL_ERROR "No targets are specified")
endif()

# --- C++ standard: Minimum C++17 ---

set(SOCKPP_MIN_CXX_STANDARD 17)

if(DEFINED CMAKE_CXX_STANDARD)
    if(CMAKE_CXX_STANDARD LESS SOCKPP_MIN_CXX_STANDARD)
        message(FATAL_ERROR
            "sockpp requires C++${SOCKPP_MIN_CXX_STANDARD} or later. "
            "CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} is not supported"
        )
    else(DEFINED CMAKE_CXX_STANDARD)
        set(SOCKPP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    endif()
else()
    set(SOCKPP_CXX_STANDARD ${SOCKPP_MIN_CXX_STANDARD})
endif()

unset(SOCKPP_MIN_CXX_STANDARD)

set(CMAKE_CXX_STANDARD ${SOCKPP_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(LIBS_SYSTEM ws2_32)
endif()

# --- Generate a version header ---

configure_file(
	${PROJECT_SOURCE_DIR}/cmake/version.h.in	
	${SOCKPP_GENERATED_DIR}/include/sockpp/version.h
	@ONLY
)

# --- Common library sources, etc ---

add_subdirectory(src)

# --- Install Targets ---
#
# Skipped when sockpp is consumed via add_subdirectory() from a parent
# project - the parent's own install()/package shouldn't be polluted with
# sockpp's headers, libs, and CMake package config files.

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)

include(GNUInstallDirs)

install(TARGETS ${SOCKPP_TARGETS}
	EXPORT Sockpp
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT Sockpp
	FILE
		sockppTargets.cmake
	NAMESPACE
		Sockpp::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${SOCKPP_GENERATED_DIR}/cmake/sockppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
	${PROJECT_SOURCE_DIR}/cmake/sockppConfigTLS.cmake.in
	${SOCKPP_GENERATED_DIR}/cmake/sockppConfigTLS.cmake
	@ONLY
)

install(
    DIRECTORY
        include/
        ${SOCKPP_GENERATED_DIR}/include/
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	FILES
		${PROJECT_SOURCE_DIR}/cmake/sockppConfig.cmake
		${SOCKPP_GENERATED_DIR}/cmake/sockppConfigVersion.cmake
		${SOCKPP_GENERATED_DIR}/cmake/sockppConfigTLS.cmake
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

# --- pkg-config file (for non-CMake consumers) ---

set(SOCKPP_PC_REQUIRES "")
set(SOCKPP_PC_LIBS_EXTRA "")
set(SOCKPP_PC_CFLAGS_EXTRA "")

if(SOCKPP_WITH_OPENSSL)
    # OpenSSL ships its own .pc file almost everywhere.
    set(SOCKPP_PC_REQUIRES "Requires: openssl")
elseif(SOCKPP_WITH_MBEDTLS)
    # MbedTLS doesn't reliably ship a .pc file, so bake in the flags directly.
    get_target_property(_sockpp_mbedtls_incs MbedTLS::mbedtls INTERFACE_INCLUDE_DIRECTORIES)
    if(_sockpp_mbedtls_incs)
        foreach(_sockpp_inc ${_sockpp_mbedtls_incs})
            string(APPEND SOCKPP_PC_CFLAGS_EXTRA " -I${_sockpp_inc}")
        endforeach()
    endif()
    set(SOCKPP_PC_LIBS_EXTRA " -lmbedtls -lmbedx509 -lmbedcrypto")
endif()

if(WIN32)
    string(APPEND SOCKPP_PC_LIBS_EXTRA " -lws2_32")
endif()

configure_file(
	${PROJECT_SOURCE_DIR}/cmake/sockpp.pc.in
	${SOCKPP_GENERATED_DIR}/cmake/sockpp.pc
	@ONLY
)

install(
	FILES
		${SOCKPP_GENERATED_DIR}/cmake/sockpp.pc
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

endif()

# --- Documentation ---

if(SOCKPP_BUILD_DOCUMENTATION)
	add_subdirectory(doc)
endif()

# --- Example applications ---

if(SOCKPP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# --- Unit Tests ---

if(SOCKPP_BUILD_TESTS)
    enable_testing()
	add_subdirectory(tests/unit)
endif()

# --- Packaging (CPack) ---
#
# Skipped when sockpp is consumed via add_subdirectory() from a parent
# project - packaging is a top-level-project concern.

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)

set(CPACK_PACKAGE_NAME "sockpp")
set(CPACK_PACKAGE_VENDOR "Frank Pagliughi")
set(CPACK_PACKAGE_CONTACT "Frank Pagliughi <fpagliughi@mindspring.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++ socket library")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/fpagliughi/sockpp")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

# Plain archives - no extra tooling required, work on every platform.
set(CPACK_GENERATOR "TGZ;ZIP")

# DEB requires dpkg-deb, so only offer it where that's realistically present.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CPACK_GENERATOR "DEB")
endif()

# Auto-compute the Debian "Depends:" line (e.g. libssl3) from the actual
# linked shared libraries, rather than hand-maintaining it here.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)

endif()
