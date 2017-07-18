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
  set(SOCKPP_BUILD_STATIC TRUE  CACHE BOOL "Build static library")
  set(SOCKPP_BUILD_SHARED FALSE CACHE BOOL "Build shared library (DLL)")
else()
  set(SOCKPP_BUILD_STATIC FALSE  CACHE BOOL "Build static library")
  set(SOCKPP_BUILD_SHARED TRUE CACHE BOOL "Build shared library")
endif()

set(SOCKPP_BUILD_SAMPLES FALSE CACHE BOOL "Build sample programs")
set(SOCKPP_BUILD_DOCUMENTATION FALSE CACHE BOOL "Create Doxygen reference documentation")

# --- C++11 build flags ---

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Library sources ---

#Bring the headers into the project
include_directories(src)

# Compile all the C++ files in the src/ dir into the library
file(GLOB SOURCES "src/*.cpp")

if(WIN32)
    list( REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/unix_address.cpp )
endif()

#message(STATUS "SOURCES: " ${SOURCES})

add_library(OBJS OBJECT ${SOURCES})

# --- System libraries ---

if(WIN32)
    set(LIBS_SYSTEM ws2_32)
elseif(UNIX)
    set(LIBS_SYSTEM c stdc++ pthread)
endif()


## --- create the shared library ---

if(SOCKPP_BUILD_SHARED)
    ## set position independent flag (-fPIC on Linux)
    set_property(TARGET OBJS PROPERTY POSITION_INDEPENDENT_CODE ON)

    add_library(${SOCKPP} SHARED $<TARGET_OBJECTS:OBJS>)

    ## add dependencies to the shared library
    target_link_libraries(${SOCKPP} ${LIBS_SYSTEM})

    ## set the shared library soname
    set_target_properties(${SOCKPP} PROPERTIES
        VERSION ${CLIENT_VERSION}
        SOVERSION ${SOCKPP_VERSION_MAJOR})

    install(TARGETS ${SOCKPP}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
endif()

# --- Create the static library ---

if(SOCKPP_BUILD_STATIC)
    add_library(${SOCKPP_STATIC} STATIC $<TARGET_OBJECTS:OBJS>)

    ## add dependencies to the static library
    target_link_libraries(${SOCKPP_STATIC} ${LIBS_SYSTEM})

    install(TARGETS ${SOCKPP}-static
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

    set_target_properties(${SOCKPP_STATIC} PROPERTIES OUTPUT_NAME ${SOCKPP})
endif()

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

