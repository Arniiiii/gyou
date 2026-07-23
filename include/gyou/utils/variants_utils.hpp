#ifndef INCLUDE_GYOU_VARIANTS_UTILS_HPP_
#define INCLUDE_GYOU_VARIANTS_UTILS_HPP_

#include <utility>
#include <variant>

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
template <typename Variant, typename... Fs>
decltype(auto) match(Variant&& vari, Fs&&... arm)
{
    return std::visit(overloads{std::forward<Fs>(arm)...},
                      std::forward<Variant>(vari));
}

#endif  // INCLUDE_GYOU_VARIANTS_UTILS_HPP_
