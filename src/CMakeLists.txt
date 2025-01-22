# CMakeLists.txt
#
# Source CMake build file (src/) for the 'sockpp' library.
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

include(GenerateExportHeader)

# The common sources.
# Additional files are added below, as configured.
set(SOURCES
    acceptor.cpp
    connector.cpp
    datagram_socket.cpp
    error.cpp
    inet_address.cpp
    inet6_address.cpp
    socket.cpp
    stream_socket.cpp
)

# Additional sources (platform & config)

if(SOCKPP_WITH_UNIX_SOCKETS)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/unix/unix_address.cpp
    )
endif()

if(SOCKPP_WITH_CAN)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/linux/can_address.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/linux/can_socket.cpp
    )
endif()

if(SOCKPP_WITH_OPENSSL)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/openssl_certificate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/openssl_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/openssl_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/openssl_error.cpp
    )
elseif(SOCKPP_WITH_MBEDTLS)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/mbedtls_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/mbedtls_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tls/mbedtls_error.cpp
    )
endif()

# -- The shared library (if configured) ---

if(${SOCKPP_BUILD_SHARED})
    message(STATUS "Creating shared library")

    add_library(sockpp-shared SHARED ${SOURCES})

    add_library(Sockpp::sockpp-shared ALIAS sockpp-shared)
    add_library(Sockpp::sockpp ALIAS sockpp-shared)

    set_target_properties(sockpp-shared PROPERTIES
        OUTPUT_NAME sockpp
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    generate_export_header(sockpp-shared
        BASE_NAME sockpp
        EXPORT_FILE_NAME ${SOCKPP_GENERATED_DIR}/include/sockpp/export.h
    )
endif()

# -- The static library (if configured) ---

if(${SOCKPP_BUILD_STATIC})
    message(STATUS "Creating static library")

    add_library(sockpp-static STATIC ${SOURCES})

    add_library(Sockpp::sockpp-static ALIAS sockpp-static)

    if(${SOCKPP_BUILD_SHARED})
        target_compile_definitions(sockpp-static PRIVATE SOCKPP_STATIC_DEFINE)
    else()
        add_library(Sockpp::sockpp ALIAS sockpp-static)

        generate_export_header(sockpp-static
            BASE_NAME sockpp
            EXPORT_FILE_NAME ${SOCKPP_GENERATED_DIR}/include/sockpp/export.h
        )
    endif()

    # On *nix systems, the static library can have the same base filename
    # as the shared library, thus 'libsockpp.a' for the static lib.
    # On Windows they need different names to tell the static lib from the
    # DLL import library.
    if(UNIX)
        set_target_properties(sockpp-static PROPERTIES OUTPUT_NAME sockpp)
    endif()
endif()

foreach(TARGET ${SOCKPP_TARGETS})
    target_include_directories(${TARGET} PUBLIC
        $<BUILD_INTERFACE:${SOCKPP_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${SOCKPP_GENERATED_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${TARGET} PUBLIC ${LIBS_SYSTEM})

    # Secure TLS library (if one is configured)

    if(SOCKPP_WITH_OPENSSL)
        target_link_libraries(${TARGET} PUBLIC
            OpenSSL::SSL 
            OpenSSL::Crypto
        )
    elseif(SOCKPP_WITH_MBEDTLS)
        target_link_libraries(${TARGET} PUBLIC 
            MbedTLS::mbedtls
            MbedTLS::mbedcrypto
            MbedTLS::mbedx509
        )
    endif()

    # --- Warnings ---

    target_compile_options(${TARGET} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W3>
      $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -Wdocumentation>
      $<$<NOT:$<OR:$<CXX_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:Clang>>>:-Wall -Wextra -Wpedantic>
    )
endforeach()

