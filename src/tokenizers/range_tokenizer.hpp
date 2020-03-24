#pragma once

#include <cstddef>

#include <vector>
#include <tuple>
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
    /// @brief      Process source range represented by iterators
    ///
    /// @param[in]  first      The start of the range
    /// @param[in]  last       The end of the range
    ///
    /// @tparam     Iterator
    ///
    /// @return     Array of decoupled source's peaces those the searcher has fired for
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last)
    {
        std::vector<std::pair<Iterator, Iterator>> res;

        for (Iterator match_start = first, match_end; match_start != last; match_start = match_end)
        {
            std::tie(match_start, match_end) = searcher_(match_start, last);
            if (match_start != match_end)
                res.push_back({match_start, match_end});
        }

        return res;
    }

    ///
    /// @brief      Searches for a pattern in the range composing the set of findings
    ///
    /// @param[in]  range   The input range to parse
    ///
    /// @tparam     Range
    ///
    /// @return     An array of pairs of start positions and findings themselves
    template <typename Range>
    auto operator()(Range const &range) { return (*this)(std::begin(range), std::end(range)); }

private:
    Searcher searcher_;
};

} // namespace mtfind
