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
#include <boost/utility/string_view.hpp>
#include <boost/range/iterator_range.hpp>
// #include <boost/container/small_vector.hpp>

#include "processors/multithreaded_task_processor.hpp"

#include "aux/range_splitter.hpp"
#include "aux/chunk_handler_ctx.hpp"
#include "aux/iterator_concept.hpp"

namespace mtfind::strat
{

namespace detail
{

template<typename Iterator,
typename HandlerGenerator,
typename Task = std::function<void()>,
typename = typename std::enable_if<mtfind::detail::is_random_access_char_iterator<Iterator>>::type>
auto generate_chunk_handlers_tasks(Iterator start, Iterator end, size_t tasks_number, HandlerGenerator handler_generator, bool process_empty_chunks = false)
{
    std::vector<Task> tasks;

    if (0 == tasks_number)
        return tasks;

    std::function<Task(Iterator, Iterator, decltype(handler_generator()))> task_generator;

    if (process_empty_chunks)
    {
        task_generator = [] (auto first, auto last, auto handler) {
            return [=] () mutable {
                mtfind::detail::RangeSplitter<Iterator> chunk_reader(first, last);
                size_t chunk_idx = 0;
                // process the input file chunk by chunk
                for (auto chunk = chunk_reader(); chunk_reader; ++chunk_idx, chunk = chunk_reader())
                    handler(chunk_idx, chunk);
            };
        };
    }
    else
    {
        task_generator = [] (auto first, auto last, auto handler) {
            return [=] () mutable {
                mtfind::detail::RangeSplitter<Iterator> chunk_reader(first, last);
                size_t chunk_idx = 0;
                // process the input file chunk by chunk
                for (auto chunk = chunk_reader(); chunk_reader; ++chunk_idx, chunk = chunk_reader())
                {
                    if (!chunk.empty())
                        handler(chunk_idx, chunk);
                };
            };
        };
    }

    // all the next code lines are dedicated to fairly dividing the region among tasks
    auto data_chunk_size = std::distance(start, end) / tasks_number;
    if (0 == data_chunk_size)
        data_chunk_size = 1;

    // a helper for seeking a nearest new line symbol
    auto find_next_nl = [=](auto first){
        return std::find(std::next(first, std::min(data_chunk_size, static_cast<size_t>(std::distance(first, end)))), end, '\n');
    };

    size_t data_chunk_i = 0;
    for (auto first = start; first != end; ++data_chunk_i)
    {
        auto last = (data_chunk_i < tasks_number - 1) ? find_next_nl(first) : end;
        // if we got onto a boundary where several successive new line
        // symbols occur, the task will get them all to process until the toplast includingly
        while (end != last && *last == '\n')
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
///             Strategy Divide-and-Conquer is used
///
/// @details    Divide-and-Conquer approach is about dividing the region into equal-length
///             subregions and pass each of them in a separate worker thread, no synchronization
///             is required
///
/// @param[in]  first                A RAI to the first of the region
/// @param[in]  last                 A RAI to the past the last of the region
/// @param[in]  tokenizer            A tokenizer being called on each chunk
/// @param[in]  chunk_findings_sink  A sink for chunk findings
/// @param[in]  workers_count        A number of threads to use
///
/// @tparam     Iterator             A RAI iterator
/// @tparam     RangeTokenizer       A functor like tokenizer for a chunk
/// @tparam     ChunkFindingsSink    A functor-like sink type
///
/// @return     0 in case of success, any other values otherwise
///
template<typename Iterator, typename RangeTokenizer, typename ChunkFindingsSink>
typename std::enable_if<mtfind::detail::is_random_access_char_iterator<Iterator>, int>::type
divide_and_conquer(Iterator first, Iterator last, RangeTokenizer tokenizer, ChunkFindingsSink &&findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    if (0 == workers_count)
        workers_count = 1;

    class HandlerCtx : public mtfind::detail::ChunkHandlerCtx<boost::string_view>
    {
    public:
        void set_last_chunk_idx(size_t chunk_idx) noexcept { last_chunk_idx_ = chunk_idx + 1; }
        size_t last_chunk_idx() const noexcept { return last_chunk_idx_; }

    private:
        size_t last_chunk_idx_ = 0;
    };

    using ContextType = HandlerCtx;
    std::vector<ContextType> handlers_ctxs(workers_count);

    // generators of handlers of the file's chunks being read
    auto chunk_handler_generator = [tokenizer, ctx_it = handlers_ctxs.begin()] () mutable {
        // constexpr size_t kSmallVectorCapacity = 100;
        using RangeIterator = typename ContextType::value_type::const_iterator;
        // @info no big difference between small_vector and a regular vector in this case of usage
        // using Container = boost::container::small_vector<boost::iterator_range<Iterator>>, kSmallVectorCapacity>;
        using Container = std::vector<boost::iterator_range<RangeIterator>>;
        // the handler will be called on each chunk by a worker
        // every worker starts handling chunks from the chunk index 1 (see above)
        // to learn what the index has been last we preserve the 'last_chunk_idx'
        // in order to restore real chunk indices in the file afterward
        return [tokenizer, &ctx = *ctx_it++, tokens = Container()](auto chunk_idx, auto const &chunk_value) mutable {
            if (!chunk_value.empty())
            {
                tokenizer(chunk_value, std::back_inserter(tokens));
                if (!tokens.empty())
                {
                    ctx.consume(chunk_idx, std::begin(chunk_value), tokens);
                    tokens.clear();
                }
            }
            ctx.set_last_chunk_idx(chunk_idx);
        };
    };

    MultithreadedTaskProcessor task_processor(workers_count);

    // generate tasks responsible for processing different equal-length regions of the underlying range
    auto tasks = detail::generate_chunk_handlers_tasks(first, last, task_processor.workers_count(), chunk_handler_generator, true);

    // run the task processor up
    task_processor.run();
    // feed the task processor with the tasks created above
    for (auto &task : tasks)
        task_processor(std::move(task));
    // wait for the workers of the task processor to finish
    task_processor.wait();

    // print out the final results
    auto all_chunks_findings = handlers_ctxs | boost::adaptors::transformed([](auto const &ctx){ return ctx.chunks_findings(); });
    std::cout << boost::accumulate(all_chunks_findings, 0, [](auto sum, auto const &item){ return sum + item.size(); }) << '\n';

    // chunk offset is necessary for adjusting chunk indices given by every worker
    // that has started from chunk index 1
    // to recover true chunk numbers from the file we need to take into account
    // the last chunk processed by every worker
    size_t chunk_offset = 0;
    for (auto &ctx : handlers_ctxs)
    {
        for (auto &chunk_findings : ctx)
        {
            // recovering real chunk numbers
            chunk_findings.first += chunk_offset;
            findings_sink(std::move(chunk_findings));
        }
        // adding last chunk index processed by current worker
        // in order to recover real chunk numbers that the next worker
        // has processed
        chunk_offset += ctx.last_chunk_idx();
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
/// @param[in]  source_range   A Range of the source region
/// @param[in]  tokenizer      A tokenizer being called on each chunk
/// @param[in]  findings_sink  A sink for chunk findings
/// @param[in]  workers_count  A number of threads to use
///
/// @tparam     Range               Range
/// @tparam     RangeTokenizer      A Functor like tokenizer for a chunk
/// @tparam     ChunkFindingsSink   A Functor-like sink type
///
/// @return     0 in case of success, any other values otherwise
///
template<typename Range, typename RangeTokenizer, typename ChunkFindingsSink>
decltype(auto) divide_and_conquer(Range const &source_range, RangeTokenizer tokenizer, ChunkFindingsSink &&findings_sink, size_t workers_count = std::thread::hardware_concurrency())
{
    return divide_and_conquer(std::begin(source_range), std::end(source_range), tokenizer, std::forward<ChunkFindingsSink>(findings_sink), workers_count);
}

} // namespace mtfind
