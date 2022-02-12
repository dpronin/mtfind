find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)

set(SOURCES
    main.cpp
)

set(HEADERS
    ${MTFIND_SRC_DIR}/aux/chunk.hpp
    ${MTFIND_SRC_DIR}/aux/chunk_handler.hpp
    ${MTFIND_SRC_DIR}/aux/iterator_concept.hpp
    ${MTFIND_SRC_DIR}/splitters/stream_splitter.hpp
    ${MTFIND_SRC_DIR}/processors/multithreaded_task_processor.hpp
    ${MTFIND_SRC_DIR}/processors/threaded_chunk_processor.hpp
    ${MTFIND_SRC_DIR}/searchers/naive_searcher.hpp
    ${MTFIND_SRC_DIR}/splitters/range_splitter.hpp
    ${MTFIND_SRC_DIR}/strat/divide_and_conquer.hpp
    ${MTFIND_SRC_DIR}/strat/round_robin.hpp
    ${MTFIND_SRC_DIR}/tokenizers/range_tokenizer.hpp
    lorem_ipsum.hpp
)

add_executable(ut ${SOURCES} ${HEADERS})
target_include_directories(ut PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${MTFIND_SRC_DIR})

add_test(NAME MTFIND_UT COMMAND ut)

target_link_libraries(ut PRIVATE Threads::Threads)
target_link_libraries(ut PRIVATE Boost::boost Boost::system)
target_link_libraries(ut PRIVATE GTest::gtest GTest::gmock)
