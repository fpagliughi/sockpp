# CMakeLists.txt
#
# Top-level CMake build file for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017 Frank Pagliughi
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

## CMake required version
cmake_minimum_required(VERSION 3.2)

## project
project(sockpp)

## library name
set(SOCKPP sockpp)
set(SOCKPP_STATIC ${SOCKPP}-static)

set(CMAKE_BUILD_TYPE Release)

# --- Project/build settings ---

set(SOCKPP_VERSION_MAJOR 0)
set(SOCKPP_VERSION_MINOR 9)
set(SOCKPP_VERSION_PATCH 0)

set(CLIENT_VERSION ${SOCKPP_VERSION_MAJOR}.${SOCKPP_VERSION_MINOR}.${SOCKPP_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_MAJOR ${SOCKPP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${SOCKPP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${SOCKPP_VERSION_PATCH})

# --- build options ---

if(WIN32)
  option(SOCKPP_BUILD_STATIC "Build static library" ON)
  option(SOCKPP_BUILD_SHARED "Build shared library (DLL)" OFF)
else()
  option(SOCKPP_BUILD_STATIC "Build static library" OFF)
  option(SOCKPP_BUILD_SHARED "Build shared library" ON)
endif()

option(SOCKPP_BUILD_SAMPLES "Build sample programs" OFF)
option(SOCKPP_BUILD_DOCUMENTATION "Create Doxygen reference documentation" OFF)

# --- C++14 build flags ---

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Library sources ---

# Compile all the C++ files in the src/ dir into the library
file(GLOB SOURCES "src/*.cpp")

if(WIN32)
    list( REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/unix_address.cpp )
endif()

# Uncomment for some CMake debug info
#message(STATUS "SOURCES: " ${SOURCES})

# --- System libraries ---

include(GNUInstallDirs)

if(WIN32)
    set(LIBS_SYSTEM ws2_32)
elseif(UNIX)
    set(LIBS_SYSTEM c stdc++)
endif()

## --- create the shared library ---

if(SOCKPP_BUILD_SHARED)
    add_library(${SOCKPP} SHARED ${SOURCES})

    set_target_properties(${SOCKPP} PROPERTIES
        VERSION ${CLIENT_VERSION}
        SOVERSION ${SOCKPP_VERSION_MAJOR})

    install(TARGETS ${SOCKPP}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# --- Create the static library ---

if(SOCKPP_BUILD_STATIC)
    add_library(${SOCKPP_STATIC} STATIC ${SOURCES})

    install(TARGETS ${SOCKPP}-static
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

		# Let the archive use the same name as the shared library
		if(UNIX)
        set_target_properties(${SOCKPP_STATIC} PROPERTIES OUTPUT_NAME ${SOCKPP})
		endif()
endif()

# ----- Specify the header location -----

target_include_directories(${SOCKPP}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Install the library ---

if(SOCKPP_BUILD_SHARED)
    set(SOCKPP_LIB ${SOCKPP})
else()
    set(SOCKPP_LIB ${SOCKPP_STATIC})
endif()

install(DIRECTORY src/${SOCKPP}/
    DESTINATION include/${SOCKPP}
    FILES_MATCHING PATTERN "*.h*")


if(SOCKPP_BUILD_SAMPLES)
    add_subdirectory(samples/tcp)
    add_subdirectory(samples/unix)
endif()

