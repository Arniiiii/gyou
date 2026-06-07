#ifndef INCLUDE_GYOU_UTILS_HPP_
#define INCLUDE_GYOU_UTILS_HPP_

#include <array>

template <std::size_t N, typename F> auto make_array_from_factory(F f)
    -> std::array<std::decay_t<decltype(f())>, N>
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return std::array<std::decay_t<decltype(f())>, N>{
                {(static_cast<void>(Is), f())...}};
        }(std::make_index_sequence<N>());
}

#endif  // INCLUDE_GYOU_UTILS_HPP_
