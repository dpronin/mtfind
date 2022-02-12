find_package(Boost REQUIRED COMPONENTS system iostreams)
find_package(Threads REQUIRED)

set(SOURCES
    main.cpp
)

set(HEADERS
    application.hpp
    aux/chunk.hpp
    aux/chunk_handler.hpp
    aux/iterator_concept.hpp
    splitters/stream_splitter.hpp
    processors/multithreaded_task_processor.hpp
    processors/threaded_chunk_processor.hpp
    searchers/boyer_moore_searcher.hpp
    searchers/naive_searcher.hpp
    splitters/range_splitter.hpp
    strat/divide_and_conquer.hpp
    strat/round_robin.hpp
    tokenizers/range_tokenizer.hpp
)

add_executable(mtfind ${SOURCES} ${HEADERS})
target_include_directories(mtfind PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(mtfind PRIVATE Boost::boost Boost::system Boost::iostreams)
target_link_libraries(mtfind PRIVATE Threads::Threads)

install(TARGETS mtfind RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
