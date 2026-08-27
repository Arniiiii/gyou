#ifndef INCLUDE_CONSTS_PCRE2_REGEX_PORTAGE_VER_HPP_
#define INCLUDE_CONSTS_PCRE2_REGEX_PORTAGE_VER_HPP_

#include <string_view>

namespace gyou
{

    inline constexpr std::string_view PCRE2_REGEX_PORTAGE_VERSION
        = R"(([\w][\w+-]*?)-((\d+)(\.\d+)*)([a-z]?)((_(pre|p|beta|alpha|rc)\d*)*)(-r(\d+))?)";
}

#endif  // INCLUDE_CONSTS_PCRE2_REGEX_PORTAGE_VER_HPP_
