#pragma once

#include <cstdint>

#include <algorithm>

#include <utility>
#include <iterator>
#include <array>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/searching/boyer_moore.hpp>

namespace mtfind
{

namespace searchers { struct Boosted {}; } // namespace searchers

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
        if (std::begin(pattern_) == std::end(pattern_))
            return boost::make_iterator_range(first, first);

        while (pattern_.size() <= std::distance(first, last))
        {
            auto end_range = std::next(first, pattern_.size());
            // need to reverse parameters of the comparator since the latter expects them in reversed order
            auto mism = std::mismatch(pattern_.rbegin(), pattern_.rend(), std::make_reverse_iterator(end_range), [&](auto p, auto c){ return comp_(c, p); });
            if (pattern_.rend() != mism.first)
            {
                auto pat_rit2 = std::next(mism.first);
                while (pattern_.rend() != pat_rit2 && !comp_(*mism.second, *pat_rit2))
                    ++pat_rit2;
                first = std::next(first, std::distance(mism.first, pat_rit2));
            }
            else
            {
                return boost::make_iterator_range(first, end_range);
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
        if (std::begin(pattern_) == std::end(pattern_))
            return boost::make_iterator_range(first, first);

        while (pattern_.size() <= std::distance(first, last))
        {
            auto end_range = std::next(first, pattern_.size());
            auto mism = std::mismatch(pattern_.rbegin(), pattern_.rend(), std::make_reverse_iterator(end_range));
            if (pattern_.rend() != mism.first)
            {
                auto const offset = std::distance(mism.first, pattern_.rend()) - 1;
                auto const pat_offset = pattern_offsets_[*mism.second];
                first = std::next(first, offset - (offset > pat_offset ? pat_offset : -1));
            }
            else
            {
                return boost::make_iterator_range(first, end_range);
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

    Pattern                             pattern_;
    std::array<int_fast32_t, kMaxChars> pattern_offsets_;
};


///
/// @brief      Searcher class that uses a pattern given to seek a subrange in an input range
///
/// @details    Searching algorithm is based on Boyer-Moore algorithm borrowed from boost library
///
/// @tparam     Pattern
///
template<typename Pattern>
class BoyerMooreSearcher<Pattern, searchers::Boosted>
{
public:
    explicit BoyerMooreSearcher(Pattern pattern) : pattern_(pattern) , searcher_(std::begin(pattern_), std::end(pattern_))
    {}

    ///
    /// @brief      Finds the first occurrence of the pattern in the input range
    ///
    /// @param[in]  first   The first random access iterator of the range to explore
    /// @param[in]  last    The last random access iterator of the range to explore
    ///
    /// @tparam     RandomAccessIterator   Random Access Iterator
    ///
    /// @return     A subrange of input's iterators that meets the pattern
    ///
    template<typename RandomAccessIterator>
    auto operator()(RandomAccessIterator first, RandomAccessIterator last) const noexcept
    {
        auto match = searcher_(first, last);
        return boost::make_iterator_range(match.first, match.second);
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
    Pattern                                                       pattern_;
    boost::algorithm::boyer_moore<decltype(std::begin(pattern_))> searcher_;
};

} // namespace mtfind
