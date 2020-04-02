#pragma once

#include <cstddef>

#include <tuple>
#include <utility>
#include <vector>

namespace mtfind::detail
{

using chunk_idx_t = size_t;
using pos_t       = size_t;

template <typename Value>
using ChunkFinding = std::tuple<chunk_idx_t, pos_t, Value>;

template <typename Value>
using ChunksFindings = std::vector<ChunkFinding<Value>>;

} // namespace mtfind::detail
