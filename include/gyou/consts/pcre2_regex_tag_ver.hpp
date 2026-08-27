#ifndef INCLUDE_CONSTS_PCRE2_REGEX_TAG_VER_HPP_
#define INCLUDE_CONSTS_PCRE2_REGEX_TAG_VER_HPP_

#include <string_view>

namespace gyou
{

    inline constexpr std::string_view PCRE2_REGEX_TAG_VERSION
        = R"(((?<name>[\w][\w+-]*?)-)?(?<ver>(\d+)(\.\d+)*)(([-_\.]?(?<tag>(pre|p|beta|alpha|rc)\d*))|(?<subver>[a-z]))*(-r(\d+))?)";
}

#endif  // INCLUDE_CONSTS_PCRE2_REGEX_TAG_VER_HPP_
