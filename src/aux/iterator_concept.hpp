#pragma once

#include <type_traits>
#include <iterator>

namespace mtfind::detail
{

template<typename Iterator>
using is_random_access_char_iterator = typename std::enable_if<std::is_same<std::random_access_iterator_tag,
typename std::iterator_traits<Iterator>::iterator_category>::value
    && std::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value, std::true_type>::type;

template<typename Iterator>
using is_forward_char_iterator = typename std::enable_if<std::is_same<std::forward_iterator_tag,
typename std::iterator_traits<Iterator>::iterator_category>::value
    && std::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value, std::true_type>::type;

template<typename Iterator>
using is_conv_to_forward_char_iterator = typename std::enable_if<std::is_convertible<
typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>::value
    && std::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value, std::true_type>::type;

} // namespace mtfind
