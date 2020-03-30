#pragma once

#include <algorithm>

#include <utility>
#include <iterator>

#include <boost/range/iterator_range.hpp>

namespace mtfind
{

///
/// @brief      Searcher class that uses a pattern and a comparator given
///             to search a subrange in an input range
///
/// @tparam     Pattern
/// @tparam     Comparator
///
template<typename Pattern, typename Comparator = void>
class Searcher
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern and the comparator given
    ///
    /// @param[in]  pattern  The pattern that input ranges will be compared with
    /// @param[in]  comp     The comparator
    ///
    explicit Searcher(Pattern pattern, Comparator comp = Comparator()) noexcept : pattern_(pattern), comp_(std::move(comp))
    {}

    ///
    /// @brief      Matches the input range against the pattern
    ///
    /// @param[in]  first   The first iterator of the range
    /// @param[in]  last    The last iterator of the range
    ///
    /// @tparam     Iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last) const noexcept
    {
        first = std::search(first, last, pattern_.begin(), pattern_.end(), comp_);
        return boost::make_iterator_range(first, last != first ? std::next(first, pattern_.size()) : last);
    }

    ///
    /// @brief      Matches the input range against the pattern
    ///
    /// @param[in]  input   Input range to explore
    ///
    /// @tparam     Range
    ///
    /// @return     A subrange that meets the pattern
    ///
    template<typename Range>
    auto operator()(Range const &input) const noexcept { return (*this)(std::begin(input), std::end(input)); }

private:
    Pattern    pattern_;
    Comparator comp_;
};

///
/// @brief      Searcher class that uses a pattern given to search a subrange in an input range
///
/// @tparam     Pattern
///
template<typename Pattern>
class Searcher<Pattern, void>
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern
    ///
    /// @param[in]  pattern  The pattern
    ///
    explicit Searcher(Pattern pattern) noexcept : pattern_(pattern)
    {}

    ///
    /// @brief      Matches the input range against the pattern
    ///
    /// @param[in]  first   The first iterator of the range
    /// @param[in]  last    The last iterator of the range
    ///
    /// @tparam     Iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last) const noexcept
    {
        first = std::search(first, last, pattern_.begin(), pattern_.end());
        return boost::make_iterator_range(first, last != first ? std::next(first, pattern_.size()) : last);
    }

    ///
    /// @brief      Matches the input range against the pattern
    ///
    /// @param[in]  input   Input range to explore
    ///
    /// @tparam     Range
    ///
    /// @return     A subrange that meets the pattern
    ///
    template<typename Range>
    auto operator()(Range const &input) const noexcept { return (*this)(std::begin(input), std::end(input)); }

private:
    Pattern pattern_;
};

} // namespace mtfind
