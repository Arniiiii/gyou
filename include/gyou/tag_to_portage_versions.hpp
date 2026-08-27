#ifndef INCLUDE_GYOU_TAG_TO_PORTAGE_VERSIONS_HPP_
#define INCLUDE_GYOU_TAG_TO_PORTAGE_VERSIONS_HPP_

#include <expected>
#include <string>

#include <reflex/pcre2matcher.h>

namespace gyou
{

    std::expected<std::string, std::string> tag_to_portage_version(

        reflex::PCRE2UTFMatcher& re_package_version_matcher,
        std::string const& tag_or_version);

}

#endif  // INCLUDE_GYOU_TAG_TO_PORTAGE_VERSIONS_HPP_
