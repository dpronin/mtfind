cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(mtfind VERSION 1.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(MTFIND_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

if (ENABLE_PROFILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if (ENABLE_GDB_SYMBOLS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(test)
endif()

if (ENABLE_BENCH)
    add_subdirectory(bench)
endif()
