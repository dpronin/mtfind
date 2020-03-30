#pragma once

#include <utility>
#include <iterator>

namespace mtfind
{

///
/// @brief      Default character range parser that uses a searcher given to
///             find and extract all matches from the source range
///
/// @tparam     Searcher type, a functor-like class with two iterators as parameters
///
template<typename Searcher>
class RangeTokenizer
{
public:
    explicit RangeTokenizer(Searcher searcher) noexcept : searcher_(searcher)
    {}

    ///
    /// @brief      Tokenize source range represented by iterators,
    ///             results are pushed to an output iterator
    ///
    /// @param[in]  first      The start of the range
    /// @param[in]  last       The end of the range
    /// @param[in]  last       The end of the range
    ///
    /// @tparam     InputIt    Input iterator
    /// @tparam     OutputIt   Output iterator
    ///
    /// @return     An array of decoupled source's peaces those the searcher has fired for
    ///
    template<typename InputIt, typename OutputIt>
    void operator()(InputIt first, InputIt last, OutputIt out)
    {
        for (; first != last; ++out)
        {
            auto token = searcher_(first, last);
            if (token.empty())
                break;
            first = token.end();
            *out = std::move(token);
        }
    }

    ///
    /// @brief      Searches for a pattern in the range composing the set of findings
    ///
    /// @param[in]  range   The input range to parse
    /// @param[in]  out     The output iterator to store tokens
    ///
    /// @tparam     Range
    ///
    /// @return     An array of decoupled source's peaces those the searcher has fired for
    ///
    template <typename Range, typename OutputIt>
    void operator()(Range const &range, OutputIt out) { return (*this)(std::begin(range), std::end(range), out); }

private:
    Searcher searcher_;
};

} // namespace mtfind
