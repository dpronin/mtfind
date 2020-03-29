find_package(Boost REQUIRED COMPONENTS iostreams system)
find_package(Threads REQUIRED)

set(SOURCES
    main.cpp
)

set(HEADERS
    application.hpp
    strat/round_robin.hpp
    strat/divide_and_conquer.hpp
    aux/chunk.hpp
    aux/range_splitter.hpp
    aux/chunk_handler_ctx.hpp
    aux/iterator_concept.hpp
    searchers/searcher.hpp
    tokenizers/range_tokenizer.hpp
    processors/multithreaded_task_processor.hpp
    processors/threaded_char_chunk_processor.hpp
)

add_executable(mtfind ${SOURCES} ${HEADERS})
target_include_directories(mtfind PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(mtfind PRIVATE Boost::boost)
target_link_libraries(mtfind PRIVATE Boost::iostreams)
target_link_libraries(mtfind PRIVATE Boost::system)
target_link_libraries(mtfind PRIVATE Threads::Threads)
