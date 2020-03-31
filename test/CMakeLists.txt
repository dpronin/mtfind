find_package(Threads REQUIRED)

set(SOURCES
    main.cpp
)

set(HEADERS
    ${MTFIND_SRC_DIR}/splitters/range_splitter.hpp
    ${MTFIND_SRC_DIR}/aux/iterator_concept.hpp
    ${MTFIND_SRC_DIR}/aux/chunk.hpp
    ${MTFIND_SRC_DIR}/aux/chunk_handler_ctx.hpp
    ${MTFIND_SRC_DIR}/tokenizers/range_tokenizer.hpp
    ${MTFIND_SRC_DIR}/processors/multithreaded_task_processor.hpp
    ${MTFIND_SRC_DIR}/processors/threaded_chunk_processor.hpp
    ${MTFIND_SRC_DIR}/searchers/naive_searcher.hpp
    ${MTFIND_SRC_DIR}/strat/round_robin.hpp
    ${MTFIND_SRC_DIR}/strat/divide_and_conquer.hpp

    lorem_ipsum.hpp
)

add_executable(mtfind_test ${SOURCES} ${HEADERS})
target_include_directories(mtfind_test PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${MTFIND_SRC_DIR})

add_test(NAME GTestMtFind COMMAND mtfind_test)

target_link_libraries(mtfind_test PRIVATE Threads::Threads)
target_link_libraries(mtfind_test PRIVATE CONAN_PKG::boost)
target_link_libraries(mtfind_test PRIVATE CONAN_PKG::gtest)
