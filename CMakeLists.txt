# CMakeLists.txt
#
# Top-level CMake build file for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2024 Frank Pagliughi
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

cmake_minimum_required(VERSION 3.12)

# --- Project setup ---

project(sockpp VERSION "2.0.0")

# --- Build Options ---

option(SOCKPP_BUILD_SHARED "Build shared library" ON)
option(SOCKPP_BUILD_STATIC "Build static library" OFF)
option(SOCKPP_BUILD_EXAMPLES "Build example applications" OFF)
option(SOCKPP_BUILD_TESTS "Build unit tests" OFF)
option(SOCKPP_BUILD_DOCUMENTATION "Create Doxygen reference documentation" OFF)
option(SOCKPP_WITH_OPENSSL "TLS Secure Sockets with OpenSSL" OFF)
option(SOCKPP_WITH_MBEDTLS "TLS Secure Sockets with Mbed TLS" OFF)

if(NOT WIN32)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        option(SOCKPP_WITH_CAN "Include support for Linux SocketCAN components" OFF)
    endif()
    option(SOCKPP_WITH_UNIX_SOCKETS "Include support for UNIX-domain sockets" ON)
else()
    option(SOCKPP_WITH_UNIX_SOCKETS "Include support for UNIX-domain sockets" OFF)
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

# --- Project uses C++17 ---

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    COMPATIBILITY AnyNewerVersion
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
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

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

