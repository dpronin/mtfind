#pragma once

#include <cstddef>

#include <vector>
#include <utility>
#include <iterator>

#include <boost/range/iterator_range.hpp>

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
    /// @return     An array of decoupled source's peaces those the searcher has fired for
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last)
    {
        std::vector<boost::iterator_range<Iterator>> res;

        while (first != last)
        {
            auto token = searcher_(first, last);
            if (token.empty())
                break;
            first = token.end();
            res.push_back(std::move(token));
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
    /// @return     An array of decoupled source's peaces those the searcher has fired for
    ///
    template <typename Range>
    auto operator()(Range const &range) { return (*this)(std::begin(range), std::end(range)); }

private:
    Searcher searcher_;
};

} // namespace mtfind
