# CMakeLists.txt
#
# CMake file for the TCP v4 socket example applications
# in the 'sockpp' library.
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
# --------------------------------------------------------------------------

# --- For apps that use threads ---

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- Executables ---

# IPv4

add_executable(tcpechosvr tcpechosvr.cpp)
add_executable(tcpecho tcpecho.cpp)
add_executable(tcpechotest tcpechotest.cpp)
add_executable(tcpechomt tcpechomt.cpp)

# IPv6

add_executable(tcp6echosvr tcp6echosvr.cpp)
add_executable(tcp6echo tcp6echo.cpp)

# --- Link for executables ---

message(STATUS "Using library for samples: ${SOCKPP_LIB}")

target_link_libraries(tcpechosvr ${SOCKPP_LIB} Threads::Threads)
target_link_libraries(tcpecho ${SOCKPP_LIB})
target_link_libraries(tcpechotest ${SOCKPP_LIB})
target_link_libraries(tcpechomt ${SOCKPP_LIB} Threads::Threads)

target_link_libraries(tcp6echosvr ${SOCKPP_LIB} Threads::Threads)
target_link_libraries(tcp6echo ${SOCKPP_LIB})


# --- Install ---

set(INSTALL_TARGETS
	tcpechosvr
	tcpecho
	tcpechotest
	tcpechomt
	tcp6echosvr
	tcp6echo
)


install(TARGETS ${INSTALL_TARGETS}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)

