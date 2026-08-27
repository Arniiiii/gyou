#include "gyou/common_ctx_create.hpp"

#include <re2/re2.h>
#include <re2/set.h>
#include <reflex/pcre2matcher.h>

#include "gyou/consts/pcre2_regex_portage_ver.hpp"
#include "gyou/consts/pcre2_regex_tag_ver.hpp"
#include "gyou/create_pcre2_regex.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/service_regex.hpp"

namespace gyou
{

    gyou::CommonContext create_common_ctx()
    {
        // to compile them all at once

        return gyou::CommonContext{
            .re_commit_str = RE2(
                R"delimiter(declare -- ([a-zA-Z_]?[a-zA-Z0-9_]*?COMMIT[a-zA-Z0-9_]*?)="([0-9a-f]{40})"\n)delimiter",
                RE2::Quiet),
            .re_src_uri = RE2(
                R"delimiter(declare SRC_URI=\$?["'](?:\\n)?(?:\\t)?(?:\s*)?(https?://\S*).*?['"])delimiter",
                RE2::Quiet),

            .re_category = RE2(R"(([\w][\w+.-]*))", RE2::Quiet),
            .re_pkg_9999 = RE2(R"([\w+.-]*9999)", RE2::Quiet),

            .re_pkg_with_date = RE2(R"([\w+.-]+?(\d{8})[\w+.-]*?)", RE2::Quiet),
            .re_set_services = std::invoke(
                []()
                    {
                        RE2::Set re_set_services(RE2::DefaultOptions,
                                                 RE2::Anchor::UNANCHORED);
                        for (auto&& service : gyou::ServicesNames)
                            {
                                re_set_services.Add(service, nullptr);
                            }
                        re_set_services.Compile();
                        return re_set_services;
                    }),
            .re_version_matcher = create_pcre2_regex(
                std::string(gyou::PCRE2_REGEX_PORTAGE_VERSION)),
            .re_package_version_matcher
            = create_pcre2_regex(std::string(gyou::PCRE2_REGEX_TAG_VERSION)),
        };
    }

}  // namespace gyou
