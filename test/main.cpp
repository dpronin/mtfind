#include <cstddef>

#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/range/iterator_range.hpp>

#include "aux/range_splitter.hpp"
#include "searchers/searcher.hpp"
#include "tokenizers/range_tokenizer.hpp"

using namespace mtfind;
using namespace testing;

namespace
{

TEST(RangeSplitter, SplitsStringInLines)
{
    std::string const text = "line1\nline2\n\nline4\r\nline5\n";
    std::vector<std::string> const expected_lines = { "line1", "line2", "", "line4\r", "line5" };

    detail::RangeSplitter<std::string::const_iterator> reader(text);

    std::vector<std::string> result_lines;
    for (auto line = reader(); reader; line = reader())
        result_lines.push_back({line.begin(), line.end()});

    EXPECT_EQ(expected_lines, result_lines);
}

TEST(RangeSplitter, SplitsStringAtWhitespaces)
{
    std::string const text = "Hello, my lo\tvely wor\nld!";
    std::vector<std::string> const expected_words = { "Hello,", "my", "lo\tvely", "wor\nld!" };

    detail::RangeSplitter<std::string::const_iterator> reader(text, ' ');

    std::vector<std::string> result_words;
    for (auto word = reader(); reader; word = reader())
        result_words.push_back({word.begin(), word.end()});

    EXPECT_EQ(expected_words, result_words);
}

TEST(Searcher, SuccessfulPatternLookupNoComparator)
{
    std::vector<std::tuple<std::string, std::string, size_t>> const records =
    {
        /*          input text          */ /* pattern to look up */ /* finding's start position */
        { "Look up a pattern in this text", "pattern",                10                         },
        { "Find\n\t\tme\nhere!",            "me",                     7                          },
        { "uuuuuu uuuuuuuuuuut",            "t",                      18                         },
    };

    for (auto const &record : records)
    {
        auto const &text = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        Searcher<decltype(pattern)> searcher(pattern);
        auto const token = searcher(text.cbegin(), text.cend());
        EXPECT_THAT(token, ElementsAreArray(pattern));
        EXPECT_EQ(std::distance(text.cbegin(), token.begin()), std::get<2>(record));
    }
}

TEST(Searcher, FailedPatternLookupNoComparator)
{
    std::vector<std::pair<std::string, std::string>> const records =
    {
        /*          input text          */ /* pattern to look up */
        { "Look up a pattern in this text","unfound",             },
        { "Find\n\t\tme\nhere!",           "\r",                  },
        { "uuuuuu uuuuuuuuuuuj",           "m",                   },
    };

    for (auto const &record : records)
    {
        auto const &text = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        Searcher<decltype(pattern)> searcher(pattern);
        auto const token = searcher(text.cbegin(), text.cend());
        EXPECT_TRUE(token.empty());
        EXPECT_EQ(token.begin(), text.cend());
    }
}

TEST(Searcher, SuccessfulPatternLookupWithComparator)
{
    using comparator_t = std::function<bool(char, char)>;
    std::vector<comparator_t> const comparators = {
        [](auto c, auto p){ return '?' == p || p == c; },                         // the pattern comparator used in the application
        [](auto c, auto p){ return '!' == p && 'e' == c || '?' == p || p == c; }, // an arbitrary pattern comparator
        [](auto c, auto p){ return ('&' == p && ('u' - c) == 1) || p == c; },     // an arbitrary pattern comparator
    };

    using token_pos_t = std::pair<std::string, size_t>;

    std::vector<std::tuple<std::string, std::string, comparator_t, std::vector<token_pos_t>>> const records =
    {
        /*          input text          */ /* pattern to look up */ /* comparator */  /*        findings and their start positions      */
        { "Look up a pattern in this text", "a??",                   comparators[0],   { { "a p",   8 }, { "att", 11 }                } },
        { "Find\n\t\tme\nhere!",            "!?",                    comparators[1],   { { "e\n",   8 }, { "er",  11 }, { "e!", 13  } } },
        { "uuuuuu uuuuuuuuuuut",            "uuu&",                  comparators[2],   { { "uuut", 15 }                               } },
    };

    for (auto const &record : records)
    {
        auto const &text = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        Searcher<decltype(pattern), comparator_t> searcher(pattern, std::get<2>(record));
        auto const &exp_token_pos_array = std::get<3>(record);
        auto exp_token_pos = exp_token_pos_array.cbegin();
        auto token = searcher(text);
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

TEST(Searcher, FailedPatternLookupWithComparator)
{
    using comparator_t = std::function<bool(char, char)>;
    std::vector<comparator_t> const comparators = {
        [](auto c, auto p){ return false; },                                        // always false binary function
        [](auto c, auto p){ return 'A' <= c && c <= 'Z' && 'a' <= p && p <= 'z'; }, // an arbitrary pattern comparator
        [](auto c, auto p){ return 'u' == p && 'u' != c; },                         // an arbitrary pattern comparator
    };

    std::vector<std::tuple<std::string, std::string, comparator_t>> const records =
    {
        /*          input text          */ /* pattern to look up */ /* comparator */
        { "No matter what text is here",    "no_matter?",            comparators[0] },
        { "Find\n\t\tme\nhere!",            "Find",                  comparators[1] },
        { "uuuuuu uuuuuuuuuuut",            "uuu&",                  comparators[2] },
    };

    for (auto const &record : records)
    {
        auto const &text = std::get<0>(record);
        auto const &pattern = std::get<1>(record);
        Searcher<decltype(pattern), comparator_t> searcher(pattern, std::get<2>(record));
        auto const token = searcher(text);
        EXPECT_TRUE(token.empty());
        EXPECT_EQ(token.begin(), text.cend());
    }
}

namespace
{

struct SearcherMock
{
    using iterator = std::string::const_iterator;
    using token_t  = boost::iterator_range<iterator>;

    MOCK_METHOD2(search, token_t(iterator, iterator));

    auto operator()(iterator first, iterator last) { return search(first, last); }
};

} // anonymous namespace

TEST(RangeTokenizer, Tokenizes)
{
    std::string const text = "London is the capital of Great Britain indeed";
    // token and its position in the input text
    std::vector<std::pair<std::string, size_t>> const exp_tokens = {
        {"London",   0},
        {"Great",   25},
        {"Britain", 31}
    };

    // searcher performs lookups of the words separated by whitespaces ' '
    // must be called exactly expected tokens size + 1 because the last word in the text
    // does not start from an upper letter, hence the searcher will be called one more time
    // than number of words found
    SearcherMock searcher;
    EXPECT_CALL(searcher, search(_, _)).Times(Exactly(exp_tokens.size() + 1)).WillRepeatedly(Invoke([](auto first, auto last){
        // searching for words starting from an upper letter
        first = std::find_if(first, last, [](auto c){ return 'A' <= c && c <= 'Z'; });
        return std::make_pair(first, std::find(first, last, ' '));
    }));

    // searcher wrapper is essential because the searcher itself is uncopiable
    auto searcher_wrapper = [&searcher](auto&&...args){ return searcher(std::forward<decltype(args)>(args)...); };

    RangeTokenizer<decltype(searcher_wrapper)> tokenizer(searcher_wrapper);
    auto const tokens = tokenizer(text);

    EXPECT_EQ(tokens.size(), exp_tokens.size());

    auto token_exp = exp_tokens.cbegin();
    for (auto token = tokens.cbegin(); token != tokens.cend(); ++token, ++token_exp)
    {
        EXPECT_THAT(*token, ElementsAreArray(token_exp->first));
        EXPECT_EQ(std::distance(text.cbegin(), token->begin()), token_exp->second);
    }
}

TEST(RangeTokenizer, ReturnsEmptyCollection)
{
    std::string const text = "London is the capital of Great Britain indeed";
    SearcherMock searcher;

    // searcher returns that nothing has been found, must be called exactly once
    EXPECT_CALL(searcher, search(_, _)).WillOnce(Invoke([](auto first, auto last){
        // searching for words starting from an upper letter
        return std::make_pair(last, last);
    }));

    // searcher wrapper is essential because the searcher itself is uncopiable
    auto searcher_wrapper = [&searcher](auto&&...args){ return searcher(std::forward<decltype(args)>(args)...); };

    RangeTokenizer<decltype(searcher_wrapper)> tokenizer(searcher_wrapper);
    EXPECT_TRUE(tokenizer(text).empty());
}

} // anonymous namespace

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
