#ifndef INCLUDE_STRUCTS_COMMON_CTX_HPP_
#define INCLUDE_STRUCTS_COMMON_CTX_HPP_

#include <re2/re2.h>
#include <re2/set.h>
#include <reflex/pcre2matcher.h>

namespace gyou
{

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct CommonContext
    {
        RE2 re_commit_str;
        RE2 re_src_uri;
        RE2 re_category;
        RE2 re_pkg_9999;
        RE2 re_pkg_with_date;
        RE2::Set re_set_services;
        reflex::PCRE2UTFMatcher re_version_matcher;
    };
}  // namespace gyou

#endif  // INCLUDE_STRUCTS_COMMON_CTX_HPP_
