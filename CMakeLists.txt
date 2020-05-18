cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake AND GEN_CONAN_BUILD_INFO)
    execute_process(COMMAND conan install ${CMAKE_CURRENT_LIST_DIR}/ -if ${CMAKE_BINARY_DIR}/ -pr gcc --build missing RESULT_VARIABLE CONAN_INSTALL_RESULT)
    if(NOT CONAN_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to conan install")
    endif()
endif()

if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(FATAL_ERROR "conanbuildinfo.cmake required is missing")
endif()

project(mtfind VERSION 0.1.0 LANGUAGES CXX)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_CXX_STANDARD 14)

set(MTFIND_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

if (ENABLE_PROFILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if (ENABLE_GDB_SYMBOLS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

add_subdirectory(src)

if (ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

if (ENABLE_BENCH)
    add_subdirectory(bench)
endif()
