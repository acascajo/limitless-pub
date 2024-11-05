#Proyecto_2017
cmake_minimum_required (VERSION 2.8)
project (gptbon_client)
set(CMAKE_PROJECT_VERSION 1.0)

set(tool_dest "bin")
set(include_dest "include/")
set(main_lib_dest "lib/")

SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_C++_COMPILER g++)
list(APPEND CMAKE_CXX_FLAGS "-std=c++2a -D_GNU_SOURCE -g -ldl")
message(${CMAKE_SOURCE_DIR})
list(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE -g")

## LIBRARY ##
add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/cliente_monitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/cpu_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/devices_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/dir_path.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/memory_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/net_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/Packed_sample.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/power_cpu_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/sample_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/server_data.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/servidor_monitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/system_features.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/src/temp_info.cpp
        )

target_include_directories(${PROJECT_NAME}
        #PRIVATE
        # where the library itself will look for its internal headers
        #${CMAKE_CURRENT_SOURCE_DIR}/priv-include
        PUBLIC
        # where top-level project will look for the library's public headers
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include>
	#$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lib_src>
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

# without its public headers won't get installed
set(public_headers
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/cliente_monitor.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/common.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/cpu_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/daemon_global_params.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/devices_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/dir_path.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/memory_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/net_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/network_data.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/Packed_sample.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/power_cpu_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/server_data.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/servidor_monitor.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/socket_params.h
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/system_features.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_src/include/temp_info.hpp
        )

# note that ${public_headers} has to be in quotes
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")


## Threads sources
#find_package (Threads)

## Redis sources
find_library(HIREDIS_LIBRARY hiredis)
# Check if hiredis library is found
if(NOT HIREDIS_LIBRARY)
    message(STATUS "hiredis library not found")
else()
    message(STATUS "hiredis library found")
endif()

INCLUDE_DIRECTORIES("include/")

# Original vs testing
add_executable(proc-sender src/daemon_monitor.cpp)
#add_executable(proc-sender src/test_daemon.cpp)

target_link_libraries(proc-sender PUBLIC
        gptbon_client)

target_link_libraries(proc-sender PUBLIC ${HIREDIS_LIBRARY})

# Install
install(TARGETS gptbon_client DESTINATION "${main_lib_dest}")
install(FILES ${public_headers} DESTINATION "${include_dest}")
install(TARGETS proc-sender DESTINATION "${tool_dest}")
