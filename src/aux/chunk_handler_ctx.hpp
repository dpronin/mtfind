#pragma once

#include <cstddef>

#include <utility>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

#include "chunk.hpp"

namespace mtfind::detail
{

template<typename Value>
class ChunkHandlerCtx
{
public:
    using value_type     = Value;
    using Container      = ChunksFindings<value_type>;
    using const_iterator = typename Container::const_iterator;

    template<typename Iterator, typename Container, typename U = Value>
    auto consume(size_t chunk_idx, Iterator first, Container const &findings) ->
        typename std::enable_if<std::is_same<U, boost::string_view>::value
            && std::is_same<typename Container::value_type, boost::iterator_range<Iterator>>::value>::type
    {
        auto transformed_findings = findings | boost::adaptors::transformed([first](auto const &finding) {
            return Finding<Value>(std::distance(first, finding.begin()) + 1, Value(&*finding.begin(), finding.size()));
        });
        chunks_findings_.push_back(std::make_pair(chunk_idx + 1, Findings<Value>{transformed_findings.begin(), transformed_findings.end()}));
    }

    template<typename Iterator, typename Container, typename U = Value>
    auto consume(size_t chunk_idx, Iterator first, Container const &findings) ->
        typename std::enable_if<!std::is_same<U, boost::string_view>::value
            && std::is_same<typename Container::value_type, boost::iterator_range<Iterator>>::value>::type
    {
        auto transformed_findings = findings | boost::adaptors::transformed([first](auto const &finding) {
            return Finding<Value>(std::distance(first, finding.begin()) + 1, Value(finding.begin(), finding.end()));
        });
        chunks_findings_.push_back(std::make_pair(chunk_idx + 1, Findings<Value>{transformed_findings.begin(), transformed_findings.end()}));
    }

    auto& chunks_findings() noexcept { return chunks_findings_; }
    auto const& chunks_findings() const noexcept { return chunks_findings_; }

    auto begin() const noexcept { return chunks_findings_.begin(); }
    auto begin() noexcept { return chunks_findings_.begin(); }

    auto end() const noexcept { return chunks_findings_.end(); }
    auto end() noexcept { return chunks_findings_.end(); }

    bool empty() const noexcept { return chunks_findings_.empty(); }
    auto size() const noexcept { return chunks_findings_.size(); }

protected:
    Container chunks_findings_;
};

} // namespace mtfind::aux
