#pragma once

#include <cstddef>

#include <thread>
#include <utility>
#include <vector>
#include <functional>
#include <iterator>
#include <type_traits>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "processors/multithreaded_task_processor.hpp"

#include "aux/char_range_chunk_reader.hpp"
#include "aux/line_handler_ctx.hpp"
#include "aux/iterator_concept.hpp"

namespace mtfind::strat
{

namespace detail
{

template<typename Iterator, typename Generator, typename = mtfind::detail::is_random_access_char_iterator<Iterator>>
auto generate_line_handlers_tasks(Iterator start, Iterator end, size_t tasks_number, Generator handler_gen, bool process_empty_lines = false)
{
    using Task = std::function<void()>;

    std::vector<Task> tasks;

    if (0 == tasks_number)
        return tasks;

    std::function<Task(Iterator, Iterator, decltype(handler_gen()))> task_generator;

    if (process_empty_lines)
    {
        task_generator = [] (auto first, auto last, auto handler) {
            return [=] () mutable {
                mtfind::detail::CharRangeChunkReader<Iterator> line_reader(first, last);
                size_t line_idx = 1;
                // process the input file line by line
                for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
                    handler(line_idx, line);
            };
        };
    }
    else
    {
        task_generator = [] (auto first, auto last, auto handler) {
            return [=] () mutable {
                mtfind::detail::CharRangeChunkReader<Iterator> line_reader(first, last);
                size_t line_idx = 1;
                // process the input file line by line
                for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
                {
                    if (!line.empty())
                        handler(line_idx, line);
                };
            };
        };
    }

    // all the next lines are dedicated to fairly dividing the region among tasks
    auto data_chunk_size = std::distance(start, end) / tasks_number;
    if (0 == data_chunk_size)
        data_chunk_size = 1;

    // a helper for searching for a nearest new line symbol
    auto find_next_nl = [=](auto first){
        return std::find(std::next(first, std::min(data_chunk_size, static_cast<size_t>(std::distance(first, end)))), end, '\n');
    };

    size_t data_chunk_i = 0;
    for (auto first = start; first != end; ++data_chunk_i)
    {
        auto last = (data_chunk_i < tasks_number - 1) ? find_next_nl(first) : end;
        while (end != last && *last == '\n')
            ++last;
        // generate a task for the piece of the region
        tasks.push_back(task_generator(first, last, handler_gen()));
        first = last;
    }

    return tasks;
}

} // namespace detail

///
/// @brief      Process and parse with a parser each line of the region provided by a range
///             All the findings will be provided via a sink callback in line index ascending order
///             Strategy Divide-and-conquer is used
///
/// @details    Divide-and-conquer approach is about dividing the region into equal-length
///             subregions and pass each of them in a separate worker thread, no synchronization
///             is required
///
/// @param[in]  first               RAI to the first character of the region
/// @param[in]  last                RAI to the past the last character of the region
/// @param[in]  parser              The parser being called on each line
/// @param[in]  line_findings_sink  The sink for line findings
/// @param[in]  workers_count       The number of threads to be used
///
/// @tparam     Iterator            RAI character iterator
/// @tparam     Parser              A Functor like parser for a line
/// @tparam     LineFindingsSink    A Functor-like sink type
///
/// @return     0 in case of success, any other value otherwise
///
template<typename Iterator, typename Parser, typename LineFindingsSink>
typename std::enable_if<mtfind::detail::is_random_access_char_iterator<Iterator>::value, int>::type
divide_and_conquer(Iterator first, Iterator last, Parser parser, LineFindingsSink &&line_findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    if (0 == workers_count)
        workers_count = 1;

    struct HandlerCtx : mtfind::detail::LineHandlerCtx { size_t last_line_idx = 0; };
    std::vector<HandlerCtx> handlers_ctxs(workers_count);

    // generators of handlers of the file's lines being read
    auto line_handler_generator = [parser, ctx_it = handlers_ctxs.begin()] () mutable {
        // the handler will be called on each line by a worker
        // every worker starts handling lines from the line index 1 (see above)
        // to learn what the index has been last we preserve the 'last_line_idx'
        // in order to restore real line indices in the file afterward
        return [parser, &ctx = *ctx_it++](auto line_idx, auto const &line) mutable {
            if (!line.empty())
            {
                auto range_of_findings = parser(std::forward<decltype(line)>(line));
                if (!range_of_findings.empty())
                    ctx.consume(line_idx, std::begin(line), std::move(range_of_findings));
            }
            ctx.last_line_idx = line_idx;
        };
    };

    MultithreadedTaskProcessor task_processor(workers_count);

    // generate tasks responsible for processing different equal-length regions of the underlying range
    auto tasks = detail::generate_line_handlers_tasks(first, last, task_processor.workers_count(), line_handler_generator, true);

    // run the task processor up
    task_processor.run();
    // feed the task processor with the tasks created above
    for (auto &task : tasks)
        task_processor(std::move(task));
    // wait for the workers of the task processor to finish
    task_processor.wait();

    // print out the final results
    auto all_lines_findings = handlers_ctxs | boost::adaptors::transformed([](auto const &ctx){ return ctx.lines_findings(); });
    std::cout << boost::accumulate(all_lines_findings, 0, [](auto sum, auto const &item){ return sum + item.size(); }) << '\n';

    // line offset is necessary for adjusting line indices given by every worker
    // that has started from line index 1
    // to recover true line numbers from the file we need to take into account
    // the last line processed by every worker
    size_t line_offset = 0;
    for (auto &ctx : handlers_ctxs)
    {
        for (auto &line_findings : ctx.lines_findings())
        {
            // recovering real line numbers
            line_findings.first += line_offset;
            line_findings_sink(std::move(line_findings));
        }
        // adding last line index processed by current worker
        // in order to recover real line numbers that the next worker
        // has processed
        line_offset += ctx.last_line_idx;
    }

    return 0;
}

///
/// @brief      Process and parse with a parser each line of the region provided by a range
///             All the findings will be provided via a sink callback in line index ascending order
///             Strategy Divide-and-conquer is used
///
/// @details    Divide-and-conquer approach is about dividing the region into equal-length
///             subregions and pass each of them in a separate worker thread, no synchronization
///             is required
///
/// @param[in]  source_range        Range of characters given
/// @param[in]  parser              The parser being called on each line
/// @param[in]  line_findings_sink  The sink for line findings
/// @param[in]  workers_count       The number of threads to be used
///
/// @tparam     Range               Range of charachters
/// @tparam     Parser              A Functor like parser for a line
/// @tparam     LineFindingsSink    A Functor-like sink type
///
/// @return     0 in case of success, any other value otherwise
///
template<typename Range, typename Parser, typename LineFindingsSink>
decltype(auto) divide_and_conquer(Range const &source_range, Parser parser, LineFindingsSink &&line_findings_sink, size_t workers = std::thread::hardware_concurrency())
{
    return divide_and_conquer(source_range.begin(), source_range.end(), parser, std::forward<LineFindingsSink>(line_findings_sink), workers);
}

} // namespace mtfind
