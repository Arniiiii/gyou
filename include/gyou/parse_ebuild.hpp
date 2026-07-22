#ifndef INCLUDE_GYOU_PARSE_EBUILD_HPP_
#define INCLUDE_GYOU_PARSE_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "gyou/bash_ebuild.hpp"
#include "gyou/file_to_string.hpp"
#include "gyou/structs/change_related/pkg_type.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"
#include "gyou/structs/service_regex.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{

    // get current version of the pkg or commit of
    // current pkg extract service and link
    [[nodiscard]] corral::Task<
        std::expected<gyou::EbuildSpecificData, std::string>>
    get_ebuild_info(auto& ioc, gyou::Config const& cfg,
                    gyou::CommonContext& common_ctx,
                    std::filesystem::directory_entry const& path_to_ebuild)
    {
        std::string category_str = path_to_ebuild.path()
                                       .parent_path()
                                       .parent_path()
                                       .filename()
                                       .string();

        auto pkg_type = gyou::PackageType::Unknown;

        std::string pkg_p = path_to_ebuild.path().filename().stem().string();
        uint64_t date = 0;

        if (RE2::FullMatch(pkg_p, common_ctx.re_pkg_with_date, &date))
            {
                pkg_type = gyou::PackageType::Commit;
                LOG_INFO(
                    "This is a package per "
                    "commit: "
                    "{} . Its date: {}",
                    pkg_p, date);
            }
        else
            {
                pkg_type = gyou::PackageType::ReleaseOrTag;
                LOG_INFO(
                    "This is a package per "
                    "release: {}",
                    pkg_p);
            }

        common_ctx.re_version_matcher.input(pkg_p);
        if (not common_ctx.re_version_matcher.find())
            {
                co_return std::unexpected(fmt::format(
                    "Failed to parse version of package : {}", pkg_p));
            }

        size_t match_id = 0;
        while (common_ctx.re_version_matcher[match_id].first != nullptr)
            {
                LOG_TRACE_L2(
                    "sth: {}",
                    std::string_view{
                        common_ctx.re_version_matcher[match_id].first,
                        common_ctx.re_version_matcher[match_id].second});
                if (common_ctx.re_version_matcher[match_id].first == nullptr)
                    {
                        LOG_DEBUG("It is nullptr.");
                    }
                ++match_id;
            }

        auto pkg_n = std::string{common_ctx.re_version_matcher[1].first,
                                 common_ctx.re_version_matcher[1].second};
        auto pkg_v = std::string{common_ctx.re_version_matcher[2].first,
                                 common_ctx.re_version_matcher[2].second};
        LOG_DEBUG("PN: '{}' PV: '{}'", pkg_n, pkg_v);
        LOG_DEBUG("Doing bash for {}", path_to_ebuild.path());
        auto temp_folder_path_res = co_await gyou::bash_ebuild(
            ioc, cfg, path_to_ebuild.path(), pkg_v);

        if (!temp_folder_path_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to run ebuild.sh: {}",
                                temp_folder_path_res.error()));
            }
        std::filesystem::path temp_file_path
            = (temp_folder_path_res.value() / "environment");

        LOG_DEBUG("Attempt to read the environment file into memory at {}",
                  temp_file_path);
        auto str_file_res = co_await gyou::file_to_string(ioc, temp_file_path);
        if (not str_file_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to read file. error: {}",
                                str_file_res.error().message()));
            }
        LOG_TRACE_L2("env_file: \n{}\n\n", str_file_res.value());

        std::string_view commit_env_var_name;

        std::string_view commit;
        if (gyou::PackageType::Commit == pkg_type)
            {
                if (not RE2::PartialMatch(str_file_res.value(),
                                          common_ctx.re_commit_str,
                                          &commit_env_var_name, &commit))
                    {
                        co_return std::unexpected(
                            fmt::format("Regex for getting commit failed, "
                                        "possible error: '{}'",
                                        common_ctx.re_commit_str.error()));
                    };
            }

        // get first url in SRC_URI
        LOG_DEBUG("Attempt to get first URL from SRC_URI from {}",
                  temp_file_path);
        std::string_view src_uri_str;

        if (not RE2::PartialMatch(str_file_res.value(), common_ctx.re_src_uri,
                                  &src_uri_str))
            {
                co_return std::unexpected(fmt::format(
                    "Regex for getting first URL from SRC_URI failed: {}",
                    common_ctx.re_src_uri.error()));
            }

        // understand what service is this
        LOG_DEBUG("Trying to undertand which service is that for {}",
                  src_uri_str);
        std::vector<int> indeces_matched;
        indeces_matched.reserve(std::size(gyou::ServicesNames));

        common_ctx.re_set_services.Match(src_uri_str, &indeces_matched);

        if (indeces_matched.empty())
            {
                co_return std::unexpected(fmt::format(
                    "Failed to understand what service is an URI. URI: {}",
                    src_uri_str));
            }

        gyou::Service service_id = magic_enum::enum_cast<gyou::Service>(
                                       static_cast<size_t>(indeces_matched[0]))
                                       .value();

        LOG_DEBUG("Matched these: {}", magic_enum::enum_name(service_id));

        co_return gyou::EbuildSpecificData{
            .filepath = path_to_ebuild,
            .p = pkg_p,
            .pv = pkg_v,
            .pn = pkg_n,
            .category = category_str,
            .first_uri = std::string(src_uri_str),
            .commit_specific = std::invoke(
                [&]() -> std::optional<gyou::EbuildCommitSpecific>
                    {
                        if (gyou::PackageType::Commit == pkg_type)
                            {
                                return gyou::EbuildCommitSpecific{
                                    .date = date,
                                    .commit = std::string{commit},
                                    .env_var_name
                                    = std::string(commit_env_var_name)};
                            }

                        return std::nullopt;
                    }),
            .service = service_id};
    }
}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSE_EBUILD_HPP_
