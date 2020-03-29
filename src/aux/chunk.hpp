#pragma once

#include <cstddef>

#include <utility>
#include <vector>

namespace mtfind::detail
{

using pos_t          = size_t;

template<typename Value>
using Finding        = std::pair<pos_t, Value>;

template<typename Value>
using Findings       = std::vector<Finding<Value>>;

using chunk_idx_t    = size_t;

template<typename Value>
using ChunkFindings  = std::pair<chunk_idx_t, Findings<Value>>;

template<typename Value>
using ChunksFindings = std::vector<ChunkFindings<Value>>;

} // namespace mtfind
