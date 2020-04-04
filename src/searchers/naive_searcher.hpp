#pragma once

#include <algorithm>

#include <iterator>
#include <utility>

#include <boost/range/iterator_range.hpp>

namespace mtfind
{

///
/// @brief      Searcher class that uses a pattern and a comparator given
///             to seek a subrange in an input range
///             Searching algorithm is naive
///
/// @tparam     Pattern
/// @tparam     Comparator
///
template <typename Pattern, typename Comparator = void>
class NaiveSearcher
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern and the comparator given
    ///
    /// @param[in]  pattern  The pattern that input ranges will be compared with
    /// @param[in]  comp     The comparator
    ///
    explicit NaiveSearcher(Pattern pattern, Comparator comp = Comparator()) noexcept
        : pattern_(pattern), comp_(comp)
    {
    }

    ///
    /// @brief      Finds the first occurrence of the pattern
    ///             with respect to the comparator in the input range
    ///
    /// @param[in]  first   The first forward iterator of the range to explore
    /// @param[in]  last    The last forward iterator of the range to explore
    ///
    /// @tparam     ForwardIt   Forward iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template <typename ForwardIt>
    auto operator()(ForwardIt first, ForwardIt last) const noexcept
    {
        first = std::search(first, last, pattern_.begin(), pattern_.end(), comp_);
        return boost::make_iterator_range(first, last != first ? std::next(first, pattern_.size()) : last);
    }

    ///
    /// @brief      Finds the first occurrence of the pattern
    ///             with respect to the comparator in the input range
    ///
    /// @param[in]  input   Input range to explore
    ///
    /// @tparam     Range
    ///
    /// @return     A subrange that meets the pattern
    ///
    template <typename Range>
    auto operator()(Range const &input) const noexcept
    {
        return (*this)(std::begin(input), std::end(input));
    }

private:
    Pattern    pattern_;
    Comparator comp_;
};

///
/// @brief      Searcher class that uses a pattern given to seek a subrange in an input range
///             Searching algorithm is naive
///
/// @tparam     Pattern
///
template <typename Pattern>
class NaiveSearcher<Pattern, void>
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern
    ///
    /// @param[in]  pattern  The pattern
    ///
    explicit NaiveSearcher(Pattern pattern) noexcept
        : pattern_(pattern)
    {
    }

    ///
    /// @brief      Finds the first occurrence of the pattern
    ///             with respect to the comparator in the input range
    ///
    /// @param[in]  first   The first forward iterator of the range to explore
    /// @param[in]  last    The last forward iterator of the range to explore
    ///
    /// @tparam     ForwardIt  Forward iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template <typename ForwardIt>
    auto operator()(ForwardIt first, ForwardIt last) const noexcept
    {
        first = std::search(first, last, pattern_.begin(), pattern_.end());
        return boost::make_iterator_range(first, last != first ? std::next(first, pattern_.size()) : last);
    }

    ///
    /// @brief      Finds the first occurrence of the pattern
    ///             with respect to the comparator in the input range
    ///
    /// @param[in]  input   Input range to explore
    ///
    /// @tparam     Range
    ///
    /// @return     A subrange that meets the pattern
    ///
    template <typename Range>
    auto operator()(Range const &input) const noexcept
    {
        return (*this)(std::begin(input), std::end(input));
    }

private:
    Pattern pattern_;
};

} // namespace mtfind
