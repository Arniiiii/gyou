#ifndef INCLUDE_GYOU_VARIANTS_UTILS_HPP_
#define INCLUDE_GYOU_VARIANTS_UTILS_HPP_

#include <utility>
#include <variant>

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

template <typename Variant, typename... Fs>
concept CIsAnyArmACoroutine = requires {
    typename std::invoke_result_t<overloads<Fs...>,
                                  decltype(std::get<0>(
                                      std::declval<Variant>()))>::promise_type;
};
template <typename Variant, typename... Fs>
    requires(not CIsAnyArmACoroutine<Variant, Fs...>)
decltype(auto) match(Variant&& vari, Fs&&... arm)
{
    return std::visit(overloads{std::forward<Fs>(arm)...},
                      std::forward<Variant>(vari));
}

// This overload is selected when the visitor returns a corral::Task (or any
// coroutine type)
template <typename Variant, typename... Fs>
    requires(CIsAnyArmACoroutine<Variant, Fs...>)
auto match(Variant vari, Fs... arm)
    -> std::invoke_result_t<overloads<Fs...>,
                            decltype(std::get<0>(std::declval<Variant>()))>
{
    // `visitor` and `vari` are stored as local variables inside THIS coroutine
    // frame. They will stay alive across all co_await suspensions until
    // completion!
    auto visitor = overloads{std::move(arm)...};
    co_return co_await std::visit(visitor, std::move(vari));
}

#endif  // INCLUDE_GYOU_VARIANTS_UTILS_HPP_
