#pragma once

#include <iterator>
#include <type_traits>

namespace mtfind::detail
{

template <typename Iterator>
constexpr bool is_random_access_iterator =
    std::is_same<std::random_access_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>::value;

template <typename Iterator>
constexpr bool is_conv_to_forward_iterator =
    std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>::value;

template <typename Iterator>
constexpr bool is_conv_to_input_iterator =
    std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::input_iterator_tag>::value;

} // namespace mtfind::detail
