#pragma once

#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include "splitters/range_splitter.hpp"

namespace mtfind
{

///
/// @brief      This class describes a stream splitter that splits stream
///             into tokens separated by a delimiter
///
template <typename ValueType>
class StreamSplitter
{
public:
    ///
    /// @brief      Constructs stream splitter
    ///
    /// @param[in]  is     Input stream
    /// @param[in]  delim  The delimiter specifying a separator between tokens
    ///
    explicit StreamSplitter(std::istream &is, ValueType const &delim = ValueType())
        : splitter_(std::istream_iterator<ValueType>(is), std::istream_iterator<ValueType>(), delim)
    {
    }

    ///
    /// @brief      Gets a token in a shape of vector of items
    ///
    /// @return     The token
    ///
    template <typename U = ValueType>
    typename std::enable_if<!std::is_same<U, char>::value, std::vector<ValueType>>::type
        operator()()
    {
        // @info you can use std::getline here instead of using Range-based splitter
        std::vector<ValueType> token;
        splitter_(std::back_inserter(token));
        return token;
    }

    ///
    /// @brief      Bool conversion operator. Checks if the splitter is not exhausted
    ///
    /// @returns    True if the splitter is not finished yet, False otherwise
    ///
    operator bool() const noexcept { return splitter_; }

    ///
    /// @brief      Bool conversion operator. Checks if the splitter is exhausted
    ///
    /// @returns    True if the splitter is exhausted, False otherwise
    ///
    bool operator!() const noexcept { return !splitter_; }

private:
    RangeSplitter<std::istream_iterator<ValueType>> splitter_;
};

///
/// @brief      This class is a specialization of template class StreamSplitter
///
template<>
class StreamSplitter<char>
{
public:
    ///
    /// @brief      Constructs stream splitter
    ///
    /// @param[in]  is     Input stream
    /// @param[in]  delim  The delimiter specifying a separator between tokens
    ///
    explicit StreamSplitter(std::istream &is, char delim = '\n')
        : is_(is), delim_(delim)
    {
    }

    ///
    /// @brief      Gets a string-like token by means of std::getline
    ///
    /// @return     The token
    ///
    auto operator()()
    {
        std::string token;
        std::getline(is_, token, delim_);
        return token;
    }

    ///
    /// @brief      Bool conversion operator. Checks if the stream is not exhausted
    ///
    /// @returns    True if the stream is not finished yet, False otherwise
    ///
    operator bool() const noexcept { return static_cast<bool>(is_); }

    ///
    /// @brief      Bool conversion operator. Checks if the stream is exhausted
    ///
    /// @returns    True if the stream is exhausted, False otherwise
    ///
    bool operator!() const noexcept { return !is_; }

private:
    std::istream &is_;
    char         delim_;
};

} // namespace mtfind
