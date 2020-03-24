#pragma once

#include <algorithm>

#include <utility>
#include <type_traits>

#include <boost/utility/string_view.hpp>

namespace mtfind
{

///
/// @brief      Searcher class that uses a pattern and a comparator given
///             to search a substring in the input string-like range
///
/// @tparam     Comparator
///
template<typename Comparator = void>
class Searcher
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern and a comparator given
    ///
    /// @param[in]  pattern  The pattern that input ranges will be compared with
    /// @param[in]  comp     The comparator
    ///
    explicit Searcher(boost::string_view pattern, Comparator comp = Comparator()) noexcept : pattern_(pattern), comp_(std::move(comp))
    {}

    ///
    /// @brief      Matches the input string-like range against the pattern
    ///
    /// @param[in]  first   The first iterator
    /// @param[in]  last    The last iterator
    ///
    /// @tparam     Iterator
    ///
    /// @return     A pair of iterators specifying the range that meets the pattern
    ///             Comparison is performed with the comparator
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last) const noexcept
    {
        std::pair<Iterator, Iterator> match;
        match.first  = std::search(first, last, pattern_.begin(), pattern_.end(), comp_);
        match.second = last != match.first ? std::next(match.first, pattern_.size()) : last;
        return match;
    }

    ///
    /// @brief      Matches the input string against the pattern
    ///
    /// @param[in]  input   The string
    ///
    /// @return     The result of the function call
    ///
    auto operator()(boost::string_view input) const noexcept { return (*this)(input.begin(), input.end()); }

private:
    boost::string_view pattern_;
    Comparator         comp_;
};

///
/// @brief      Searcher class that uses a pattern given
///             to search a substring in the input string-like range
///
/// @tparam     Comparator
///
template<>
class Searcher<void>
{
public:
    ///
    /// @brief      Constructs the searcher based on the pattern
    ///
    /// @param[in]  pattern  The pattern
    ///
    explicit Searcher(boost::string_view pattern) noexcept : pattern_(pattern)
    {}

    ///
    /// @brief      Matches the input string-like range against the pattern
    ///
    /// @param[in]  first   The first iterator
    /// @param[in]  last    The last iterator
    ///
    /// @tparam     Iterator
    ///
    /// @return     A pair of iterators specifying the range that meets the pattern
    ///
    template<typename Iterator>
    auto operator()(Iterator first, Iterator last) const noexcept
    {
        std::pair<Iterator, Iterator> match;
        match.first  = std::search(first, last, pattern_.begin(), pattern_.end());
        match.second = last == match.first ? last : std::next(match.first, pattern_.size());
        return match;
    }

    ///
    /// @brief      Matches the input string against the pattern
    ///
    /// @param[in]  input   The string
    ///
    /// @return     The result of the function call
    ///
    auto operator()(boost::string_view input) const noexcept { return (*this)(input.begin(), input.end()); }

private:
    boost::string_view pattern_;
};

} // namespace mtfind
