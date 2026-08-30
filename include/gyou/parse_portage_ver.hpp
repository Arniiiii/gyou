#ifndef INCLUDE_GYOU_PARSE_PORTAGE_VER_HPP_
#define INCLUDE_GYOU_PARSE_PORTAGE_VER_HPP_

#include <string>

#include <reflex/pcre2matcher.h>

#include "gyou/structs/portage_ver.hpp"

namespace gyou
{
    std::optional<gyou::ParsedEbuildStem> parse_ebuild_name(
        reflex::PCRE2UTFMatcher& re_portage_version_matcher,
        std::string const& ebuild_stem);
}

#endif  // INCLUDE_GYOU_PARSE_PORTAGE_VER_HPP_
