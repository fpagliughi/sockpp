# Building Applications with CMake

The library, when installed can normally be discovered with `find_package(sockpp)`. It uses the namespace `Sockpp` and the library name `sockpp`.

A simple _CMakeLists.txt_ file might look like this:

```
cmake_minimum_required(VERSION 3.15)
project(mysock VERSION 1.0.0)

find_package(sockpp REQUIRED)

add_executable(mysock mysock.cpp)
target_link_libraries(mysock Sockpp::sockpp)
```
