#include <cstddef>

#include <iostream>
#include <utility>
#include <stdexcept>
#include <iterator>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/utility/string_view.hpp>

#include "aux/range_splitter.hpp"
#include "searchers/searcher.hpp"
#include "tokenizers/range_tokenizer.hpp"

#include "strat/round_robin.hpp"
#include "strat/divide_and_conquer.hpp"

#include "application.hpp"

using namespace mtfind;

constexpr bool kFileUseRR = false;

///
/// @brief      main function, entry point to the program
///
/// @param[in]  argc  The count of arguments
/// @param      argv  The arguments array
///
///             argv[0] - name of the program
///             argv[1] - input file path or '-' if stdin is used
///             argv[2] - input pattern to search against
///
/// @return     result of the program, 0 when success, another value otherwise
///
int main(int argc, char const *argv[]) try
{
    std::cout.sync_with_stdio(false);
    std::cerr.sync_with_stdio(false);

    // printing the help page if no arguments given
    if (argc < 2)
    {
        Application::instance().help();
        return 0;
    }

    // there must be at most 2 meaningful arguments to the program, read the help page
    if (argc < 3)
    {
        std::cerr << "error: invalid number of parameters\n";
        Application::instance().help();
        return 1;
    }

    // preserve a pattern string given in argv[2]
    boost::string_view const pattern(argv[2]);
    // check the pattern against correctness of its content
    if (!boost::algorithm::all_of(pattern, Application::instance().pattern_validator()))
    {
        std::cerr << "error: pattern has incorrect format\n";
        Application::instance().help();
        return 1;
    }

    // the function printing out the findings for a line
    auto line_findings_sink = [](auto const &line_findings) {
        auto item_outputter = [line_no = line_findings.first](auto const &p){
            std::cout << line_no << " " << p.first << " " << p.second << '\n';
        };
        boost::copy(line_findings.second, boost::make_function_output_iterator(item_outputter));
    };

    // construct a searcher based on the pattern and the comparator dedicated to comparing
    // symbols from the pattern and the source file
    using PatternSearcher = Searcher<decltype(pattern), decltype(Application::instance().pattern_comparator())>;
    // construct a tokenizer based on the searcher finding subranges
    using Tokenizer = RangeTokenizer<PatternSearcher>;
    Tokenizer tokenizer(PatternSearcher(pattern, Application::instance().pattern_comparator()));

    boost::string_view const input(argv[1]);
    if (bool const use_stdin = input == "-")
    {
        std::cin.sync_with_stdio(false);
        std::cin >> std::noskipws;
        detail::RangeSplitter<std::istream_iterator<char>> line_reader(std::istream_iterator<char>(std::cin), std::istream_iterator<char>());
        return strat::round_robin(line_reader, tokenizer, line_findings_sink);
    }
    else
    {
        // open an input file with its path given in argv[1] by means of mapping, opening in readonly mode
        boost::iostreams::mapped_file_source const source(input.data());
        if (!source)
        {
            std::cerr << "error: couldn't open an input file '" << input << "'\n";
            return 1;
        }

        return kFileUseRR
            ? strat::round_robin(detail::RangeSplitter<decltype(source.begin())>(source), tokenizer, line_findings_sink)
            : strat::divide_and_conquer(source, tokenizer, line_findings_sink);
    }
}
catch (std::exception const &ex)
{
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
}
catch (...)
{
    std::cerr << "internal error\n";
    return 1;
}

