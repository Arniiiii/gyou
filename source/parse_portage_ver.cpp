#include "gyou/parse_portage_ver.hpp"

#include <optional>
#include <string>

#include <reflex/pcre2matcher.h>

#include "gyou/consts/pcre2_regex_portage_ver.hpp"
#include "gyou/structs/portage_ver.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{
    std::optional<gyou::ParsedEbuildStem> parse_ebuild_name(
        reflex::PCRE2UTFMatcher& re_portage_version_matcher,
        std::string const& ebuild_stem)
    {
        // Probably this loop should be rewritten into a perfect hash map.
        bool have_we_entered_the_ugly_loop = false;

        re_portage_version_matcher.input(ebuild_stem);

        ParsedEbuildStem res;

        for (auto& match : re_portage_version_matcher.find)
            {
                have_we_entered_the_ugly_loop = true;
                std::pair<const char*, size_t> subpattern;
                std::pair<size_t, const char*> grp_id = match.group_id();
                while (grp_id.first != 0)
                    {
                        if (grp_id.second == nullptr)
                            {
                                grp_id = match.group_next_id();
                                continue;
                            }
                        LOG_TRACE_L2(
                            "sth named '{}': '{}'", grp_id.second,
                            std::string_view(match[grp_id.first].first,
                                             match[grp_id.first].second));
                        if (strcmp(grp_id.second, "pn") == 0)
                            {
                                subpattern = match[grp_id.first];
                                res.pn = std::string(subpattern.first,
                                                     subpattern.second);
                                grp_id = match.group_next_id();
                                continue;
                            }

                        if (strcmp(grp_id.second, "pn_inval") == 0)
                            {
                                subpattern = match[grp_id.first];
                                res.pn_inval = std::string(subpattern.first,
                                                           subpattern.second);

                                grp_id = match.group_next_id();
                                continue;
                            }

                        if (strcmp(grp_id.second, "ver") == 0)
                            {
                                subpattern = match[grp_id.first];
                                res.ver = std::string(subpattern.first,
                                                      subpattern.second);

                                grp_id = match.group_next_id();
                                continue;
                            }

                        if (strcmp(grp_id.second, "rev") == 0)
                            {
                                subpattern = match[grp_id.first];
                                res.rev = std::string(subpattern.first,
                                                      subpattern.second);

                                grp_id = match.group_next_id();
                                continue;
                            }

                        grp_id = match.group_next_id();
                    }
            }

        if (not have_we_entered_the_ugly_loop)
            {
                return std::nullopt;
            }

        return res;
    }
}  // namespace gyou
