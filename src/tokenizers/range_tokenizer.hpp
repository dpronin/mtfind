#pragma once

#include <iterator>
#include <utility>

namespace mtfind
{

///
/// @brief      A tokenizer that uses a searcher given to
///             find and extract all matches from the source range
///
/// @tparam     Searcher      A searcher type, a functor-like class with two iterators as parameters
///
template <typename Searcher>
class RangeTokenizer
{
public:
    explicit RangeTokenizer(Searcher searcher) noexcept
        : searcher_(searcher)
    {
    }

    ///
    /// @brief      Tokenizing source range represented by iterators,
    ///             results are pushed to an output iterator
    ///
    /// @param[in]  first      The start of the range to parse
    /// @param[in]  last       The end of the range to parse
    /// @param[in]  out        The output iterator to store token results from the searcher
    ///
    /// @tparam     RangeIt    Range iterator
    /// @tparam     OutputIt   Output iterator
    ///
    template <typename RangeIt, typename OutputIt>
    void operator()(RangeIt first, RangeIt last, OutputIt out)
    {
        for (; first != last; ++out)
        {
            auto token = searcher_(first, last);
            if (token.empty())
                break;
            first = token.end();
            *out  = std::move(token);
        }
    }

    ///
    /// @brief      Searches for a pattern in the range composing the set of findings
    ///
    /// @param[in]  range   The input range to parse
    /// @param[in]  out     The output iterator to store token results
    ///
    /// @tparam     Range
    /// @tparam     OutputIt
    ///
    template <typename Range, typename OutputIt>
    void operator()(Range const &range, OutputIt out)
    {
        return (*this)(std::begin(range), std::end(range), out);
    }

private:
    Searcher searcher_;
};

} // namespace mtfind
