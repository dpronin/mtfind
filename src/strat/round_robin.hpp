#pragma once

#include <cstddef>
#include <cstdlib>

#include <thread>
#include <utility>
#include <algorithm>
#include <memory>
#include <vector>
#include <iterator>
#include <type_traits>
#include <memory>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/iterator_range.hpp>
// #include <boost/container/small_vector.hpp>

#include "processors/threaded_chunk_processor.hpp"

#include "aux/chunk.hpp"
#include "aux/chunk_handler_ctx.hpp"

namespace mtfind::strat
{

namespace detail
{

template<typename T1, typename T2>
struct RRChunk
{
    T1 idx;
    T2 value;
};

template<typename ChunkReader, typename ChunkHandler>
int process_rr(ChunkReader &&reader, ChunkHandler handler, bool process_empty_chunks = false)
{
    // process the input file chunk by chunk
    if (process_empty_chunks)
    {
        size_t chunk_idx = 0;
        for (auto chunk = reader(); reader; ++chunk_idx, chunk = reader())
            handler(chunk_idx, std::move(chunk));
    }
    else
    {
        size_t chunk_idx = 0;
        for (auto chunk = reader(); reader; ++chunk_idx, chunk = reader())
        {
            if (!chunk.empty())
                handler(chunk_idx, std::move(chunk));
        }
    }
    return 0;
}

template<typename ChunkReader, typename ChunkHandlerGenerator>
int process_rr(ChunkReader &&reader, ChunkHandlerGenerator generator, size_t workers_count = 1, bool process_empty_chunks = false)
{
    // one threaded case of solving
    if (workers_count < 2)
        return process_rr(std::forward<ChunkReader>(reader), generator(), process_empty_chunks);

    auto const processors_count = workers_count - 1;

    auto generator_wrapper = [generator] () mutable {
        return [handler = generator()](auto &&chunk) mutable {
            handler(chunk.idx, std::forward<decltype(chunk.value)>(chunk.value));
        };
    };

    using Chunk          = RRChunk<size_t, decltype(reader())>;
    using ChunkProcessor = ThreadedChunkProcessor<decltype(generator_wrapper()), Chunk>;

    using data_aligned_t = typename std::aligned_storage<sizeof(ChunkProcessor), alignof(ChunkProcessor)>::type;
    std::unique_ptr<data_aligned_t[]> storage(new data_aligned_t[processors_count]);
    auto *processors_ptr = reinterpret_cast<ChunkProcessor*>(storage.get());
    auto processors = boost::make_iterator_range(processors_ptr, processors_ptr + processors_count);

    for (auto &processor : processors)
        new (std::addressof(processor)) ChunkProcessor(generator_wrapper());

    auto rr_handler = [&processors, cur_proc = processors.begin()] (auto chunk_idx, auto const &value) mutable {
        while (!(*cur_proc)({chunk_idx, value}))
            ;
        {
            ++cur_proc;
            if (processors.end() == cur_proc)
                cur_proc = processors.begin();
        }
    };

    // start chunk processors up
    for (auto &processor : processors)
        processor.start();

    // process the input file chunk by chunk handing chunks over workers
    // with each new chunk we switch to the next worker, this way we're balancing
    // the load between workers
    if (process_empty_chunks)
    {
        size_t chunk_idx = 0;
        for (auto chunk = reader(); reader; ++chunk_idx, chunk = reader())
            rr_handler(chunk_idx, std::move(chunk));
    }
    else
    {
        size_t chunk_idx = 0;
        for (auto chunk = reader(); reader; ++chunk_idx, chunk = reader())
        {
            if (!chunk.empty())
                rr_handler(chunk_idx, std::move(chunk));
        }
    }

    // stop chunk processors, wait for the workers to finish
    for (auto &processor : processors)
        processor.stop();

    // call dtors manually
    for (auto &processor : processors)
        processor.~ChunkProcessor();

    return 0;
}

} // namespace detail

///
/// @brief      Process and parse with a tokenizer each chunk that the reader returns
///             All the findings will be given to a sink callback in chunk index ascending order
///             Strategy 'Round-Robin' is used
///
/// @details    Round-Robin approach is about distributing the load among the workers
///             equally by calling them to process chunks one by one
///             The underlying implementation reads a source region chunk by chunk and
///             iterates over the workers passing them chunks with their indices, no synchronization
///             is required
///
/// @param[in]  reader         A functor-like object that would be called to receive a chunk
///                            The reader is called until it is exhausted, what it is checked by calling operator!()
/// @param[in]  tokenizer      A tokenizer being called on each chunk to receive findings
/// @param[in]  findings_sink  A sink for chunk findings
/// @param[in]  workers_count  A number of threads to use
///
/// @tparam     ChunkReader    A reader that will be fetching a chunk until reader's source has depleted
/// @tparam     ChunkTokenizer A functor like tokenizer for a chunk fetched
/// @tparam     FindingsSink   A functor-like sink type for a chunk finding
///
/// @return     0 in case of success, any other values otherwise
///
template<typename ChunkReader, typename ChunkTokenizer, typename FindingsSink>
int round_robin(ChunkReader &&reader, ChunkTokenizer tokenizer, FindingsSink findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    if (0 == workers_count)
        workers_count = 1;

    using ContextType = mtfind::detail::ChunkHandlerCtx<decltype(reader())>;
    std::vector<ContextType> handlers_ctxs(workers_count);

    // generators of handlers of the file's chunks being read
    auto chunk_handler_gen = [tokenizer, ctx_it = handlers_ctxs.begin()] () mutable {
        // constexpr size_t kSmallVectorCapacity = 100;
        using RangeIterator = typename ContextType::value_type::const_iterator;
        // @info no big difference between small_vector and a regular vector in this case of usage
        // using Container = boost::container::small_vector<boost::iterator_range<Iterator>>, kSmallVectorCapacity>;
        using Container = std::vector<boost::iterator_range<RangeIterator>>;
        // the handler will be called on each chunk by a worker
        // every worker is given a chunk and its index, which they pass to this handler
        return [tokenizer, &ctx = *ctx_it++, tokens = Container()](auto chunk_idx, auto const &chunk_value) mutable {
            tokenizer(chunk_value, std::back_inserter(tokens));
            if (!tokens.empty())
            {
                ctx.consume(chunk_idx, std::begin(chunk_value), tokens);
                tokens.clear();
            }
        };
    };

    if (auto const res = detail::process_rr(std::forward<ChunkReader>(reader), chunk_handler_gen, workers_count))
        return res;

    // erase all contextes with empty findings
    boost::remove_erase_if(handlers_ctxs, [](auto const &ctx){ return ctx.empty(); });

    // print out the final results
    std::cout << boost::accumulate(handlers_ctxs, 0, [](auto sum, auto const &item){ return sum + item.size(); }) << '\n';

    using ChunksFindingsIterator = typename ContextType::const_iterator;
    using ChunksFindingsRange    = std::pair<ChunksFindingsIterator, ChunksFindingsIterator>;
    using ChunksFindingsRanges   = std::vector<ChunksFindingsRange>;
    ChunksFindingsRanges result_ranges;
    result_ranges.reserve(handlers_ctxs.size());

    boost::transform(handlers_ctxs, std::back_inserter(result_ranges), [](auto const &ctx) {
        return std::make_pair(ctx.begin(), ctx.end());
    });

    // send one by one findings to the sink in ascending order sorted by chunk index
    // contexts's chunks findings ranges given are sorted ascendingly themselves since RR strategy is used
    // the task is to behave like merge sort algorithm by finding out the minimal-indexed item
    // on each iteration
    while (!result_ranges.empty())
    {
        // find a topleast item between the least items of all the contexts's chunks findings
        auto range_it = boost::min_element(result_ranges, [](auto item, auto min){
            return (item.first)->first < (min.first)->first;
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

} // namespace mtfind
