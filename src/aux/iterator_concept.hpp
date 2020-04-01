#pragma once

#include <type_traits>
#include <iterator>

namespace mtfind::detail
{

template<typename Iterator>
constexpr bool is_char_iterator = std::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value;

template<typename Iterator>
constexpr bool is_random_access_char_iterator =
    std::is_same<std::random_access_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>::value
    && is_char_iterator<Iterator>;

template<typename Iterator>
constexpr bool is_conv_to_forward_iterator =
    std::is_convertible< typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>::value;

template<typename Iterator>
constexpr bool is_conv_to_input_iterator =
    std::is_convertible< typename std::iterator_traits<Iterator>::iterator_category, std::input_iterator_tag>::value;

} // namespace mtfind
