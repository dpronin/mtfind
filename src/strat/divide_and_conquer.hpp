#pragma once

#include <cstddef>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/numeric.hpp>

#include "processors/multithreaded_task_processor.hpp"

#include "aux/chunk_handler.hpp"
#include "aux/iterator_concept.hpp"
#include "splitters/range_splitter.hpp"

namespace mtfind::strat
{

namespace detail
{

template <typename Iterator, typename HandlerGenerator, typename ValueType = typename std::iterator_traits<Iterator>::value_type, typename Task = std::function<void()>>
typename std::enable_if<mtfind::detail::is_random_access_iterator<Iterator>, std::vector<Task>>::type
    generate_chunk_handlers_tasks(Iterator start, Iterator end, size_t tasks_number, HandlerGenerator handler_generator, ValueType const &delim = ValueType())
{
    std::vector<Task> tasks;

    if (0 == tasks_number)
        return tasks;

    auto task_generator = [=](auto first, auto last, auto handler) {
        return [=]() mutable {
            RangeSplitter<Iterator> splitter(first, last, delim);
            size_t                  chunk_idx = 0;
            // process the input produced by the splitter chunk by chunk
            for (auto chunk = splitter(); splitter; ++chunk_idx, chunk = splitter())
                handler(chunk_idx, std::move(chunk));
        };
    };

    // all the next code lines are dedicated to fairly dividing the region among tasks
    auto data_chunk_size = std::max(static_cast<size_t>(1), std::distance(start, end) / tasks_number);

    // a helper for seeking a nearest delimiter symbol
    auto find_next_nl = [=](auto first) {
        return std::find(std::next(first, std::min(data_chunk_size, static_cast<size_t>(std::distance(first, end)))), end, delim);
    };

    size_t data_chunk_i = 0;
    for (auto first = start; first != end; ++data_chunk_i)
    {
        auto last = (data_chunk_i < tasks_number - 1) ? find_next_nl(first) : end;
        // if we got onto a boundary where several successive delimiter
        // symbols occur, the task will get them all to process until the toplast includingly
        while (end != last && *last == delim)
            ++last;
        // generate a task for the piece of the region
        tasks.push_back(task_generator(first, last, handler_generator()));
        first = last;
    }

    return tasks;
}

} // namespace detail

///
/// @brief      Process and parse with a tokenizer each chunk of the region provided by a range
///             All the findings will be provided via a sink callback in chunk index ascending order
///             The number of findings will be passed to the sink for number of findings
///             Strategy Divide-and-Conquer is used
///
/// @details    Divide-and-Conquer approach is about dividing the region into equal-length
///             subregions and pass each of them in a separate worker thread, no synchronization
///             is required
///
/// @param[in]  first                A RAI to the first of the region
/// @param[in]  last                 A RAI to the past the last of the region
/// @param[in]  tokenizer            A tokenizer being called on each chunk
/// @param[in]  findings_number_sink A sink for overall number of findings
/// @param[in]  chunk_findings_sink  A sink for chunk findings
/// @param[in]  delim                A delimiter for separating chunks from each other
/// @param[in]  workers_count        A number of threads to use
///
/// @tparam     Iterator             A RAI iterator
/// @tparam     ChunkTokenizer       A functor like tokenizer for a chunk
/// @tparam     FindingsNumberSink   A functor-like sink type for overall number of findings
/// @tparam     FindingsSink         A functor-like sink type for a chunk finding
///
/// @return     0 in case of success, any other values otherwise
///
template <typename Iterator, typename ChunkTokenizer, typename FindingsNumberSink, typename FindingsSink, typename ValueType = typename std::iterator_traits<Iterator>::value_type>
typename std::enable_if<mtfind::detail::is_random_access_iterator<Iterator>, int>::type
    divide_and_conquer(Iterator first, Iterator last, ChunkTokenizer tokenizer, FindingsNumberSink findings_number_sink, FindingsSink findings_sink, ValueType const &delim = ValueType(), size_t workers_count = std::thread::hardware_concurrency())
{
    workers_count = std::max(static_cast<size_t>(1), workers_count);

    using ChunkHandler = mtfind::detail::ChunkHandler<ChunkTokenizer, boost::iterator_range<Iterator>>;

    std::vector<ChunkHandler> handlers;
    handlers.reserve(workers_count);
    std::generate_n(std::back_inserter(handlers), workers_count, [tokenizer]() mutable { return ChunkHandler(tokenizer); });

    // generators of handlers of the chunks being read
    auto chunk_handler_generator = [cur_handler = handlers.begin()]() mutable { return std::ref(*cur_handler++); };

    MultithreadedTaskProcessor task_processor(workers_count);

    // generate tasks responsible for processing different equal-lengthed regions of the underlying range
    auto tasks = detail::generate_chunk_handlers_tasks(first, last, task_processor.workers_count(), chunk_handler_generator, delim);

    // run the task processor up
    task_processor.run();
    // feed the task processor with the tasks created above
    for (auto &task : tasks)
        task_processor(std::move(task));
    // wait for the workers of the task processor to finish
    task_processor.wait();

    auto all_findings = handlers | boost::adaptors::transformed([](auto const &handler) { return handler.findings(); });
    // send out the final number of findings
    findings_number_sink(boost::accumulate(all_findings, 0, [](auto sum, auto const &findings) { return sum + findings.size(); }));

    // chunk offset is necessary for adjusting chunk indices given by every worker
    // that has started from chunk index 1
    // to recover true chunk numbers with respect to the chunk reader's order we need to take into account
    // the last chunk processed by every worker
    size_t chunk_offset = 0;
    for (auto &handler : handlers)
    {
        for (auto &finding : handler.findings())
        {
            // recovering real chunk numbers
            std::get<0>(finding) += chunk_offset;
            findings_sink(std::move(finding));
        }
        // adding last chunk index processed by current worker
        // in order to recover real chunk numbers that the next worker
        // has processed
        chunk_offset += handler.last_chunk_idx();
    }

    return EXIT_SUCCESS;
}

///
/// @brief      Process and parse with a tokenizer each chunk of the region provided by a range
///             All the findings will be provided via a sink callback in chunk index ascending order
///             Strategy Divide-and-Conquer is used
///
/// @details    Divide-and-Conquer approach is about dividing the region into equal-length
///             subregions and pass each of them in a separate worker thread, no synchronization
///             is required
///
/// @param[in]  source_range         A Range of the source region
/// @param[in]  tokenizer            A tokenizer being called on each chunk
/// @param[in]  findings_number_sink A sink for overall number of findings
/// @param[in]  findings_sink        A sink for chunk findings
/// @param[in]  delim                A delimiter for separating chunks from each other
/// @param[in]  workers_count        A number of threads to use
///
/// @tparam     Range                Range
/// @tparam     ChunkTokenizer       A Functor like tokenizer for a chunk
/// @tparam     FindingsNumberSink   A functor-like sink type for overall number of findings
/// @tparam     FindingsSink         A Functor-like sink type for a chunk finding
///
/// @return     0 in case of success, any other values otherwise
///
template <typename Range, typename ChunkTokenizer, typename FindingsNumberSink, typename FindingsSink, typename ValueType = typename std::iterator_traits<decltype(std::begin(std::declval<Range>()))>::value_type>
decltype(auto) divide_and_conquer(Range const &source_range, ChunkTokenizer tokenizer, FindingsNumberSink findings_number_sink, FindingsSink findings_sink, ValueType const &delim = ValueType(), size_t workers_count = std::thread::hardware_concurrency())
{
    return divide_and_conquer(std::begin(source_range), std::end(source_range), tokenizer, findings_number_sink, findings_sink, delim, workers_count);
}

} // namespace mtfind::strat
