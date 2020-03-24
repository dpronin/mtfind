find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(SOURCES
    main.cpp
)

set(HEADERS
    ${MTFIND_SRC_DIR}/aux/char_range_chunk_reader.hpp
    ${MTFIND_SRC_DIR}/aux/iterator_concept.hpp
    ${MTFIND_SRC_DIR}/aux/line.hpp
    ${MTFIND_SRC_DIR}/aux/line_handler_ctx.hpp
    ${MTFIND_SRC_DIR}/tokenizers/range_tokenizer.hpp
    ${MTFIND_SRC_DIR}/processors/multithreaded_task_processor.hpp
    ${MTFIND_SRC_DIR}/processors/threaded_char_chunk_processor.hpp
    ${MTFIND_SRC_DIR}/searchers/searcher.hpp
    ${MTFIND_SRC_DIR}/strat/round_robin.hpp
    ${MTFIND_SRC_DIR}/strat/divide_and_conquer.hpp
)

set(CMAKE_CXX_STANDARD 14)

add_executable(mtfind_bench ${SOURCES} ${HEADERS})
target_include_directories(mtfind_bench PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(mtfind_bench PRIVATE Boost::boost)
target_link_libraries(mtfind_bench PRIVATE Threads::Threads)
target_link_libraries(mtfind_bench PRIVATE CONAN_PKG::benchmark)
