#ifndef INCLUDE_GYOU_UTILS_HPP_
#define INCLUDE_GYOU_UTILS_HPP_

#include <array>
#include <utility>

namespace gyou
{

    template <std::size_t N, typename F> auto make_array_from_factory(F factory)
        -> std::array<std::decay_t<decltype(factory())>, N>
    {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                return std::array<std::decay_t<decltype(factory())>, N>{
                    {(static_cast<void>(Is), factory())...}};
            }(std::make_index_sequence<N>());
    }
}  // namespace gyou

#endif  // INCLUDE_GYOU_UTILS_HPP_
