#include "gyou/apply_change.hpp"

#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/utility.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <quill/std/FilesystemPath.h>

#include "gyou/bash_ebuild_manifest.hpp"
#include "gyou/file_to_string.hpp"
#include "gyou/string_to_file.hpp"
#include "gyou/utils/rusty_macros.hpp"
#include "gyou/utils/string_replace.hpp"
#include "gyou/utils/variants_utils.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>> apply_change(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        std::filesystem::path const where_to_change,
        gyou::InfoForDiff const& diff_info)
    {
        LOG_INFO("Trying to apply a change for next ebuild: {}",
                 diff_info.path_to_ebuild);
        co_return co_await (match(
            diff_info.data_for_how_to_change,
            overloads{
                [&](gyou::EditCommit const& diff_details)
                    -> corral::Task<std::expected<void, std::string>>
                    {
                        // change commit in ebuild

                        std::string const editted_ebuild_content
                            = __extension__({
                                  std::string ebuild_content
                                      = TRY_OR_CO_RETURN_TRANSFORM_ERROR(
                                          co_await gyou::file_to_string(
                                              ioc, diff_info.path_to_ebuild),
                                          [](boost::system::error_code errc)
                                              { return errc.what(); });
                                  gyou::Replace(std::move(ebuild_content),
                                                diff_details.old_ver.commit,
                                                diff_details.new_ver.commit);
                              });

                        std::string const new_name = gyou::Replace(
                            diff_info.path_to_ebuild.filename().string(),
                            fmt::format("{}", diff_details.old_ver.date),
                            fmt::format("{}", diff_details.new_ver.date));

                        std::filesystem::path const path_base
                            = where_to_change
                              / (diff_info.path_to_ebuild.parent_path()
                                     .parent_path()
                                     .filename())
                              / diff_info.path_to_ebuild.parent_path()
                                    .filename();

                        std::filesystem::path const old_path
                            = path_base / diff_info.path_to_ebuild.filename();
                        std::filesystem::path const new_path
                            = path_base / new_name;

                        LOG_TRACE_L1(
                            "Path base: '{}'\nOld path: '{}'\nNew path: "
                            "'{}'\nNew name: '{}'",
                            path_base, old_path, new_path, new_name);

                        TRY_OR_CO_RETURN_VOID_TRANSFORM_ERROR(
                            co_await gyou::string_to_file(
                                ioc, editted_ebuild_content, old_path),
                            [](boost::system::error_code errc)
                                { return errc.what(); });

                        // move the file to a file with new date. for now,
                        // just use std::filesystem::rename
                        {
                            std::error_code errc;
                            std::filesystem::rename(old_path, new_path, errc);
                            if (errc)
                                {
                                    co_return std::unexpected(fmt::format(
                                        "During applying changes, failed "
                                        "to "
                                        "rename a file from '{}' to "
                                        "'{}'. errc,message: '{}'",
                                        old_path, new_path, errc.message()));
                                };
                        }
                        LOG_INFO(
                            "Presumably successfully applied change to the "
                            "ebuild: {}",
                            diff_info.path_to_ebuild);
                        co_return {};
                    },
                [&](gyou::EditVerOrTag const& diff_details)
                    -> corral::Task<std::expected<void, std::string>>
                    {
                        LOG_TRACE_L2(
                            "{}\ns/{}/{}",
                            diff_info.path_to_ebuild.filename().string(),
                            diff_details.old_ver, diff_details.new_ver);

                        std::string const new_name = gyou::Replace(
                            diff_info.path_to_ebuild.filename().string(),
                            diff_details.old_ver, diff_details.new_ver);

                        std::filesystem::path const path_base
                            = where_to_change
                              / (diff_info.path_to_ebuild.parent_path()
                                     .parent_path()
                                     .filename())
                              / diff_info.path_to_ebuild.parent_path()
                                    .filename();

                        std::filesystem::path const old_path
                            = path_base / diff_info.path_to_ebuild.filename();
                        std::filesystem::path const new_path
                            = path_base / new_name;

                        LOG_TRACE_L1(
                            "Path base: '{}'\nOld path: '{}'\nNew path: "
                            "'{}'\nNew name: '{}'",
                            path_base, old_path, new_path, new_name);

                        // this probably should be recoded via async rename
                        // via liburing or whatever
                        {
                            std::error_code errc;
                            std::filesystem::rename(old_path, new_path, errc);
                            if (errc)
                                {
                                    co_return std::unexpected(fmt::format(
                                        "During applying changes, failed "
                                        "to "
                                        "rename a file from '{}' to "
                                        "'{}'. errc,message: '{}'",
                                        old_path, new_path, errc.message()));
                                };
                        }

                        LOG_INFO(
                            "Presumably applied change to the "
                            "ebuild: {}",
                            diff_info.path_to_ebuild);

                        TRY_OR_CO_RETURN_VOID(
                            co_await gyou::ebuild_manifest_update(ioc, cfg,
                                                                  new_path));

                        LOG_INFO(
                            "Presumably updated manifest for the ebuild: {}",
                            diff_info.path_to_ebuild);

                        co_return {};
                    }}));

        CORRAL_SUSPEND_FOREVER();
    }

}  // namespace gyou
