#pragma once

#include <string>

#include "splitters/range_splitter.hpp"

namespace mtfind
{

class StreamSplitter
{
public:
    explicit StreamSplitter(std::istream &is, char delim = '\0')
        : splitter_(std::istream_iterator<char>(is), std::istream_iterator<char>(), delim)
    {}

    auto operator()()
    {
        std::string token;
        splitter_(std::back_inserter(token));
        return token;
    }

    operator bool() const noexcept { return splitter_; }
    bool operator!() const noexcept { return !splitter_; }

private:
    RangeSplitter<std::istream_iterator<char>> splitter_;
};

} // namespace mtfind
