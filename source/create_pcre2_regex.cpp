
#include "gyou/create_pcre2_regex.hpp"

#include <reflex/pcre2matcher.h>

namespace gyou
{
    reflex::PCRE2UTFMatcher create_pcre2_regex(std::string const& regex_str)
    {
        // NOLINTBEGIN(hicpp-signed-bitwise)
        std::string str_re_versions = reflex::PCRE2UTFMatcher::convert(
            regex_str,
            reflex::convert_flag::unicode | reflex::convert_flag::notnewline);
        // NOLINTEND(hicpp-signed-bitwise)

        const reflex::PCRE2UTFMatcher::Pattern& pattern_re_versions(
            str_re_versions);

        return {pattern_re_versions};
    }

}  // namespace gyou
