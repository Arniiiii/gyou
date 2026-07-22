#ifndef INCLUDE_UTILS_RUSTY_MACROS_HPP_
#define INCLUDE_UTILS_RUSTY_MACROS_HPP_

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) */
#define TRY_OR_CO_RETURN(expr)                                      \
    __extension__({                                                 \
        auto&& _res = (expr);                                       \
        if (!_res)                                                  \
            {                                                       \
                co_return std::unexpected(std::move(_res.error())); \
            }                                                       \
        std::move(*_res);                                           \
    })

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
 */
#define TRY_OR_CO_RETURN_VOID(expr)                                     \
    do                                                                  \
        {                                                               \
            auto&& _res = (expr);                                       \
            if (!_res)                                                  \
                {                                                       \
                    co_return std::unexpected(std::move(_res.error())); \
                }                                                       \
        }                                                               \
    while (0)

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
 */
#define TRY_OR_CO_RETURN_VOID_TRANSFORM_ERROR(expr, expr2)                     \
    do                                                                         \
        {                                                                      \
            auto&& _res = (expr);                                              \
            if (!_res)                                                         \
                {                                                              \
                    co_return std::unexpected(expr2(std::move(_res.error()))); \
                }                                                              \
        }                                                                      \
    while (0)

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) */
#define TRY_OR_CO_RETURN_TRANSFORM_ERROR(expr, expr2)                      \
    __extension__({                                                        \
        auto&& _res = (expr);                                              \
        if (!_res)                                                         \
            {                                                              \
                co_return std::unexpected(expr2(std::move(_res.error()))); \
            }                                                              \
        std::move(*_res);                                                  \
    })

#endif  // INCLUDE_UTILS_RUSTY_MACROS_HPP_
