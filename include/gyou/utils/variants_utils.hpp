#ifndef INCLUDE_GYOU_VARIANTS_UTILS_HPP_
#define INCLUDE_GYOU_VARIANTS_UTILS_HPP_

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

#endif  // INCLUDE_GYOU_VARIANTS_UTILS_HPP_
