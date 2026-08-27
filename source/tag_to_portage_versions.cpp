#include "gyou/tag_to_portage_versions.hpp"

#include <expected>

#include <fmt/format.h>
#include <reflex/pcre2matcher.h>

#include "overwrite_log_macros.hpp"

namespace gyou
{

    std::expected<std::string, std::string> tag_to_portage_version(

        reflex::PCRE2UTFMatcher& re_package_version_matcher,
        std::string const& tag_or_version)
    {
        // time wasted: 5 hours.
        // re-flex's API is un-intuitive.

        std::string portage_version{};

        bool have_we_entered_the_ugly_loop = false;

        re_package_version_matcher.input(tag_or_version);

        std::string ver{};
        std::string tag{};
        std::string subver{};

        for (auto& match : re_package_version_matcher.find)
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
                        if (strcmp(grp_id.second, "ver") == 0)
                            {
                                subpattern = match[grp_id.first];
                                ver = std::string(subpattern.first,
                                                  subpattern.second);
                                grp_id = match.group_next_id();
                                continue;
                            }

                        if (strcmp(grp_id.second, "tag") == 0)
                            {
                                subpattern = match[grp_id.first];
                                tag = std::string(subpattern.first,
                                                  subpattern.second);

                                grp_id = match.group_next_id();
                                continue;
                            }

                        if (strcmp(grp_id.second, "subver") == 0)
                            {
                                subpattern = match[grp_id.first];
                                subver = std::string(subpattern.first,
                                                     subpattern.second);

                                grp_id = match.group_next_id();
                                continue;
                            }

                        grp_id = match.group_next_id();
                    }
            }

        if (not have_we_entered_the_ugly_loop)
            {
                return std::unexpected(
                    fmt::format("Failed to parse version of "
                                "fetched version : {}",
                                tag_or_version));
            }

        if (not tag.empty())
            {
                tag = "-" + tag;
            }
        if (not subver.empty())
            {
                subver = "-" + subver;
            }

        portage_version = fmt::format("{}{}{}", ver, tag, subver);

        return portage_version;
    }

}  // namespace gyou
