#pragma once

#include <cstddef>

#include <utility>
#include <vector>

#include <boost/utility/string_view.hpp>

namespace mtfind::detail
{

using pos_t         = size_t;
using match_t       = boost::string_view;
using Finding       = std::pair<pos_t, match_t>;
using Findings      = std::vector<Finding>;

using line_idx_t    = size_t;
using LineFindings  = std::pair<line_idx_t, Findings>;
using LinesFindings = std::vector<LineFindings>;

} // namespace mtfind
