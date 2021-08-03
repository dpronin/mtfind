cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(mtfind VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(FATAL_ERROR "conanbuildinfo.cmake required is missing")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_CXX_STANDARD 17)

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
