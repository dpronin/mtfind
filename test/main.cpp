#include <cstddef>
#include <cstdlib>

#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/range/iterator_range.hpp>
#include <boost/range/numeric.hpp>

#include "processors/multithreaded_task_processor.hpp"
#include "processors/threaded_chunk_processor.hpp"
#include "searchers/boyer_moore_searcher.hpp"
#include "searchers/naive_searcher.hpp"
#include "splitters/range_splitter.hpp"
#include "splitters/stream_splitter.hpp"
#include "tokenizers/range_tokenizer.hpp"

#include "strat/divide_and_conquer.hpp"
#include "strat/round_robin.hpp"

#include "aux/chunk.hpp"

#include "lorem_ipsum.hpp"

using namespace testing;

namespace mtfind::test
{

template <typename T>
struct RangeSplitterTest : public Test
{
};

using RangeSplitters = Types<RangeSplitter<std::string::const_iterator>>;
TYPED_TEST_SUITE(RangeSplitterTest, RangeSplitters);

TYPED_TEST(RangeSplitterTest, SplitsStringInLines)
{
    using SplitterT = TypeParam;

    std::string const              text           = "line1\nline2\n\nline4\r\nline5\n";
    std::vector<std::string> const expected_lines = {"line1", "line2", "", "line4\r", "line5"};

    SplitterT line_splitter(text, '\n');

    std::vector<std::string> result_lines;
    result_lines.reserve(expected_lines.size());
    for (auto line = line_splitter(); line_splitter; line = line_splitter())
        result_lines.push_back({line.begin(), line.end()});

    EXPECT_EQ(expected_lines, result_lines);
}

TYPED_TEST(RangeSplitterTest, SplitsStringAtWhitespaces)
{
    using SplitterT = TypeParam;

    std::string const              text           = "Hello, my lo\tvely wor\nld!";
    std::vector<std::string> const expected_words = {"Hello,", "my", "lo\tvely", "wor\nld!"};

    SplitterT wsp_word_splitter(text, ' ');

    std::vector<std::string> result_words;
    result_words.reserve(expected_words.size());
    for (auto word = wsp_word_splitter(); wsp_word_splitter; word = wsp_word_splitter())
        result_words.push_back({word.begin(), word.end()});

    EXPECT_EQ(expected_words, result_words);
}

template <typename T>
struct StreamSplitterTest : public Test
{
};

using StreamSplitters = Types<StreamSplitter<char>>;
TYPED_TEST_SUITE(StreamSplitterTest, StreamSplitters);

TYPED_TEST(StreamSplitterTest, SplitsStringStreamInLines)
{
    using SplitterT = TypeParam;

    std::istringstream iss("line1\nline2\n\nline4\r\nline5\n");
    iss >> std::noskipws;
    std::vector<std::string> const expected_lines = {"line1", "line2", "", "line4\r", "line5"};

    SplitterT line_splitter(iss, '\n');

    std::vector<std::string> result_lines;
    for (auto line = line_splitter(); line_splitter; line = line_splitter())
        result_lines.push_back({line.begin(), line.end()});

    EXPECT_EQ(expected_lines, result_lines);
}

TYPED_TEST(StreamSplitterTest, SplitsStringStreamAtWhitespaces)
{
    using SplitterT = TypeParam;

    std::istringstream iss("Hello, my lo\tvely wor\nld!");
    iss >> std::noskipws;
    std::vector<std::string> const expected_words = {"Hello,", "my", "lo\tvely", "wor\nld!"};

    SplitterT wsp_word_splitter(iss, ' ');

    std::vector<std::string> result_words;
    for (auto word = wsp_word_splitter(); wsp_word_splitter; word = wsp_word_splitter())
        result_words.push_back({word.begin(), word.end()});

    EXPECT_EQ(expected_words, result_words);
}

template <typename T>
struct Searcher : public Test
{
};

using Searchers = Types<
    NaiveSearcher<std::string>,
    BoyerMooreSearcher<std::string>,
    BoyerMooreSearcher<std::string, searchers::Boosted>>;
TYPED_TEST_SUITE(Searcher, Searchers);

TYPED_TEST(Searcher, SuccessfulPatternLookupNoComparator)
{
    using SearcherT = TypeParam;

    // clang-format off
    std::vector<std::tuple<std::string, std::string, size_t>> const records =
    {
        /*          input text          */ /* pattern to look up */ /* finding's start position */
        { "Look up a pattern in this text", "pattern",                10                         },
        { "Find\n\t\tme\nhere!",            "me",                     7                          },
        { "uuuuuu uuuuuuuuuuut",            "t",                      18                         },
        { "abcbeafcb",                      "afcb",                   5                          },
    };
    // clang-format on

    for (auto const &record : records)
    {
        auto const &text    = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        SearcherT   searcher(pattern);
        auto const  token = searcher(text.cbegin(), text.cend());
        EXPECT_THAT(token, ElementsAreArray(pattern));
        EXPECT_EQ(std::distance(text.cbegin(), token.begin()), std::get<2>(record));
    }
}

TYPED_TEST(Searcher, FailedPatternLookupNoComparator)
{
    using SearcherT = TypeParam;

    // clang-format off
    std::vector<std::pair<std::string, std::string>> const records =
    {
        /*          input text          */ /* pattern to look up */
        { "Look up a pattern in this text","unfound",             },
        { "Find\n\t\tme\nhere!",           "\r",                  },
        { "uuuuuu uuuuuuuuuuuj",           "m",                   },
        { "abcbeafeb",                     "afcb",                },
        { "abc",                           "abcdef"               }
    };
    // clang-format on

    for (auto const &record : records)
    {
        auto const &text    = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        SearcherT   searcher(pattern);
        auto const  token = searcher(text.cbegin(), text.cend());
        EXPECT_TRUE(token.empty());
        EXPECT_EQ(token.begin(), text.cend());
    }
}

template <typename T>
struct ComparatoredSearcher : public Test
{
};

using ComparatoredSearchers = Types<
    NaiveSearcher<std::string, std::function<bool(char, char)>>,
    BoyerMooreSearcher<std::string, std::function<bool(char, char)>>>;
TYPED_TEST_SUITE(ComparatoredSearcher, ComparatoredSearchers);

TYPED_TEST(ComparatoredSearcher, SuccessfulPatternLookupWithComparator)
{
    using SearcherT = TypeParam;

    using comparator_t = std::function<bool(char, char)>;
    // clang-format off
    std::vector<comparator_t> const comparators = {
        [](auto c, auto p){ return '?' == p || p == c; },                         // the pattern comparator used in the application
        [](auto c, auto p){ return ('!' == p && 'e' == c) || '?' == p || p == c; }, // an arbitrary pattern comparator
        [](auto c, auto p){ return ('&' == p && ('u' - c) == 1) || p == c; },     // an arbitrary pattern comparator
    };
    // clang-format on

    using token_pos_t = std::pair<std::string, size_t>;

    // clang-format off
    std::vector<std::tuple<std::string, std::string, comparator_t, std::vector<token_pos_t>>> const records =
    {
        /*          input text          */ /* pattern to look up */ /* comparator */  /*        findings and their start positions      */
        { "Look up a pattern in this text", "a??",                   comparators[0],   { { "a p",   8 }, { "att", 11 }                } },
        { "Find\n\t\tme\nhere!",            "!?",                    comparators[1],   { { "e\n",   8 }, { "er",  11 }, { "e!", 13  } } },
        { "uuuuuu uuuuuuuuuuut",            "uuu&",                  comparators[2],   { { "uuut", 15 }                               } },
        { "\xFF""\xFE""\x80""\x81""good",   "?ood",                  comparators[0],   { { "good",  4 }                               } }
    };
    // clang-format on

    for (auto const &record : records)
    {
        auto const &text    = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        SearcherT   searcher(pattern, std::get<2>(record));
        auto const &exp_token_pos_array = std::get<3>(record);
        auto        exp_token_pos       = exp_token_pos_array.cbegin();
        auto        token               = searcher(text);
        for (; text.cend() != token.begin() && exp_token_pos_array.cend() != exp_token_pos;
             ++exp_token_pos, token = searcher(token.end(), text.cend()))
        {
            ASSERT_THAT(token, ElementsAreArray(exp_token_pos->first));
            ASSERT_EQ(std::distance(text.cbegin(), token.begin()), exp_token_pos->second);
        }
        EXPECT_EQ(exp_token_pos, exp_token_pos_array.cend());
        EXPECT_TRUE(token.empty());
        EXPECT_EQ(token.begin(), text.cend());
    }
}

TYPED_TEST(ComparatoredSearcher, FailedPatternLookupWithComparator)
{
    using SearcherT = TypeParam;

    using comparator_t = std::function<bool(char, char)>;

    // clang-format off
    std::vector<comparator_t> const comparators = {
        [](auto c, auto p){ return false; },                                        // always false binary function
        [](auto c, auto p){ return 'A' <= c && c <= 'Z' && 'a' <= p && p <= 'z'; }, // an arbitrary pattern comparator
        [](auto c, auto p){ return 'u' == p && 'u' != c; },                         // an arbitrary pattern comparator
        [](auto c, auto p){ return '?' == p || p == c; },                           // the pattern comparator used in the application
    };
    // clang-format on

    // clang-format off
    std::vector<std::tuple<std::string, std::string, comparator_t>> const records =
    {
        /*          input text          */ /* pattern to look up */  /* comparator */
        { "No matter what text is here",    "no_matter?",             comparators[0] },
        { "Find\n\t\tme\nhere!",            "Find",                   comparators[1] },
        { "uuuuuu uuuuuuuuuuut",            "uuu&",                   comparators[2] },
        { "\xFF""\xFE""\x80""\x81""good",   "g?ud",                   comparators[3] },
        { "abc",                            "?b?def",                 comparators[3] }
    };
    // clang-format on

    for (auto const &record : records)
    {
        auto const &text    = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        SearcherT   searcher(pattern, std::get<2>(record));
        auto const  token = searcher(text);
        EXPECT_TRUE(token.empty());
        EXPECT_EQ(token.begin(), text.cend());
    }
}

struct SearcherMock
{
    using iterator = std::string::const_iterator;
    using token_t  = boost::iterator_range<iterator>;

    MOCK_METHOD(token_t, search, (iterator, iterator));
    auto operator()(iterator first, iterator last) { return search(first, last); }
};

template <typename T>
struct Tokenizer : public Test
{
};

using Tokenizers = Types<
    RangeTokenizer<std::function<boost::iterator_range<std::string::const_iterator>(
        std::string::const_iterator, std::string::const_iterator)>>>;
TYPED_TEST_SUITE(Tokenizer, Tokenizers);

TYPED_TEST(Tokenizer, Tokenizes)
{
    using TokenizerT = TypeParam;

    std::string const text = "London is the capital of Great Britain indeed";

    // clang-format off
    // token and its position in the input text
    std::vector<std::pair<std::string, size_t>> const exp_tokens =
    {
        {"London",   0},
        {"Great",   25},
        {"Britain", 31}
    };
    // clang-format on

    // searcher performs lookups of the words separated by whitespaces ' ' and starting from an upper letter
    // must be called exactly expected tokens size + 1 because the last word in the text
    // does not start from an upper letter, hence the searcher will be called one more time
    // than number of words found
    SearcherMock searcher;
    EXPECT_CALL(searcher, search(_, _)).Times(Exactly(exp_tokens.size() + 1)).WillRepeatedly(Invoke([](auto first, auto last)
                                                                                                    {
        // searching for words starting from an upper letter
        first = std::find_if(first, last, [](auto c) { return 'A' <= c && c <= 'Z'; });
        return std::make_pair(first, std::find(first, last, ' ')); }));

    // searcher wrapper is essential because the searcher itself is uncopiable
    auto searcher_wrapper = [&searcher](auto &&...args)
    { return searcher(std::forward<decltype(args)>(args)...); };

    TokenizerT tokenizer(searcher_wrapper);

    std::vector<boost::iterator_range<std::string::const_iterator>> tokens;
    tokenizer(text, std::back_inserter(tokens));

    EXPECT_EQ(tokens.size(), exp_tokens.size());

    auto token_exp = exp_tokens.cbegin();
    for (auto token = tokens.cbegin(); token != tokens.cend(); ++token, ++token_exp)
    {
        EXPECT_THAT(*token, ElementsAreArray(token_exp->first));
        EXPECT_EQ(std::distance(text.cbegin(), token->begin()), token_exp->second);
    }
}

TYPED_TEST(Tokenizer, ReturnsEmptyCollection)
{
    std::string const text = "London is the capital of Great Britain indeed";
    SearcherMock      searcher;

    // searcher returns that nothing has been found, must be called exactly once
    EXPECT_CALL(searcher, search(_, _)).WillOnce(Invoke([](auto first, auto last)
                                                        {
        // searching for words starting from an upper letter
        return std::make_pair(last, last); }));

    // searcher wrapper is essential because the searcher itself is uncopiable
    auto searcher_wrapper = [&searcher](auto &&...args)
    { return searcher(std::forward<decltype(args)>(args)...); };

    RangeTokenizer<decltype(searcher_wrapper)> tokenizer(searcher_wrapper);

    std::vector<boost::iterator_range<std::string::const_iterator>> tokens;
    tokenizer(text, std::back_inserter(tokens));
    EXPECT_TRUE(tokens.empty());
}

struct TaskMock
{
    MOCK_METHOD(void, exec, ());
    void operator()() { exec(); }
};

TEST(MultithreadedTaskProcessors, HandlesTasksExpectedTimes)
{
    size_t constexpr kCallsCount = 100;
    MultithreadedTaskProcessor processor(std::thread::hardware_concurrency());

    TaskMock task;
    EXPECT_CALL(task, exec()).Times(Exactly(kCallsCount));

    processor.run();
    for (size_t i = 0; i < kCallsCount; ++i)
        processor(std::ref(task));
    processor.wait();
}

TEST(MultithreadedTaskProcessors, DoesNotHandleTaskIfNotRunning)
{
    MultithreadedTaskProcessor processor;

    TaskMock task;
    EXPECT_CALL(task, exec()).Times(0);

    processor(std::ref(task));
    processor.wait();
}

TEST(ThreadedChunkProcessor, HandlesTasksAsChunksExpectedTimes)
{
    size_t constexpr kCallsCount = 100;

    TaskMock task;
    EXPECT_CALL(task, exec()).Times(Exactly(kCallsCount));

    std::function<void()> caller = [&task]
    { task(); };
    auto handler = [](std::function<void()> &&caller)
    { caller(); };

    ThreadedChunkProcessor<decltype(handler), std::function<void()>> processor(handler);

    processor.start();
    for (size_t i = 0; i < kCallsCount; ++i)
        processor(caller);
    processor.stop();
}

TEST(ThreadedChunkProcessor, DoesNotHandleTaskAsChunkIfNotRunning)
{
    TaskMock task;
    EXPECT_CALL(task, exec()).Times(0);

    std::function<void()> caller = [&task]
    { task(); };
    auto handler = [](auto &&caller)
    { caller(); };

    ThreadedChunkProcessor<decltype(handler), std::function<void()>> processor(handler);

    processor(caller);
}

template <typename ValueType>
class FindingsSink
{
public:
    using Container = detail::ChunksFindings<ValueType>;

    void operator()(typename Container::value_type finding) { findings_.push_back(std::move(finding)); }
    void operator()(size_t findings_number_received) noexcept { findings_number_received_ = findings_number_received; }

    auto const &findings() const noexcept { return findings_; }
    size_t      findings_number_received() const noexcept { return findings_number_received_; }

private:
    Container findings_;
    size_t    findings_number_received_ = 0;
};

class ParseLoremIpsum : public Test
{
public:
    ParseLoremIpsum()
        : searcher_(kPattern_), tokenizer_(searcher_)
    {
    }
    ~ParseLoremIpsum() override = default;

protected:
    template <typename T>
    void validate(FindingsSink<T> const &sink)
    {
        auto const &findings = sink.findings();
        EXPECT_EQ(sink.findings_number_received(), findings.size());
        auto exp_line_it = kExpLinesFindings_.cbegin();
        for (auto const &finding : findings)
        {
            EXPECT_EQ(std::get<0>(finding), exp_line_it->first);
            EXPECT_EQ(std::get<1>(finding), exp_line_it->second);
            EXPECT_THAT(std::get<2>(finding), ElementsAreArray(kPattern_));
            ++exp_line_it;
        }
        EXPECT_EQ(exp_line_it, kExpLinesFindings_.cend());
    }

protected:
    std::string const kPattern_ = "vitae";
    // clang-format off
    std::vector<std::pair<size_t, size_t>> const kExpLinesFindings_ = {
        { 5 ,  21  },
        { 6 ,  84  },
        { 10,   8  },
        { 11,  28, },
        { 11, 103, },
        { 12,  42  },
        { 17,  32  },
        { 19,  82  },
        { 32,  48  },
        { 33,  63  }
    };
    // clang-format on

    BoyerMooreSearcher<decltype(kPattern_)> searcher_;
    RangeTokenizer<decltype(searcher_)>     tokenizer_;
};

TEST_F(ParseLoremIpsum, RoundRobinWithRandomAccessRangeLoremIpsum)
{
    RangeSplitter<decltype(std::begin(kLoremIpsum))>  line_splitter(kLoremIpsum, '\n');
    FindingsSink<boost::iterator_range<const char *>> sink;
    ASSERT_EQ(strat::round_robin(line_splitter, tokenizer_, std::ref(sink), std::ref(sink), std::thread::hardware_concurrency()), EXIT_SUCCESS);
    validate(sink);
}

TEST_F(ParseLoremIpsum, RoundRobinWithStreamedAccessLoremIpsum)
{
    std::istringstream iss(kLoremIpsum);
    iss >> std::noskipws;
    StreamSplitter<char>      line_splitter(iss, '\n');
    FindingsSink<std::string> sink;
    ASSERT_EQ(strat::round_robin(line_splitter, tokenizer_, std::ref(sink), std::ref(sink), std::thread::hardware_concurrency()), EXIT_SUCCESS);
    validate(sink);
}

TEST_F(ParseLoremIpsum, DivideAndConquer)
{
    FindingsSink<boost::iterator_range<const char *>> sink;
    ASSERT_EQ(strat::divide_and_conquer(kLoremIpsum, tokenizer_, std::ref(sink), std::ref(sink), '\n', std::thread::hardware_concurrency()), EXIT_SUCCESS);
    validate(sink);
}

} // namespace mtfind::test

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
