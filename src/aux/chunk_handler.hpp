#pragma once

#include <cstddef>

#include <iterator>
#include <tuple>
#include <utility>

#include <boost/function_output_iterator.hpp>

#include "chunk.hpp"

namespace mtfind::detail
{

template <typename Tokenizer, typename ValueType>
class ChunkHandlerBase
{
public:
    using value_type     = ValueType;
    using Container      = ChunksFindings<value_type>;
    using const_iterator = typename Container::const_iterator;

public:
    explicit ChunkHandlerBase(Tokenizer tokenizer)
        : tokenizer_(tokenizer)
    {
    }

    void operator()(size_t chunk_idx, value_type const &chunk_value)
    {
        tokenizer_(chunk_value, boost::make_function_output_iterator([this, chunk_idx = chunk_idx + 1, first = std::begin(chunk_value)](auto &&token) {
                       findings_.push_back(std::make_tuple(
                           chunk_idx,
                           static_cast<size_t>(std::distance(first, token.begin()) + 1),
                           ValueType{token.begin(), token.end()}));
                   }));
    }

    auto &      findings() noexcept { return findings_; }
    auto const &findings() const noexcept { return findings_; }

protected:
    Tokenizer tokenizer_;
    Container findings_;
};

template <typename Tokenizer, typename ValueType>
class ChunkHandler : public ChunkHandlerBase<Tokenizer, ValueType>
{
public:
    using base_t = ChunkHandlerBase<Tokenizer, ValueType>;

    explicit ChunkHandler(Tokenizer tokenizer)
        : base_t(tokenizer)
    {
    }

    void operator()(size_t chunk_idx, ValueType const &chunk_value)
    {
        base_t::operator()(chunk_idx, chunk_value);
        set_last_chunk_idx(chunk_idx);
    }

    size_t last_chunk_idx() const noexcept { return last_chunk_idx_; }

private:
    void set_last_chunk_idx(size_t chunk_idx) noexcept { last_chunk_idx_ = chunk_idx + 1; }

private:
    size_t last_chunk_idx_ = 0;
};

} // namespace mtfind::detail
