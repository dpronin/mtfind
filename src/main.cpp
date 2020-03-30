#include <cstddef>
#include <cstdlib>

#include <iostream>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <functional>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/utility/string_view.hpp>

#include "aux/range_splitter.hpp"
#include "searchers/naive_searcher.hpp"
#include "searchers/boyer_moore_searcher.hpp"
#include "tokenizers/range_tokenizer.hpp"

#include "strat/round_robin.hpp"
#include "strat/divide_and_conquer.hpp"

#include "application.hpp"

using namespace mtfind;

constexpr bool kFileUseRR = false;

namespace
{

template<typename PatternSearcher>
int run(boost::string_view const input_path, PatternSearcher &&searcher)
{
    // the function printing out the findings for a line
    auto line_findings_sink = [](auto const &line_findings) {
        auto item_outputter = [line_no = line_findings.first](auto const &p){
            std::cout << line_no << " " << p.first << " " << p.second << '\n';
        };
        boost::copy(line_findings.second, boost::make_function_output_iterator(item_outputter));
    };

    // define and construct a tokenizer based on the searcher finding subranges
    // those will be extracted using the searcher constructed and based on the PatternSearcher type
    using Tokenizer = RangeTokenizer<PatternSearcher>;
    Tokenizer tokenizer(std::forward<PatternSearcher>(searcher));

    // input may be stdin specified as '-' or a path to a file
    auto use_stdin = [](auto &&input_path){ return input_path == "-"; };
    if (use_stdin(input_path))
    {
        // synchronization cin with scanf-like functions is disabled
        // it can be done since no scanf-like functions are used in the application
        // it could slightly increase performance
        std::cin.sync_with_stdio(false);
        std::cin >> std::noskipws;
        return strat::round_robin(detail::RangeSplitter<std::istream_iterator<char>>(std::istream_iterator<char>(std::cin), std::istream_iterator<char>()),
            tokenizer,
            line_findings_sink);
    }
    else
    {
        // open an input_path file with its path given in argv[1] by means of mapping, opening in readonly mode
        boost::iostreams::mapped_file_source const source_file(input_path.data());
        if (!source_file)
        {
            std::cerr << "error: couldn't open an input file '" << input_path << "'\n";
            return EXIT_FAILURE;
        }
        else
        {
            return kFileUseRR
                ? strat::round_robin(detail::RangeSplitter<decltype(source_file.begin())>(source_file), tokenizer, line_findings_sink)
                : strat::divide_and_conquer(source_file, tokenizer, line_findings_sink);
        }
    }
}

} // anonymous namespace

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
    // synchronization with printf-like function is disabled
    // since no printf-like functions are used in the application
    // as the result it could slightly increase performance
    std::cout.sync_with_stdio(false);
    std::cerr.sync_with_stdio(false);

    // printing the help page if no arguments given
    if (argc < 2)
    {
        Application::instance().help();
        return EXIT_SUCCESS;
    }

    // there must be at most 2 meaningful arguments to the program, read the help page
    if (argc < 3)
    {
        std::cerr << "error: invalid number of parameters\n";
        Application::instance().help();
        return EXIT_FAILURE;
    }

    // preserve a pattern string given in argv[2]
    boost::string_view const pattern(argv[2]);
    // check the pattern against correctness of its content by given validator
    auto pattern_validator = Application::instance().pattern_validator();
    if (!boost::algorithm::all_of(pattern, std::ref(pattern_validator)))
    {
        std::cerr << "error: pattern has incorrect format\n";
        Application::instance().help();
        return EXIT_FAILURE;
    }

    // below we define a searcher type based on the pattern [and the comparator] dedicated to comparing
    // symbols from the pattern and the source file
    if (pattern_validator.has_masked_symbols())
    {
        auto pattern_comparator = Application::instance().masked_pattern_comparator();
        return run(argv[1], BoyerMooreSearcher<decltype(pattern), decltype(pattern_comparator)>(pattern, pattern_comparator));
    }
    else
    {
        return run(argv[1], BoyerMooreSearcher<decltype(pattern)>(pattern));
    }
}
catch (std::exception const &ex)
{
    std::cerr << "error: " << ex.what() << '\n';
    return EXIT_FAILURE;
}
catch (...)
{
    std::cerr << "internal error\n";
    return EXIT_FAILURE;
}

