#include <cstdlib>

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <random>

#include <benchmark/benchmark.h>

#include "splitters/range_splitter.hpp"
#include "splitters/stream_splitter.hpp"

#include "searchers/boyer_moore_searcher.hpp"
#include "searchers/naive_searcher.hpp"

using namespace benchmark;

namespace mtfind::bench
{

template <typename SplitterT>
void BM_RangeSplitter_Lines(State &state)
{
    std::string const line = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";

    auto const lines_number = state.range(0);

    std::string text;
    text.reserve((line.size() + 1) * lines_number);

    for (size_t i = 0; i < lines_number; ++i)
        text += line;

    for (auto _ : state)
    {
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

template <typename SplitterT>
void BM_StreamSplitter_Lines(State &state)
{
    std::string const line = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";

    auto const lines_number = state.range(0);

    std::string text;
    text.reserve((line.size() + 1) * lines_number);

    for (size_t i = 0; i < lines_number; ++i)
        text += line;

    for (auto _ : state)
    {
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

namespace
{
    auto bm_generate_text(size_t symbols_count)
    {
        std::string text;

        std::random_device rdev;
        std::default_random_engine gen{rdev()};
        std::uniform_int_distribution<char> dist(0, 127);

        text.resize(symbols_count);
        std::generate(text.begin(), text.end(), [&]{ return dist(gen); });

        return text;
    }

    auto bm_pattern_comparator() { return [](char c, char p) { return '?' == p || c == p; }; }
} // anonymous namespace

template <typename SearcherT>
void BM_Searcher_NoComp_LoremIpsum(State &state)
{
    size_t symbols_count = state.range(0);

    std::string pattern(10, '\0');
    std::string const text = bm_generate_text(symbols_count);
    std::copy_n(text.rbegin(), std::min(text.size(), pattern.size()), pattern.rbegin());
    SearcherT searcher(pattern);

    for (auto _ : state)
    {
        auto token = searcher(text);
        DoNotOptimize(token);
    }

    state.SetBytesProcessed(state.iterations() * symbols_count);
    state.SetComplexityN(symbols_count);
}

template <typename SearcherT>
void BM_Searcher_WithComp_LoremIpsum(State &state)
{
    size_t symbols_count = state.range(0);

    std::string pattern(10, '?');
    std::string const text = bm_generate_text(state.range(0));
    std::copy_n(text.rbegin(), std::min(text.size(), pattern.size() - 1), pattern.rbegin());
    SearcherT searcher(pattern, bm_pattern_comparator());

    for (auto _ : state)
    {
        auto token = searcher(text);
        DoNotOptimize(token);
    }

    state.SetBytesProcessed(state.iterations() * symbols_count);
    state.SetComplexityN(symbols_count);
}

BENCHMARK_TEMPLATE(BM_RangeSplitter_Lines, RangeSplitter<std::string::const_iterator>)
    ->RangeMultiplier(10)
    ->Range(1000, 10000000)
    ->Unit(kMillisecond)
    ->Complexity(oN);

BENCHMARK_TEMPLATE(BM_StreamSplitter_Lines, StreamSplitter<char>)
    ->RangeMultiplier(10)
    ->Range(1000, 10000000)
    ->Unit(kMillisecond)
    ->Complexity(oN);

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, NaiveSearcher<std::string>)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, BoyerMooreSearcher<std::string>)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_NoComp_LoremIpsum, BoyerMooreSearcher<std::string, searchers::Boosted>)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_WithComp_LoremIpsum, NaiveSearcher<std::string, decltype(bm_pattern_comparator())>)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(kMillisecond)
    ->Complexity();

BENCHMARK_TEMPLATE(BM_Searcher_WithComp_LoremIpsum, BoyerMooreSearcher<std::string, decltype(bm_pattern_comparator())>)
    ->RangeMultiplier(10)
    ->Range(1000, 100000000)
    ->Unit(kMillisecond)
    ->Complexity();

} // namespace mtfind::bench

BENCHMARK_MAIN();
