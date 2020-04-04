#pragma once

#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <iterator>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/numeric.hpp>
#include <boost/scope_exit.hpp>

#include "processors/threaded_chunk_processor.hpp"

#include "aux/chunk.hpp"
#include "aux/chunk_handler.hpp"

namespace mtfind::strat
{

namespace detail
{

template <typename T1, typename T2>
struct RRChunk
{
    T1 idx;
    T2 value;
};

template <typename ChunkReader, typename ChunkHandler>
int handle_rr(ChunkReader reader, ChunkHandler handler)
{
    // process the input produced by the reader chunk by chunk
    size_t chunk_idx = 0;
    for (auto chunk = reader(); reader; ++chunk_idx, chunk = reader())
        handler(chunk_idx, std::move(chunk));
    return EXIT_SUCCESS;
}

template <typename ChunkReader, typename ChunkHandlerGenerator>
int process_rr(ChunkReader reader, ChunkHandlerGenerator generator, size_t workers_count = 1)
{
    // this call will run processing the input produced by the reader chunk by chunk in one thread
    if (workers_count < 2)
        return handle_rr(reader, generator());

    auto const processors_count = workers_count - 1;

    auto generator_wrapper = [generator]() mutable {
        return [handler = generator()](auto &&chunk) mutable {
            handler(chunk.idx, std::forward<decltype(chunk.value)>(chunk.value));
        };
    };

    using Chunk          = RRChunk<size_t, decltype(reader())>;
    using ChunkProcessor = ThreadedChunkProcessor<decltype(generator_wrapper()), Chunk>;

    using data_aligned_t = typename std::aligned_storage<sizeof(ChunkProcessor), alignof(ChunkProcessor)>::type;
    std::unique_ptr<data_aligned_t[]> storage(new data_aligned_t[processors_count]);
    auto *                            processors_ptr = reinterpret_cast<ChunkProcessor *>(storage.get());
    auto                              processors     = boost::make_iterator_range(processors_ptr, processors_ptr + processors_count);

    // safe allocating new chunk processors
    // if an exception takes place we will gracefully call all the dtors of created items before returning
    for (auto it = processors.begin(); it != processors.end(); ++it)
    {
        try
        {
            new (std::addressof(*it)) ChunkProcessor(generator_wrapper());
        }
        catch (...)
        {
            auto processors_dealloc_range = boost::make_iterator_range(
                std::make_reverse_iterator(it),
                std::make_reverse_iterator(processors.begin()));
            for (auto &processor : processors_dealloc_range)
                processor.~ChunkProcessor();
            throw;
        }
    }

    // this will safely deallocate resources if any exception occurs
    // also will work when the function normally returns
    BOOST_SCOPE_EXIT_ALL(&)
    {
        // call dtors manually as placement new is used above
        for (auto &processor : processors)
            processor.~ChunkProcessor();
    };

    // round robin handler switching among processors as handling chinks one by one
    auto rr_handler = [&processors, cur_proc = processors.begin()](auto chunk_idx, auto const &value) mutable {
        Chunk chunk{chunk_idx, value};
        while (!(*cur_proc)(chunk))
            ;
        ++cur_proc;
        if (processors.end() == cur_proc)
            cur_proc = processors.begin();
    };

    // start chunk processors up
    for (auto &processor : processors)
        processor.start();

    // this call will run processing the input produced by the reader chunk by chunk handing chunks over workers
    // with each new chunk we switch to the next worker, this way we're balancing
    // the load between workers
    auto const res = handle_rr(reader, rr_handler);

    // stop chunk processors, wait for the workers to finish
    for (auto &processor : processors)
        processor.stop();

    return res;
}

} // namespace detail

///
/// @brief      Process and parse with a tokenizer each chunk that the reader returns
///             All the findings will be given to a sink callback in chunk index ascending order
///             The number of findings will be passed to the sink for number of findings
///             Strategy 'Round-Robin' is used
///
/// @details    Round-Robin approach is about distributing the load among the workers
///             equally by calling them to process chunks one by one
///             The underlying implementation reads a source region chunk by chunk and
///             iterates over the workers passing them chunks with their indices, no synchronization
///             is required
///
/// @param[in]  reader               A functor-like object that would be called to receive a chunk
///                                  The reader is called until it is exhausted, what it is checked by calling operator!()
/// @param[in]  tokenizer            A tokenizer being called on each chunk to receive findings
/// @param[in]  findings_number_sink A sink for overall number of findings
/// @param[in]  findings_sink        A sink for chunk findings
/// @param[in]  workers_count        A number of threads to use
///
/// @tparam     ChunkReader          A reader that will be fetching a chunk until reader's source has depleted
/// @tparam     ChunkTokenizer       A functor like tokenizer for a chunk fetched
/// @tparam     FindingsNumberSink   A functor-like sink type for overall number of findings
/// @tparam     FindingsSink         A functor-like sink type for a chunk finding
///
/// @return     0 in case of success, any other values otherwise
///
template <typename ChunkReader, typename ChunkTokenizer, typename FindingsNumberSink, typename FindingsSink>
int round_robin(ChunkReader reader, ChunkTokenizer tokenizer, FindingsNumberSink findings_number_sink, FindingsSink findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    workers_count = std::max(static_cast<size_t>(1), workers_count);

    using ChunkHandler = mtfind::detail::ChunkHandlerBase<ChunkTokenizer, decltype(reader())>;

    std::vector<ChunkHandler> handlers;
    handlers.reserve(workers_count);
    std::generate_n(std::back_inserter(handlers), workers_count, [tokenizer]() mutable { return ChunkHandler(tokenizer); });

    // generator of handlers of the chunks being read
    // each worker will be resulting in its own handler
    auto chunk_handler_generator = [cur_handler = handlers.begin()]() mutable { return std::ref(*cur_handler++); };

    if (auto const res = detail::process_rr(reader, chunk_handler_generator, workers_count))
        return res;

    // send out the final number of findings
    findings_number_sink(
        boost::accumulate(handlers, 0, [](auto sum, auto const &handler) { return sum + handler.findings().size(); }));

    using ChunksFindingsIterator = typename ChunkHandler::const_iterator;
    using ChunksFindingsRange    = std::pair<ChunksFindingsIterator, ChunksFindingsIterator>;
    using ChunksFindingsRanges   = std::vector<ChunksFindingsRange>;
    ChunksFindingsRanges result_ranges;
    result_ranges.reserve(handlers.size());

    boost::transform(handlers, std::back_inserter(result_ranges), [](auto const &handler) {
        return std::make_pair(handler.findings().begin(), handler.findings().end());
    });

    // erase all empty ranges
    boost::remove_erase_if(result_ranges, [](auto const &findings) { return findings.first == findings.second; });

    // send each finding to the sink in ascending order sorted by chunk index
    // contexts's chunks findings ranges given are sorted ascendingly themselves since RR strategy is used
    // the task is to behave like merge sort algorithm by finding out the minimal-indexed item
    // on each iteration
    while (!result_ranges.empty())
    {
        // find a topleast item between the least items of all the contexts's chunks findings
        auto range_it = boost::min_element(result_ranges, [](auto const &item, auto const &min) {
            // comparing chunks indices
            return std::get<0>(*item.first) < std::get<0>(*min.first);
        });

        findings_sink(*(range_it->first));

        // advance the topleast item's iterator to the next
        ++(range_it->first);
        // if the range is exhausted erase it from the container
        if (range_it->first == range_it->second)
        {
            if (range_it != result_ranges.end())
                *range_it = std::move(result_ranges.back());
            result_ranges.pop_back();
        }
    }

    return EXIT_SUCCESS;
}

} // namespace mtfind::strat
