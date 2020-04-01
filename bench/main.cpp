#include <cstdlib>

#include <algorithm>

#include <benchmark/benchmark.h>

#include "splitters/range_splitter.hpp"

using namespace mtfind;

namespace
{

template<typename SplitterT>
void BM_RangeSplitter_Lines(benchmark::State& state) {

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
            benchmark::DoNotOptimize(line);
    }

    state.SetItemsProcessed(state.iterations() * lines_number);
    state.SetBytesProcessed(state.iterations() * text.size());
    state.SetComplexityN(lines_number);
}
BENCHMARK_TEMPLATE(BM_RangeSplitter_Lines, RangeSplitter<std::string::const_iterator>)
    ->RangeMultiplier(10)->Range(1000, 10000000)
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

} // anonymous namespace

BENCHMARK_MAIN();
