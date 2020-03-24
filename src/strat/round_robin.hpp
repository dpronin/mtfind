#pragma once

#include <cstddef>

#include <thread>
#include <utility>
#include <algorithm>
#include <memory>
#include <vector>
#include <iterator>
#include <type_traits>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext.hpp>

#include "processors/threaded_char_chunk_processor.hpp"

#include "aux/line.hpp"
#include "aux/line_handler_ctx.hpp"

namespace mtfind::strat
{

namespace detail
{

template<typename LineReader, typename Handler>
int process_rr(LineReader &line_reader, Handler line_handler, bool process_empty_lines = false)
{
    size_t line_idx = 1;
    // process the input file line by line
    if (process_empty_lines)
    {
        for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
            line_handler(line_idx, line);
    }
    else
    {
        for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
        {
            if (!line.empty())
                line_handler(line_idx, line);
        }
    }
    return 0;
}

template<typename LineReader, typename Generator>
int process_rr(LineReader &line_reader, Generator handler_gen, size_t workers_count = 1, bool process_empty_lines = false)
{
    if (workers_count < 2)
        return process_rr(line_reader, handler_gen(), process_empty_lines);

    auto const processors_count = workers_count - 1;

    using LineProcessor = ThreadedCharChunkProcessor<decltype(handler_gen())>;
    std::vector<std::unique_ptr<LineProcessor>> workers;
    workers.reserve(processors_count);
    std::generate_n(std::back_inserter(workers), processors_count, [=] () mutable { return std::make_unique<LineProcessor>(handler_gen()); });

    // start line processors up
    for (auto &worker : workers)
        worker->start();

    auto rr_handler = [first = workers.begin(),
        last = workers.end(),
        worker_it = workers.begin()](auto line_idx, auto &&line) mutable {
            (*(*worker_it))(line_idx, std::forward<decltype(line)>(line));
            ++worker_it;
            if (last == worker_it)
                worker_it = first;
    };

    // process the input file line by line handing lines over workers
    // with each new line we switch to the next worker, this way we're balancing
    // the load between workers
    if (process_empty_lines)
    {
        size_t line_idx = 1;
        for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
            rr_handler(line_idx, line);
    }
    else
    {
        size_t line_idx = 1;
        for (auto line = line_reader(); line_reader; ++line_idx, line = line_reader())
        {
            if (!line.empty())
                rr_handler(line_idx, line);
        }
    }

    // stop line processors, wait for the workers to finish
    for (auto &worker : workers)
        worker->stop();

    return 0;
}

} // namespace detail

///
/// @brief      Process and parse with a parser each line of the region provided by a range
///             All the findings will be provided via a sink callback in line index ascending order
///             Strategy 'Round-robin' is used
///
/// @details    Round-robin approach is about distributing the load among the workers
///             equally by calling them to process lines one by one
///             The underlying implementation reads a source region line by line and
///             iterates over the workers passing them lines with their indices, no synchronization
///             is required
///
/// @param[in]  line_reader         The functor-like object that would be called to receive a line
///                                 On each iteration the reader is called If lines are exhausted by calling operator !
/// @param[in]  last                Forward iterator to the past the last character of the region
/// @param[in]  parser              The parser being called on each line
/// @param[in]  line_findings_sink  The sink for line findings
/// @param[in]  workers_count       The number of threads to be used
///
/// @tparam     Iterator            Forward character iterator
/// @tparam     Parser              A Functor like parser for a line
/// @tparam     LineFindingsSink    A Functor-like sink type
///
/// @return     0 in case of success, any other value otherwise
///
template<typename LineReader, typename Parser, typename LineFindingsSink>
int round_robin(LineReader &line_reader, Parser line_parser, LineFindingsSink &&line_findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    if (0 == workers_count)
        workers_count = 1;

    using HandlerCtx = mtfind::detail::LineHandlerCtx;
    std::vector<HandlerCtx> handlers_ctxs(workers_count);

    // generators of handlers of the file's lines being read
    auto line_handler_gen = [line_parser, ctx_it = handlers_ctxs.begin()] () mutable {
        // the handler will be called on each line by a worker
        // every worker is given a line and its index, which they pass to this handler
        return [line_parser, &ctx = *ctx_it++](auto line_idx, auto const &line) mutable {
            auto range_of_findings = line_parser(line);
            if (!range_of_findings.empty())
                ctx.consume(line_idx, std::begin(line), std::move(range_of_findings));
        };
    };

    if (auto const res = detail::process_rr(line_reader, line_handler_gen, workers_count))
        return res;

    // print out the final results
    auto all_lines_findings = handlers_ctxs | boost::adaptors::transformed([](auto const &ctx){ return ctx.lines_findings(); });
    std::cout << boost::accumulate(all_lines_findings, 0, [](auto sum, auto const &item){ return sum + item.size(); }) << '\n';

    using LinesFindingsIteratorRange = typename mtfind::detail::LinesFindings::const_iterator;
    using LinesFindingsRange         = std::pair<LinesFindingsIteratorRange, LinesFindingsIteratorRange>;
    using LinesFindingsRanges        = std::vector<LinesFindingsRange>;
    LinesFindingsRanges result_ranges;
    result_ranges.reserve(workers_count);

    boost::remove_erase_if(handlers_ctxs, [](auto const &ctx){ return ctx.lines_findings().empty(); });
    boost::transform(handlers_ctxs, std::back_inserter(result_ranges), [](auto const &ctx) {
        return std::make_pair(ctx.lines_findings().begin(), ctx.lines_findings().end());
    });

    // send one by one findings to the sink in ascending order sorted by line index
    // contexts's lines findings ranges given are sorted ascendingly themselves since RR strategy is used
    // the task is to behave like merge sort algorithm by finding out the minimal item
    // on each iteration
    while (!result_ranges.empty())
    {
        // find a topleast item between the least items of all the contexts's lines findings
        auto range_it = boost::min_element(result_ranges, [](auto item, auto min){
            return *(item.first) < *(min.first);
        });

        line_findings_sink(*(range_it->first));

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

    return 0;
}

} // namespace mtfind
