#pragma once

#include <cstdint>

#include <algorithm>

#include <utility>
#include <iterator>
#include <array>

#include <boost/range/iterator_range.hpp>

namespace mtfind
{

///
/// @brief      Searcher class that uses a pattern and a comparator given
///             to seek a subrange in an input range
///
/// @details    Searching algorithm is based on Boyer-Moore algorithm with
///             Bad Character Heuristic used
///
/// @tparam     Pattern
/// @tparam     Comparator
///
template<typename Pattern, typename Comparator = void>
class BoyerMooreSearcher
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern and the comparator given
    ///
    /// @param[in]  pattern  The pattern that input ranges will be compared with using the comparator
    /// @param[in]  comp     The comparator
    ///
    explicit BoyerMooreSearcher(Pattern pattern, Comparator comp = Comparator()) noexcept : pattern_(pattern), comp_(std::move(comp))
    {}

    ///
    /// @brief      Finds the first occurrence of the pattern
    ///             with respect to the comparator in the input range
    ///
    /// @param[in]  first   The first bidirectional iterator of the range to explore
    /// @param[in]  last    The last bidirectional iterator of the range to explore
    ///
    /// @tparam     BidirIterator   Bidirectional iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template<typename BidirIterator>
    auto operator()(BidirIterator first, BidirIterator last) const noexcept
    {
        while (pattern_.size() <= std::distance(first, last))
        {
            auto pat_rit = pattern_.rbegin();
            auto txt_it = std::make_reverse_iterator(std::next(first, pattern_.size()));

            for (; pattern_.rend() != pat_rit && comp_(*txt_it, *pat_rit); ++pat_rit, ++txt_it)
                ;

            if (pattern_.rend() != pat_rit)
            {
                auto pat_rit2 = std::next(pat_rit);
                while (pattern_.rend() != pat_rit2 && !comp_(*txt_it, *pat_rit2))
                    ++pat_rit2;
                first = std::next(first, std::distance(pat_rit, pat_rit2));
            }
            else
            {
                return boost::make_iterator_range(first, std::next(first, pattern_.size()));
            }
        }
        return boost::make_iterator_range(last, last);
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
    template<typename Range>
    auto operator()(Range const &input) const noexcept { return (*this)(std::begin(input), std::end(input)); }

private:
    Pattern    pattern_;
    Comparator comp_;
};

///
/// @brief      Searcher class that uses a pattern given to seek a subrange in an input range
///
/// @details    Searching algorithm is based on Boyer-Moore algorithm with
///             Bad Character Heuristic used
///
/// @tparam     Pattern
///
template<typename Pattern>
class BoyerMooreSearcher<Pattern, void>
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern
    ///
    /// @param[in]  pattern     The pattern that input ranges will be compared with
    ///
    explicit BoyerMooreSearcher(Pattern pattern) noexcept : pattern_(pattern)
    {
        pattern_offsets_.fill(-1);
        // Fill distances to the last occurrence of the characters
        for (int_fast32_t i = 0; i < pattern_.size(); i++)
            pattern_offsets_[pattern_[i]] = i;
    }

    ///
    /// @brief      Finds the first occurrence of the pattern in the input range
    ///
    /// @param[in]  first   The first bidirectional iterator of the range to explore
    /// @param[in]  last    The last bidirectional iterator of the range to explore
    ///
    /// @tparam     BidirIterator   Bidirectional iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template<typename BidirIterator>
    auto operator()(BidirIterator first, BidirIterator last) const noexcept
    {
        while (pattern_.size() <= std::distance(first, last))
        {
            auto pat_rit = pattern_.rbegin();
            auto txt_it = std::make_reverse_iterator(std::next(first, pattern_.size()));

            for (; pattern_.rend() != pat_rit && *txt_it == *pat_rit; ++pat_rit, ++txt_it)
                ;

            if (pattern_.rend() != pat_rit)
            {
                auto offset = std::distance(pat_rit, pattern_.rend()) - 1;
                offset -= offset > pattern_offsets_[*txt_it] ? pattern_offsets_[*txt_it] : -1;
                first = std::next(first, offset);
            }
            else
            {
                return boost::make_iterator_range(first, std::next(first, pattern_.size()));
            }
        }
        return boost::make_iterator_range(last, last);
    }

    ///
    /// @brief      Finds the first occurrence of the pattern in the input range
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
    static constexpr size_t  kMaxChars = 256;

    Pattern                  pattern_;
    std::array<int_fast32_t, kMaxChars> pattern_offsets_;
};

} // namespace mtfind
