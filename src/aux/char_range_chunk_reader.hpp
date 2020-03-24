#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/utility/string_view.hpp>

#include "aux/iterator_concept.hpp"

namespace mtfind::detail
{

template<typename Iterator, typename = is_conv_to_forward_char_iterator<Iterator>>
class CharRangeChunkReader {
public:
    using iterator       = Iterator;
    using const_iterator = Iterator;

    template<typename Range>
    explicit CharRangeChunkReader(Range const &source_range, char delim = '\n')
        : CharRangeChunkReader(source_range.begin(), source_range.end(), delim)
    {}

    explicit CharRangeChunkReader(Iterator first, Iterator last, char delim = '\n')
        : first_(first)
        , last_(last)
        , current_pos_(first_)
        , delim_(delim)
    {}

    auto operator()() noexcept
    {
        boost::string_view line;
        eorange_ = last_ == current_pos_;
        if (!eorange_)
        {
            auto end_of_part_pos = std::find(current_pos_, last_, delim_);
            line = boost::string_view(&*current_pos_, static_cast<size_t>(std::distance(current_pos_, end_of_part_pos)));
            current_pos_ = last_ != end_of_part_pos ? std::next(end_of_part_pos) : end_of_part_pos;
        }
        return line;
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

} // namespace mtfind::aux
