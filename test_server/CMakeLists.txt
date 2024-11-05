#Proyecto_2017
cmake_minimum_required (VERSION 3.0)
project (gptbon)
set(CMAKE_PROJECT_VERSION 1.0)

## LIBRARY ##
add_library(${PROJECT_NAME} STATIC)

## prometheus sources
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/prometheus-cpp/core/include" )
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/prometheus-cpp/pull/include" )
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/prometheus-cpp/push/include" )
INCLUDE_DIRECTORIES( "${CMAKE_SOURCE_DIR}/prometheus-cpp/build/core/include/prometheus/detail" )
LINK_DIRECTORIES("${CMAKE_SOURCE_DIR}/prometheus-cpp/build/lib")

target_sources(${PROJECT_NAME}
        PRIVATE
        ../lib/src/cl_input.cpp
        ../lib/src/common.cpp
        ../lib/src/dir_path.cpp
        ../lib/src/network_data.cpp
        ../lib/src/Packed_sample.cpp
        ../lib/src/server_data.cpp
        ../lib/src/servidor_monitor.cpp
        ../lib/src/system_features.cpp
        ../lib/src/net_info.cpp
        )

target_include_directories(${PROJECT_NAME}
        #PRIVATE
        # where the library itself will look for its internal headers
        #${CMAKE_CURRENT_SOURCE_DIR}/priv-include
        PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../lib/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/../lib>
        )

# note that it is not CMAKE_INSTALL_PREFIX we are checking here
if(DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(
            STATUS
            "CMAKE_INSTALL_PREFIX is not set\n"
            "Default value: ${CMAKE_INSTALL_PREFIX}\n"
            "Will set it to ${CMAKE_SOURCE_DIR}/install"
    )
    set(CMAKE_INSTALL_PREFIX
            "${CMAKE_SOURCE_DIR}/install"
            CACHE PATH "Where the library will be installed to" FORCE
            )
else()
    message(
            STATUS
            "CMAKE_INSTALL_PREFIX was already set\n"
            "Current value: ${CMAKE_INSTALL_PREFIX}"
    )
endif()

# without it public headers won't get installed
set(public_headers
        ../lib/include/cl_input.hpp
        ../lib/include/cliente_monitor.hpp
        ../lib/include/common.hpp
        ../lib/include/cpu_info.hpp
        ../lib/include/daemon_global_params.hpp
        ../lib/include/devices_info.hpp
        ../lib/include/dir_path.hpp
        ../lib/include/library.hpp
        ../lib/include/memory_info.hpp
        ../lib/include/network_data.hpp
        ../lib/include/net_info.hpp
        ../lib/include/network_data.hpp
        ../lib/include/Packed_sample.hpp
        ../lib/include/power_cpu_info.hpp
        ../lib/include/server_data.hpp
        ../lib/include/servidor_monitor.hpp
        ../lib/include/socket_params.h
        ../lib/include/system_features.hpp
        ../lib/include/temp_info.hpp
        )
# note that ${public_headers} has to be in quotes
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")

target_link_libraries(${PROJECT_NAME} PUBLIC
        -lprometheus-cpp-core
        -lprometheus-cpp-pull
        -pthread
        )

## Executable
find_package (Threads)

SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_C++_COMPILER g++)
list(APPEND CMAKE_CXX_FLAGS "-std=c++14 -D_GNU_SOURCE -g -ldl -lprometheus-cpp-core -lprometheus-cpp-pull")

message(${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE -g")

add_executable(LDS
        test_server.cpp
        )
target_link_libraries(LDS PUBLIC
        gptbon
        )
