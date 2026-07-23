#ifndef INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_
#define INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <optional>

#include <boost/date_time.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <magic_enum/magic_enum.hpp>

#include "gyou/get_latest_updates.hpp"
#include "gyou/parse_ebuild.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"
#include "gyou/structs/result_of_parsing.hpp"
#include "gyou/utils/variants_utils.hpp"
#include "gyou/utils/rusty_macros.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{

    //  return what to change
    [[nodiscard]] corral::Task<
        std::expected<std::optional<InfoForDiff>, std::string>>
    logic_per_ebuild(auto& ioc, gyou::Config const& cfg,
                     std::filesystem::directory_entry const& path_to_ebuild,
                     auto& semaphores, gyou::CommonContext& common_ctx)
    {
        gyou::EbuildSpecificData const ebuild_data
            = TRY_OR_CO_RETURN(co_await gyou::get_ebuild_info(
                ioc, cfg, common_ctx, path_to_ebuild));

        LOG_TRACE_L1("Current data: '{}' '{}' '{}' '{}' '{}' '{}'",
                     ebuild_data.first_uri,
                     magic_enum::enum_name(ebuild_data.service),
                     ebuild_data.filepath, ebuild_data.p, ebuild_data.pn,
                     ebuild_data.pv);

        if (ebuild_data.commit_specific.has_value())
            {
                LOG_DEBUG("Current date: '{}' env_var: '{}' commit: '{}'",
                          ebuild_data.commit_specific.value().date,
                          ebuild_data.commit_specific.value().env_var_name,
                          ebuild_data.commit_specific.value().commit);
            }

        std::variant<gyou::CommitSpecific, std::string> fetched_ver
            = TRY_OR_CO_RETURN(co_await gyou::get_latest_info(
                ioc, cfg, semaphores, common_ctx, ebuild_data));

        LOG_DEBUG("Current ver: {}", ebuild_data.pv);

        if (ebuild_data.commit_specific.has_value())
            {
                LOG_INFO("Current date: {} commit: {}",
                         ebuild_data.commit_specific.value().date,
                         ebuild_data.commit_specific.value().commit);
            }

        fetched_ver.visit(overloads{
            [&](gyou::CommitSpecific const& fetched) mutable
                {
                    LOG_INFO("Fetched: '{}' '{}' '{}'", ebuild_data.p,
                             fetched.date, fetched.commit);
                },
            [&](std::string const& fetched)
                { LOG_INFO("Fetched: '{}' '{}'", ebuild_data.p, fetched); }});

        bool is_changed = false;
        fetched_ver.visit(overloads{
            [&](gyou::CommitSpecific const& fetched) mutable
                {
                    if (ebuild_data.commit_specific.value().date < fetched.date)
                        {
                            is_changed = true;
                        }
                },
            [&](std::string const& fetched) mutable
                {
                    if (ebuild_data.pv < fetched)
                        {
                            is_changed = true;
                        }
                }});

        if (is_changed)
            {
                LOG_INFO("New version!!!");
                InfoForDiff diff{};
                fetched_ver.visit(overloads{
                    [&](gyou::CommitSpecific const& fetched) mutable
                        {
                            diff
                                = {.data_for_how_to_change = EditCommit{
                                       .env_var_name
                                       = ebuild_data.commit_specific.value()
                                             .env_var_name,
                                       .old_ver = gyou::
                                           CommitSpecific{.date
                                                          = ebuild_data
                                                                .commit_specific
                                                                .value()
                                                                .date,
                                                          .commit
                                                          = ebuild_data
                                                                .commit_specific
                                                                .value()
                                                                .commit},
                                       .new_ver = fetched}};
                        },
                    [&](std::string const& fetched) mutable
                        {
                            diff = {.data_for_how_to_change
                                    = EditVerOrTag{.old_ver = ebuild_data.pv,
                                                   .new_ver = fetched}};
                        }});
                diff.path_to_ebuild = ebuild_data.filepath;
                co_return diff;
            }

        // return what has to be changed

        co_return std::nullopt;
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_
