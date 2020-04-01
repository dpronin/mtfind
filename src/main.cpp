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
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include "splitters/stream_splitter.hpp"
#include "splitters/range_splitter.hpp"
#include "searchers/naive_searcher.hpp"
#include "searchers/boyer_moore_searcher.hpp"
#include "tokenizers/range_tokenizer.hpp"

#include "strat/round_robin.hpp"
#include "strat/divide_and_conquer.hpp"

#include "application.hpp"

using namespace mtfind;

namespace
{

template<typename...Args>
int streamed_run(std::istream &is, Args&&...args)
{
    is >> std::noskipws;
    return strat::round_robin(StreamSplitter(is, '\n'), std::forward<Args>(args)...);
}

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
    if (!use_stdin(input_path))
    {
        boost::filesystem::path const input_file_path(input_path.data());

        // check if the input file exists
        if (!boost::filesystem::exists(input_file_path))
        {
            std::cerr << "error: input file " << input_file_path << " doesn't exist\n";
            return EXIT_FAILURE;
        }

        // check if the input file is a regular file, not a directory, socket, block device, etc.
        if (!boost::filesystem::is_regular_file(input_file_path))
        {
            std::cerr << "error: input file " << input_file_path << " is not regular\n";
            return EXIT_FAILURE;
        }

        // check if the input file is empty, if it is, no processing is required
        if (boost::filesystem::is_empty(input_file_path))
        {
            std::cerr << "input file " << input_file_path << " is empty\n";
            return EXIT_SUCCESS;
        }

        boost::iostreams::mapped_file_source mmap_source_file;
        try
        {
            // map input file provided by path input_file_path, opening performed is in readonly mode
            mmap_source_file.open(input_file_path);
        }
        catch (std::exception const &ex)
        {
            std::cerr << "WARNING: mapping file " << input_file_path << " failed\n";
        }

        if (!mmap_source_file)
        {
            std::cerr << "WARNING: coudn't map input file " << input_file_path << " to memory\n"
                      << "WARNING: falling back to the default slow stream-oriented reading mode\n";

            boost::filesystem::ifstream stream_source_file{input_file_path};
            if (!stream_source_file)
            {
                std::cerr << "opening file " << input_file_path << " in stream-mode failed\n";
                return EXIT_FAILURE;
            }

            return streamed_run(stream_source_file, tokenizer, line_findings_sink);
        }
        else
        {
            // @info for debug purposes there has been used RR for a mapped file
            // actually it works a little bit slower than deviding and conquering in case of having SSD
            // strat::round_robin(RangeSplitter<decltype(mmap_source_file.begin())>(mmap_source_file, '\n'), tokenizer, line_findings_sink)
            return strat::divide_and_conquer(mmap_source_file, tokenizer, line_findings_sink);
        }
    }
    else
    {
        // synchronization cin with scanf-like functions is disabled
        // it can be done since no scanf-like functions are used in the application
        // it could slightly increase performance
        std::cin.sync_with_stdio(false);
        return streamed_run(std::cin, tokenizer, line_findings_sink);
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

    if (argc > 3)
    {
        for (int i = 3; i < argc; ++i)
            std::cerr << "WARNING: redundant parameter '" << argv[i] << "' provided, skipped\n";
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
