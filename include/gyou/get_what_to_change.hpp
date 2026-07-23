#ifndef INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_
#define INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_

#include <filesystem>
#include <string>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/Nursery.h>
#include <corral/asio.h>
#include <corral/corral.h>

#include "gyou/parsing_one_ebuild.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/result_of_parsing.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<gyou::PackagesToUpdate> get_what_to_change(
        auto& ioc, gyou::Config const& cfg, auto& semaphores,
        gyou::CommonContext& common_ctx)
    {
        gyou::PackagesToUpdate changes;
        CORRAL_WITH_NURSERY(nursery)
        {
            for (const auto& category :
                 std::filesystem::directory_iterator(cfg.path_to_repo))
                {
                    if (not category.is_directory())
                        {
                            continue;
                        }
                    std::string category_str
                        = category.path().filename().string();
                    if (not RE2::FullMatch(category_str,
                                           common_ctx.re_category))
                        {
                            continue;
                        }

                    for (const auto& pkg_name :
                         std::filesystem::directory_iterator(category))
                        {
                            if (not pkg_name.is_directory())
                                {
                                    continue;
                                }

                            for (const auto& file :
                                 std::filesystem::directory_iterator(pkg_name))
                                {
                                    if (not file.is_regular_file())
                                        {
                                            continue;
                                        }

                                    std::filesystem::path pkg_filename
                                        = file.path().filename();

                                    if (pkg_filename.extension() != ".ebuild")
                                        {
                                            continue;
                                        }

                                    std::string pkg_p
                                        = pkg_filename.stem().string();

                                    if (RE2::FullMatch(pkg_p,
                                                       common_ctx.re_pkg_9999))
                                        {
                                            continue;
                                        }
                                    nursery.start(
                                        [&](std::filesystem::directory_entry
                                                file_arg) mutable
                                            -> corral::Task<void>
                                            {
                                                auto sth = co_await gyou::
                                                    logic_per_ebuild(
                                                        ioc, cfg, file_arg,
                                                        semaphores, common_ctx);
                                                if (not sth)
                                                    {
                                                        changes.is_any_failed
                                                            = true;
                                                        LOG_ERROR(
                                                            "Failed to do "
                                                            "sth "
                                                            "with a "
                                                            "ebuild: {}",
                                                            sth.error());
                                                        co_return;
                                                    }
                                                if (not sth.value().has_value())
                                                    {
                                                        changes
                                                            .is_any_successful
                                                            = true;
                                                        LOG_INFO(
                                                            "Nothing to change "
                                                            "for {}",
                                                            file_arg.path());
                                                        co_return;
                                                    }
                                                changes.what_to_change
                                                    .emplace_back(
                                                        sth.value().value());
                                            },
                                        file);
                                }
                        }
                }

            co_return corral::join;
        };
        co_return changes;
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_
