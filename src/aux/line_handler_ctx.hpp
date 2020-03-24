#pragma once

#include <cstddef>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/utility/string_view.hpp>

#include "line.hpp"

namespace mtfind::detail
{

class LineHandlerCtx
{
public:
    template<typename Iterator, typename RangeOfFindings>
    void consume(size_t line_idx, Iterator first, RangeOfFindings &&range_of_findings)
    {
        auto findings = range_of_findings | boost::adaptors::transformed([first](auto const &finding_range){
            return Finding(std::distance(first, finding_range.first) + 1,
                {&*finding_range.first, static_cast<size_t>(std::distance(finding_range.first, finding_range.second))});
        });
        lines_findings_.push_back(std::make_pair(line_idx, Findings{findings.begin(), findings.end()}));
    }

    auto& lines_findings() noexcept { return lines_findings_; }
    auto const& lines_findings() const noexcept { return lines_findings_; }

protected:
    LinesFindings lines_findings_;
};

} // namespace mtfind::aux
