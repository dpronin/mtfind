#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/iterator_range.hpp>

#include "aux/iterator_concept.hpp"

namespace mtfind::detail
{

template<typename Iterator, typename = void>
class RangeSplitter
{};

template<typename Iterator>
class RangeSplitter<Iterator,
    typename std::enable_if<is_conv_to_forward_char_iterator<Iterator>>::type>
{
public:
    using iterator       = Iterator;
    using const_iterator = Iterator;

    template<typename Range>
    explicit RangeSplitter(Range const &source_range, char delim = '\n')
        : RangeSplitter(source_range.begin(), source_range.end(), delim)
    {}

    explicit RangeSplitter(Iterator first, Iterator last, char delim = '\n')
        : first_(first)
        , last_(last)
        , current_pos_(first_)
        , delim_(delim)
    {}

    auto operator()() noexcept
    {
        boost::iterator_range<Iterator> token;
        eorange_ = last_ == current_pos_;
        if (!eorange_)
        {
            auto end_of_part_pos = std::find(current_pos_, last_, delim_);
            token = boost::make_iterator_range(current_pos_, end_of_part_pos);
            current_pos_ = last_ != end_of_part_pos ? std::next(end_of_part_pos) : end_of_part_pos;
        }
        return token;
    }

    operator bool() const noexcept { return !eorange(); }
    bool operator!() const noexcept { return eorange(); }

    auto current_pos() const noexcept { return current_pos_; }

    auto bytes_left() const noexcept { return std::distance(current_pos_, last_); }
    auto size()       const noexcept { return std::distance(first_, last_); }

    bool eorange() const noexcept { return eorange_; }

    void reset() noexcept { current_pos_ = first_; }

private:
    Iterator first_;
    Iterator last_;
    Iterator current_pos_;
    char     delim_;
    bool     eorange_ = false;
};

template<typename Iterator>
class RangeSplitter<Iterator,
    typename std::enable_if<!is_conv_to_forward_char_iterator<Iterator> && is_conv_to_input_char_iterator<Iterator>>::type>
{
public:
    using iterator       = Iterator;
    using const_iterator = Iterator;

    explicit RangeSplitter(Iterator first, Iterator last, char delim = '\n')
        : first_(first)
        , last_(last)
        , delim_(delim)
    {}

    auto operator()() noexcept
    {
        std::string token;
        eorange_ = last_ == first_;
        if (!eorange_)
        {
            while (first_ != last_ && *first_ != delim_)
                token.push_back(*first_++);
            if (first_ != last_)
                ++first_;
        }
        return token;
    }

    operator bool() const noexcept { return !eorange(); }
    bool operator!() const noexcept { return eorange(); }

    bool eorange() const noexcept { return eorange_; }

private:
    Iterator first_;
    Iterator last_;
    char     delim_;
    bool     eorange_ = false;
};

} // namespace mtfind::aux
