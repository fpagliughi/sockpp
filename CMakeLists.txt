# CMakeLists.txt
#
# Top-level CMake build file for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2018 Frank Pagliughi
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
cmake_minimum_required(VERSION 3.5)

## project
project(sockpp VERSION "0.7.0")

## library name
set(SOCKPP sockpp)
set(SOCKPP_STATIC ${SOCKPP}-static)

# --- Build Options ---

if(WIN32)
	option(SOCKPP_BUILD_STATIC "Build static library" ON)
	option(SOCKPP_BUILD_SHARED "Build shared library (DLL)" OFF)
else()
	option(SOCKPP_BUILD_STATIC "Build static library" OFF)
	option(SOCKPP_BUILD_SHARED "Build shared library" ON)
endif()

option(SOCKPP_BUILD_EXAMPLES "Build example applications" OFF)
option(SOCKPP_BUILD_TESTS "Build unit tests" OFF)
option(SOCKPP_BUILD_DOCUMENTATION "Create Doxygen reference documentation" OFF)

# --- C++14 build flags ---

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Generate a version header ---

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/version.h.in  
	generated/${SOCKPP}/version.h
	@ONLY
)

# --- Common Library Sources, etc ---

add_subdirectory(src)

# --- Header Locations ---

target_include_directories(sockpp-objs 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE 
		${CMAKE_CURRENT_BINARY_DIR}/generated
		src
)

# --- System libraries ---

include(GNUInstallDirs)

if(WIN32)
	set(LIBS_SYSTEM ws2_32)
elseif(UNIX)
	set(LIBS_SYSTEM c stdc++)
endif()


## --- create the library ---

set(SOCKPP_LIB ${SOCKPP})

if((NOT SOCKPP_BUILD_STATIC) AND (NOT SOCKPP_BUILD_SHARED))
	message(FATAL_ERROR "Please choose whether to build static or shared library.")
else()
	message(STATUS "Creating library: ${SOCKPP}")
	if(SOCKPP_BUILD_STATIC)
		list(APPEND LIB_TYPE STATIC)
	endif()
		
	if(SOCKPP_BUILD_SHARED)
		list(APPEND LIB_TYPE SHARED)
	endif()
		
	add_library(${SOCKPP} ${LIB_TYPE} $<TARGET_OBJECTS:sockpp-objs>)
	target_link_libraries(${SOCKPP} ${LIBS_SYSTEM})
endif()

# --- Documentation ---

if(SOCKPP_BUILD_DOCUMENTATION)
	add_subdirectory(doc)
endif()

# --- Examples Applications ---

if(SOCKPP_BUILD_EXAMPLES)
	add_subdirectory(examples/tcp)
	add_subdirectory(examples/udp)
	if(UNIX)
		add_subdirectory(examples/unix)
	endif()
endif()

# --- Unit Tests ---

if(SOCKPP_BUILD_TESTS)
	add_subdirectory(tests/unit)
endif()


# --- Exporting ---

install(TARGETS
        ${SOCKPP}
    EXPORT sockppConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
export(TARGETS
        ${SOCKPP}
    NAMESPACE sockpp::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/sockppConfig.cmake"
)
install(EXPORT
        sockppConfig
    DESTINATION "${CMAKE_INSTALL_DATADIR}/sockpp/cmake"
    NAMESPACE sockpp::
)