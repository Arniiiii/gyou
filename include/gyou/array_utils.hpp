#ifndef INCLUDE_GYOU_UTILS_HPP_
#define INCLUDE_GYOU_UTILS_HPP_

#include <array>
#include <tuple>
#include <utility>

template <std::size_t N, typename F> auto make_array_from_factory(F f)
    -> std::array<std::decay_t<decltype(f())>, N>
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return std::array<std::decay_t<decltype(f())>, N>{
                {(static_cast<void>(Is), f())...}};
        }(std::make_index_sequence<N>());
}

template <std::size_t I, typename... Args> struct Part
{
    std::tuple<Args...> args;
    static constexpr std::size_t index = I;
};

template <typename T, typename... Parts> struct ArrayBuilder
{
    std::tuple<Parts...> parts;

  public:
    template <auto I, typename... Args> constexpr auto e(Args&&... args) &&
    {
        static constexpr auto index = []() constexpr
            {
                if constexpr (std::is_scoped_enum_v<decltype(I)>)
                    {
                        return static_cast<std::size_t>(std::to_underlying(I));
                    }
                else
                    {
                        return static_cast<std::size_t>(I);
                    }
            }();
        static_assert(((index != Parts::index) && ...),
                      "index appears multiple times!");
        return std::apply(
            [&](const auto&... parts)
                {
                    using new_part_type = Part<index, Args&&...>;
                    return ArrayBuilder<T, Parts..., new_part_type>{
                        std::tuple<Parts..., new_part_type>(
                            parts..., new_part_type{std::tuple<Args&&...>(
                                          std::forward<Args>(args)...)})};
                },
            parts);
    }

  private:
    static constexpr std::size_t max_index = std::max({0ZU, Parts::index...});

    template <std::size_t I, std::size_t J = 0>
    constexpr T initialize_for_index()
    {
        if constexpr (J == sizeof...(Parts))
            {
                return T();
            }
        else if constexpr (std::tuple_element_t<J, std::tuple<Parts...>>::index
                           == I)
            {
                return std::apply(
                    []<typename... Args>(Args&&... args)
                        { return T(std::forward<Args>(args)...); },
                    std::get<J>(parts).args);
            }
        else
            {
                return initialize_for_index<I, J + 1ZU>();
            }
    }

  public:
    constexpr auto build() &&
    {
        return [&]<std::size_t... I>(std::index_sequence<I...>)
            {
                return std::array<T, max_index + 1ZU>{
                    {initialize_for_index<I>()...}};
            }(std::make_index_sequence<max_index + 1ZU>{});
    }
};

#endif  // INCLUDE_GYOU_UTILS_HPP_
