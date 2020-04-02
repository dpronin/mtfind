#include <cstdlib>

#include <string>
#include <algorithm>
#include <sstream>
#include <functional>

#include <benchmark/benchmark.h>

#include "splitters/range_splitter.hpp"
#include "splitters/stream_splitter.hpp"

#include "searchers/naive_searcher.hpp"
#include "searchers/boyer_moore_searcher.hpp"

#include "lorem_ipsum.hpp"

using namespace benchmark;

namespace mtfind::bench
{

template<typename SplitterT>
void BM_RangeSplitter_Lines(State& state)
{
    std::string const line = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";

    auto const lines_number = state.range(0);

    std::string text;
    text.reserve((line.size() + 1) * lines_number);

    for (size_t i = 0; i < lines_number; ++i)
        text += line;

    for (auto _ : state) {
        state.PauseTiming();
        SplitterT line_splitter(text, '\n');
        state.ResumeTiming();
        for (auto line = line_splitter(); line_splitter; line = line_splitter())
            DoNotOptimize(line);
    }

    state.SetItemsProcessed(state.iterations() * lines_number);
    state.SetBytesProcessed(state.iterations() * text.size());
    state.SetComplexityN(lines_number);
}

template<typename SplitterT>
void BM_StreamSplitter_Lines(State& state)
{
    std::string const line = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";

    auto const lines_number = state.range(0);

    std::string text;
    text.reserve((line.size() + 1) * lines_number);

    for (size_t i = 0; i < lines_number; ++i)
        text += line;

    for (auto _ : state) {
        state.PauseTiming();
        std::istringstream text_stream(text);
        SplitterT line_splitter(text_stream, '\n');
        state.ResumeTiming();
        for (auto line = line_splitter(); line_splitter; line = line_splitter())
            DoNotOptimize(line);
    }

    state.SetItemsProcessed(state.iterations() * lines_number);
    state.SetBytesProcessed(state.iterations() * text.size());
    state.SetComplexityN(lines_number);
}

template<typename SearcherT>
void BM_Searcher_NoComp_LoremIpsum(State& state)
{
    auto const lorem_ipsum_count = state.range(0);

    std::string const pattern = "taciti";
    SearcherT searcher(std::move(pattern));

    for (auto _ : state)
    {
        for (size_t i = 0; i < lorem_ipsum_count; ++i)
        {
            auto token = searcher(std::begin(kLoremIpsum), std::end(kLoremIpsum));
            DoNotOptimize(token);
        }
    }

    state.SetItemsProcessed(state.iterations() * lorem_ipsum_count);
    state.SetBytesProcessed(state.items_processed() * sizeof(kLoremIpsum));
    state.SetComplexityN(lorem_ipsum_count);
}

namespace
{

auto bm_pattern_comparator(){ return [](char c, char p){ return '?' == p || c == p; }; }

} // anonymous namespace

template<typename SearcherT>
void BM_Searcher_WithComp_LoremIpsum(State& state)
{
    auto const lorem_ipsum_count = state.range(0);

    std::string const pattern = "?aciti";
    SearcherT searcher(std::move(pattern), bm_pattern_comparator());

    for (auto _ : state)
    {
        for (size_t i = 0; i < lorem_ipsum_count; ++i)
        {
            auto token = searcher(kLoremIpsum);
            DoNotOptimize(token);
        }
    }

    state.SetItemsProcessed(state.iterations() * lorem_ipsum_count);
    state.SetBytesProcessed(state.items_processed() * sizeof(kLoremIpsum));
    state.SetComplexityN(lorem_ipsum_count);
}

BENCHMARK_TEMPLATE(BM_RangeSplitter_Lines, RangeSplitter<std::string::const_iterator>)
    ->RangeMultiplier(10)->Range(1000, 10000000)
    ->Unit(kMillisecond)
    ->Complexity(oN);

BENCHMARK_TEMPLATE(BM_StreamSplitter_Lines, StreamSplitter)
    ->RangeMultiplier(10)->Range(1000, 10000000)
    ->Unit(kMillisecond)
    ->Complexity(oN);

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, NaiveSearcher<std::string>)
    ->RangeMultiplier(10)->Range(1000, 1000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, BoyerMooreSearcher<std::string>)
    ->RangeMultiplier(10)->Range(1000, 1000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, BoyerMooreSearcher<std::string, searchers::Boosted>)
    ->RangeMultiplier(10)->Range(1000, 1000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_WithComp_LoremIpsum, NaiveSearcher<std::string, decltype(bm_pattern_comparator())>)
    ->RangeMultiplier(10)->Range(1000, 1000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_WithComp_LoremIpsum, BoyerMooreSearcher<std::string, decltype(bm_pattern_comparator())>)
    ->RangeMultiplier(10)->Range(1000, 1000000)
    ->Unit(kMillisecond)
    ->Complexity();

} // namespace mtfind::bench

BENCHMARK_MAIN();
