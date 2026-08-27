#ifndef INCLUDE_GYOU_COMMON_CTX_CREATE_HPP_
#define INCLUDE_GYOU_COMMON_CTX_CREATE_HPP_

#include "gyou/structs/common_ctx.hpp"

namespace gyou
{

    /**
     * @brief Idea is next: compile all regexes once at initialization. If there
     * was a library for pcre2 and re2 compiling at compile-time it would be
     * used. But here I do what I can to do it once per runtime.
     *
     * This shit can give exceptions. If it does, it only means one thing:
     * somewhere is a bad regex string.
     */
    gyou::CommonContext create_common_ctx();

}  // namespace gyou

#endif  // INCLUDE_GYOU_COMMON_CTX_CREATE_HPP_
