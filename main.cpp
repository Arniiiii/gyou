#include <array>
#include <chrono>
#include <expected>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include <CLI/CLI.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/file_base.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields_fwd.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/start_dir.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/url.hpp>
#include <corral/Nursery.h>
#include <corral/Semaphore.h>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>
#include <corral/run.h>
#include <corral/wait.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <glaze/glaze.hpp>
#include <inja/environment.hpp>
#include <inja/inja.hpp>
#include <inja/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <quill/core/LogLevel.h>
#include <quill/std/Chrono.h>
#include <quill/std/FilesystemPath.h>
#include <quill/std/Vector.h>
#include <re2/re2.h>
#include <re2/set.h>
#include <reflex/pcre2matcher.h>

#include "gyou/bash_ebuild.hpp"
#include "gyou/file_to_string.hpp"
#include "gyou/get_latest_updates.hpp"
#include "gyou/git_worktree.hpp"
#include "gyou/http_requests.hpp"
#include "gyou/parse_ebuild.hpp"
#include "gyou/parsing_groupsci.hpp"
#include "gyou/rss_parse_into_tree.hpp"
#include "gyou/string_to_file.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/change_related/pkg_type.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/consts.hpp"
#include "gyou/structs/ebuild_commit_specific.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"
#include "gyou/structs/return_code.hpp"
#include "gyou/structs/rss/most_cases.hpp"
#include "gyou/structs/service_enum.hpp"
#include "gyou/structs/service_regex.hpp"
#include "gyou/utils/array_utils.hpp"
#include "gyou/utils/boost_stacktrace_format.hpp"  // IWYU pragma: keep
#include "gyou/utils/omega_exception.hpp"
#include "gyou/utils/rusty_macros.hpp"
#include "gyou/utils/string_replace.hpp"
#include "gyou/utils/variants_utils.hpp"
#include "overwrite_log_macros.hpp"
#include "quill_static.hpp"

namespace
{

    [[nodiscard]] corral::Task<std::expected<int, std::string_view>>
    gh_create_pr(auto& ioc, gyou::Config const& cfg,
                 std::filesystem::path const& path_to_gh,
                 std::filesystem::path const& folder_path,
                 std::string const& branch_name)
    {
        // check if authorized in any account via `gh auth status` and regex
        // `Logged in to github.com account`

        // somehow check whether we need a pr

        // create pr
    }

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EditCommit
    {
        std::string env_var_name;
        gyou::CommitSpecific old_ver;
        gyou::CommitSpecific new_ver;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EditVerOrTag
    {
        std::string old_ver;
        std::string new_ver;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct InfoForDiff
    {
        std::filesystem::path path_to_ebuild;
        std::variant<EditCommit, EditVerOrTag> data_for_how_to_change;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct PackagesToUpdate
    {
        std::vector<InfoForDiff> what_to_change;
        bool is_any_successful = false;
        bool is_any_failed = false;
    };

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
            = TRY_OR_CO_RETURN(co_await get_latest_info(
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

    [[nodiscard]] corral::Task<PackagesToUpdate> get_what_to_change(
        auto& ioc, gyou::Config const& cfg, auto& semaphores,
        gyou::CommonContext& common_ctx)
    {
        PackagesToUpdate changes;
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
                                    // NOLINTBEGIN(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                                    nursery.start(
                                        [&](std::filesystem::directory_entry
                                                file_arg) mutable
                                            -> corral::Task<void>
                                            {
                                                auto sth
                                                    = co_await logic_per_ebuild(
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
                                    // NOLINTEND(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                                }
                        }
                }

            co_return corral::join;
        };
        co_return changes;
    }

    [[nodiscard]] corral::Task<std::expected<void, std::string>> apply_change(
        auto& ioc, std::filesystem::path const where_to_change,
        InfoForDiff const& diff_info)
    {
        LOG_INFO("Trying to apply a change for next ebuild: {}",
                 diff_info.path_to_ebuild);
        if (std::holds_alternative<EditCommit>(
                diff_info.data_for_how_to_change))
            {
                auto const& diff_details
                    = std::get<EditCommit>(diff_info.data_for_how_to_change);
                // change commit in ebuild

                std::string const editted_ebuild_content = __extension__({
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
                      / diff_info.path_to_ebuild.parent_path().filename();

                std::filesystem::path const old_path
                    = path_base / diff_info.path_to_ebuild.filename();
                std::filesystem::path const new_path = path_base / new_name;

                TRY_OR_CO_RETURN_VOID_TRANSFORM_ERROR(
                    co_await gyou::string_to_file(ioc, editted_ebuild_content,
                                                  old_path),
                    [](boost::system::error_code errc) { return errc.what(); });

                // move the file to a file with new date. for now, just use
                // std::filesystem::rename
                {
                    std::error_code errc;
                    std::filesystem::rename(old_path, new_path, errc);
                    if (errc)
                        {
                            co_return std::unexpected(fmt::format(
                                "During applying changes, failed to "
                                "rename a file from '{}' to "
                                "'{}'. errc,message: '{}'",
                                old_path, new_path, errc.message()));
                        };
                }
                LOG_INFO(
                    "Presumably successfully applied change to the ebuild: {}",
                    diff_info.path_to_ebuild);
                co_return {};
            }
        else if (std::holds_alternative<EditVerOrTag>(
                     diff_info.data_for_how_to_change))
            {
                // move the file to new ver or tag. for now, just use
                // std::filesystem::rename

                auto const& diff_details
                    = std::get<EditVerOrTag>(diff_info.data_for_how_to_change);

                std::string const new_name = gyou::Replace(
                    diff_info.path_to_ebuild.filename().string(),
                    diff_details.old_ver, diff_details.new_ver);

                std::filesystem::path const path_base
                    = where_to_change
                      / (diff_info.path_to_ebuild.parent_path()
                             .parent_path()
                             .filename())
                      / diff_info.path_to_ebuild.parent_path().filename();

                std::filesystem::path const old_path
                    = path_base / diff_info.path_to_ebuild.filename();
                std::filesystem::path const new_path = path_base / new_name;

                // this probably should be recoded via async rename via liburing
                // or whatever
                {
                    std::error_code errc;
                    std::filesystem::rename(old_path, new_path, errc);
                    if (errc)
                        {
                            co_return std::unexpected(fmt::format(
                                "During applying changes, failed to "
                                "rename a file from '{}' to "
                                "'{}'. errc,message: '{}'",
                                old_path, new_path, errc.message()));
                        };
                }

                LOG_INFO(
                    "Presumably successfully applied change to the ebuild: {}",
                    diff_info.path_to_ebuild);
                co_return {};
            }

        co_return std::unexpected(
            "Apparently, not all alternatives of InfoForDiffWereCovered");
    }

    [[nodiscard]] corral::Task<gyou::ReturnCode> chief_logic(
        auto& ioc, gyou::Config const& cfg, auto& semaphores,
        gyou::CommonContext& common_ctx)
    {
        PackagesToUpdate const changes
            = co_await get_what_to_change(ioc, cfg, semaphores, common_ctx);

        for (auto const& diff : changes.what_to_change)
            {
                if (std::holds_alternative<EditCommit>(
                        diff.data_for_how_to_change))
                    {
                        EditCommit commits_diff
                            = std::get<EditCommit>(diff.data_for_how_to_change);
                        LOG_INFO(
                            "Edit: path: '{}' old date: '{}' new date: '{}' "
                            "old commit: '{}' new commit: '{}'",
                            diff.path_to_ebuild, commits_diff.old_ver.date,
                            commits_diff.new_ver.date,
                            commits_diff.old_ver.commit,
                            commits_diff.new_ver.commit);
                    }
                else if (std::holds_alternative<EditVerOrTag>(
                             diff.data_for_how_to_change))
                    {
                        EditVerOrTag commits_diff = std::get<EditVerOrTag>(
                            diff.data_for_how_to_change);
                        LOG_INFO("Edit: path: '{}' old ver: '{}' new ver: '{}'",
                                 diff.path_to_ebuild, commits_diff.old_ver,
                                 commits_diff.new_ver);
                    }
            }

        std::filesystem::path const path_to_grouping
            = cfg.path_to_repo / "groups-ci.json";

        std::string const str_grouping = __extension__({
            auto res = co_await gyou::file_to_string(ioc, path_to_grouping);
            if (!res)
                {
                    LOG_ERROR(
                        "Got error during getting the groups-ci.json file : "
                        "'{}'",
                        std::move(res.error().message()));
                    co_return gyou::ReturnCode::FailedReadingGroupCiFile;
                };
            std::move(res.value());
        });

        gyou::GroupsCollection const groups = __extension__({
            auto res = gyou::parse_groups(str_grouping);
            if (!res)
                {
                    LOG_ERROR(
                        "Got error during parsing the groups-ci.json file : "
                        "'{}'",
                        std::move(res.error()));
                    co_return gyou::ReturnCode::FailedParsingGroupCiFile;
                }
            std::move(res.value());
        });

        std::unordered_multimap<size_t, size_t> const group_to_change
            = __extension__({
                  std::unordered_multimap<size_t, size_t> group_to_change_{};
                  group_to_change_.reserve(groups.amount_of_groups);

                  for (size_t change_index = 0;
                       change_index < std::size(changes.what_to_change);
                       ++change_index)
                      {
                          auto const& diff
                              = changes.what_to_change.at(change_index);
                          std::string const pv_to_look
                              = diff.path_to_ebuild.parent_path()
                                    .parent_path()
                                    .filename()
                                    .string()
                                + "/"
                                + diff.path_to_ebuild.parent_path()
                                      .filename()
                                      .string();
                          auto const it_str_to_group_index
                              = groups.groups.find(pv_to_look);
                          if (it_str_to_group_index != groups.groups.end())
                              {
                                  LOG_TRACE_L1(
                                      "Found match to a group: pkg_name: '{}' "
                                      "group index: {}",
                                      pv_to_look,
                                      it_str_to_group_index->second);
                                  group_to_change_.emplace(
                                      it_str_to_group_index->second,
                                      change_index);
                              }
                          else
                              {
                                  LOG_TRACE_L1(
                                      "Have not found match to a group: "
                                      "pkg_name: '{}'",
                                      pv_to_look);
                                  group_to_change_.emplace(0, change_index);
                              }
                      }
                  std::move(group_to_change_);
              });

        std::filesystem::path const temp_folder_worktrees
            = cfg.path_to_tmp / "worktrees";
        std::error_code errc_mkdir_p_worktrees;
        std::filesystem::create_directories(temp_folder_worktrees,
                                            errc_mkdir_p_worktrees);
        if (errc_mkdir_p_worktrees)
            {
                LOG_ERROR("Failed to create temp directory for worktrees: {}",
                          errc_mkdir_p_worktrees.message());
                co_return gyou::ReturnCode::FailedCreateDirTmpWorktrees;
            };
        LOG_TRACE_L2("Created folder for worktrees.");

        std::filesystem::path const path_to_git = __extension__({
            auto path_to_git_
                = boost::process::environment::find_executable("git");
            if (path_to_git_.empty())
                {
                    LOG_ERROR(
                        "Failed to find 'git' executable in your environment.");
                    co_return gyou::ReturnCode::FailedToFindGit;
                }
            path_to_git_;
        });

        std::filesystem::path const path_to_gh = __extension__({
            auto path_to_gh_
                = boost::process::environment::find_executable("gh");
            if (path_to_gh_.empty())
                {
                    LOG_ERROR(
                        "Failed to find 'gh' executable in your environment.");
                    co_return gyou::ReturnCode::FailedToFindGh;
                }
            path_to_gh_;
        });

        std::optional<gyou::ReturnCode> return_code;
        CORRAL_WITH_NURSERY(nursery)
        {
            // logic for 0, i.e. no grouping
            auto range_grp_to_change_idx_no_grouping
                = group_to_change.equal_range(0);
            for (auto it_grp_to_chg_idx
                 = range_grp_to_change_idx_no_grouping.first;
                 it_grp_to_chg_idx
                 != range_grp_to_change_idx_no_grouping.second;
                 ++it_grp_to_chg_idx)
                {
                    std::filesystem::path const path_to_ebuild
                        = changes.what_to_change.at(it_grp_to_chg_idx->second)
                              .path_to_ebuild;
                    std::string const base_name
                        = path_to_ebuild.parent_path()
                              .parent_path()
                              .filename()
                              .string()
                          + path_to_ebuild.parent_path().filename().string();
                    std::filesystem::path const folder_path
                        = temp_folder_worktrees / ("0_" + base_name);
                    std::string const branch_name = "ci_update/" + base_name;

                    // NOLINTBEGIN(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                    nursery.start(
                        [&, folder_path = folder_path,
                         branch_name = branch_name,
                         chg_idx
                         = it_grp_to_chg_idx->second]() -> corral::Task<void>
                            {
                                {
                                    auto res
                                        = co_await gyou::git_create_worktree(
                                            ioc, cfg, path_to_git, folder_path,
                                            branch_name);
                                    if (not res)
                                        {
                                            LOG_ERROR(
                                                "Failed to create a git "
                                                "worktree: "
                                                "{}",
                                                std::move(res.error()));
                                            return_code = gyou::ReturnCode::
                                                FailedMakingGitToMakeWorktrees;
                                            nursery.cancel();
                                            co_return;
                                        }
                                }
                                // make changes

                                {
                                    auto res = co_await apply_change(
                                        ioc, folder_path,
                                        changes.what_to_change.at(chg_idx));
                                    if (not res)
                                        {
                                            LOG_ERROR(
                                                "Failed to apply changes: "
                                                "{}",
                                                std::move(res.error()));
                                            return_code = gyou::ReturnCode::
                                                FailedApplyChange;
                                            nursery.cancel();
                                            co_return;
                                        }
                                }

                                // gh
                            });
                    // NOLINTEND(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                }

            // logic for groups

            for (size_t group_num = 1; group_num < groups.amount_of_groups + 1;
                 ++group_num)
                {
                    auto range_grp_to_change_idx
                        = group_to_change.equal_range(group_num);

                    std::filesystem::path const path_to_ebuild
                        = changes.what_to_change
                              .at(range_grp_to_change_idx.first->second)
                              .path_to_ebuild;
                    std::string const base_name
                        = path_to_ebuild.parent_path()
                              .parent_path()
                              .filename()
                              .string()
                          + path_to_ebuild.parent_path().filename().string();
                    std::filesystem::path const folder_path
                        = temp_folder_worktrees
                          / (fmt::format("{}_", group_num) + base_name);
                    std::string const branch_name = "ci_update/" + base_name;

                    // NOLINTBEGIN(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                    nursery.start(
                        [&, folder_path = folder_path,
                         branch_name = branch_name,
                         range_grp_to_change_idx
                         = range_grp_to_change_idx]() -> corral::Task<void>
                            {
                                {
                                    auto res = co_await git_create_worktree(
                                        ioc, cfg, path_to_git, folder_path,
                                        branch_name);
                                    if (not res)
                                        {
                                            LOG_ERROR(
                                                "Failed to create a git "
                                                "worktree: "
                                                "{}",
                                                std::move(res.error()));
                                            return_code = gyou::ReturnCode::
                                                FailedMakingGitToMakeWorktrees;
                                            nursery.cancel();
                                            co_return;
                                        }
                                }

                                for (auto it_grp_to_chg_idx
                                     = range_grp_to_change_idx.first;
                                     it_grp_to_chg_idx
                                     != range_grp_to_change_idx.second;
                                     ++it_grp_to_chg_idx)
                                    {
                                        nursery.start(
                                            [&, folder_path = folder_path,
                                             chg_idx
                                             = it_grp_to_chg_idx->second]()
                                                -> corral::Task<void>
                                                {
                                                    auto res
                                                        = co_await apply_change(
                                                            ioc, folder_path,
                                                            changes
                                                                .what_to_change
                                                                .at(chg_idx));
                                                    if (not res)
                                                        {
                                                            LOG_ERROR(
                                                                "Failed to "
                                                                "apply "
                                                                "changes: "
                                                                "{}",
                                                                std::move(
                                                                    res.error()));
                                                            return_code = gyou::
                                                                ReturnCode::
                                                                    FailedApplyChange;
                                                            nursery.cancel();
                                                            co_return;
                                                        }
                                                });
                                    }
                            });
                    // NOLINTEND(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                }

            co_return corral::join;
        };

        if (return_code.has_value())
            {
                co_return return_code.value();
            }

        if (changes.is_any_failed and changes.is_any_successful)
            {
                co_return gyou::ReturnCode::PartialSuccess;
            }
        else if (changes.is_any_failed and not changes.is_any_successful)
            {
                co_return gyou::ReturnCode::AllHaveFailed;
            }
        else if (not changes.is_any_successful)
            {
                co_return gyou::ReturnCode::NoEbuildFound;
            }
        else
            {
                co_return gyou::ReturnCode::Success;
                ;
            }
    }

    corral::Task<gyou::ReturnCode> actual_chief(auto& ioc,
                                                gyou::Config const& cfg)
    {
        constexpr size_t amount_of_services
            = magic_enum::enum_count<gyou::Service>();

        std::array<corral::Semaphore, amount_of_services>
            semaphores_per_services
            = gyou::make_array_from_factory<amount_of_services>(
                [concurrency_per_service = cfg.concurrency_per_service]()
                    { return corral::Semaphore(concurrency_per_service); });

        // to compile them all at once
        // NOLINTBEGIN(hicpp-signed-bitwise)
        std::string str_re_versions = reflex::PCRE2UTFMatcher::convert(
            R"(([\w][\w+-]*?)-((\d+)(\.\d+)*)([a-z]?)((_(pre|p|beta|alpha|rc)\d*)*)(-r(\d+))?)",
            reflex::convert_flag::unicode | reflex::convert_flag::notnewline);
        // NOLINTEND(hicpp-signed-bitwise)

        const reflex::PCRE2UTFMatcher::Pattern& pattern_re_versions(
            str_re_versions);

        gyou::CommonContext common_ctx{
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
                        return std::move(re_set_services);
                    }),
            .re_version_matcher = reflex::PCRE2UTFMatcher(pattern_re_versions),
        };

        gyou::ReturnCode res = co_await chief_logic(
            ioc, cfg, semaphores_per_services, common_ctx);
        co_return res;
    }

}  // namespace

int main(int argc, char* argv[])
{
    gyou::Config cfg;

    // gyou::Config parsing
    {
        CLI::App app{
            "Post-processor for YouTube's RSS feed, so that you get "
            "summary of video inside the feed via sending an HTTP "
            "request to something like an Ollama instance."};

        // Temporary storage for CLI11 to map types it doesn't handle
        // natively without custom validators
        std::vector<std::string> headers_raw
            = {"Content-Type: application/json"};
        std::string log_level_str = "info";

        app.add_option("-H,--header", headers_raw,
                       "HTTP headers for request to an ?Ollama? instance.")
            ->take_all();

        app.add_option("-l,--log-file", cfg.log_file,
                       "Filepath to internal logs")
            ->default_val("./logs.log");

        app.add_option("-R,--repo-path", cfg.path_to_repo, "Filepath to repo")
            ->required();

        app.add_option("-t,--tmp-path", cfg.path_to_tmp,
                       "Filepath to portage's tmp")
            ->required();

        app.add_option("-G,--gentoo-repo-path", cfg.path_to_gentoo_repo,
                       "Filepath to gentoo's repo path")
            ->default_val("/var/db/repos/gentoo/");

        app.add_option("-P,--portage-bin-path", cfg.path_to_portage_bin,
                       "Filepath to portage's bin folder i.e. `git clone "
                       "https://github.com/Arniiiii/portage` and give the path "
                       "to `./bin/` folder")
            ->required();

        app.add_option("-Y,--portage-pym-path", cfg.path_to_portage_pym,
                       "Filepath to portage's PYM folder. I have no idea what "
                       "this is, but it can be any empty folder. ")
            ->required();

        app.add_option("--log-level", log_level_str,
                       "Log level: "
                       "tracel3,tracel2,tracel1,debug,info,notice,warning,"
                       "error,critical")
            ->default_val("info");
        app.add_option("--main-branch-name", cfg.main_branch_name,
                       "Main branch name of the repo ")
            ->default_val("master");

        app.add_option("-J,--jobs-requests", cfg.concurrency_per_service,
                       "Amount of concurrent request per service"
                       "by this application")
            ->check(CLI::PositiveNumber)
            ->default_val(gyou::DEFAULT_MAX_CONCURRENT_REQUESTS_PER_SERVICE);
        try
            {
                app.parse(argc, argv);

                auto const path_to_ebuild_sh
                    = cfg.path_to_portage_bin / "ebuild.sh";
                if (not std::filesystem::exists(path_to_ebuild_sh))
                    {
                        throw OmegaException<std::filesystem::path>(
                            "An expected executable does not exists.",
                            path_to_ebuild_sh);
                    }

                cfg.log_level = quill::loglevel_from_string(log_level_str);

                for (const auto& header_raw_str : headers_raw)
                    {
                        std::vector<std::string> parts;
                        boost::split(parts, header_raw_str,
                                     boost::is_any_of(":"));
                        if (parts.size() >= 2)
                            {
                                std::string key = boost::trim_copy(parts[0]);
                                std::string val = boost::trim_copy(parts[1]);
                                cfg.headers.insert(key, val);
                            }
                    }
            }
        catch (const CLI::ParseError& e)
            {
                return app.exit(e);
            }
        catch (OmegaException<std::filesystem::path>& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                std::string log = fmt::format(
                    R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
This is an Omega exception. Most definitely an incorrect value was specified in command line args, specifically a file path to something. 

Here's .what():
{}

Here's data:
{}

Here's trace:
{}

Here's line where something failed:
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace:
{}
)",
                    e.what(), e.data(), e.stack(), e.where(),
                    boost::stacktrace::to_string(trace));

                fmt::print(std::cerr, "{}", log);
                return std::to_underlying(
                    gyou::ReturnCode::FailSpecifiedValueIsIncorrect);
            }
        catch (OmegaException<std::string>& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                std::string log = fmt::format(
                    R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
This is an Omega exception. Most definitely an incorrect value was specified in command line args. 

Here's .what():
{}

Here's data:
{}

Here's trace:
{}

Here's line where something failed:
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace:
{}
)",
                    e.what(), e.data(), e.stack(), e.where(),
                    boost::stacktrace::to_string(trace));

                fmt::print(std::cerr, "{}", log);
                return std::to_underlying(
                    gyou::ReturnCode::FailSpecifiedValueIsIncorrect);
            }
        catch (std::exception& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                fmt::print(
                    std::cerr,
                    R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
You know that standard exceptions sucks. 
Also, this exception is from the part of parsing command line args. Most definitely an incorrect value was specified in args.
                
Here's .what():
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace... Hope it works or kill youself debugging this shit:
{}
)",
                    e.what(), boost::stacktrace::to_string(trace));
                return std::to_underlying(
                    gyou::ReturnCode::FailDuringInitializationConfig);
            }
    }

    // Enabling logging
    try
        {
            setup_quill(cfg.log_file, cfg.log_level);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            fmt::print(
                std::cerr,
                R"(Oohh, look at you, who got an exception, my cutie lovely guy.  
You know that standard exceptions sucks. 
Also, this exception appeared during setting up logging.

Here's .what():
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace... Hope it works or kill youself debugging this shit:
{}
)",
                e.what(), boost::stacktrace::to_string(trace));
            return std::to_underlying(
                gyou::ReturnCode::FailInitializationLogger);
        }

    // Finally main.
    try
        {
            boost::asio::io_context ioc;
            LOG_DEBUG("Entering asynchronous main...");
            boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
            auto&& [main_opt, signals_opt] = corral::run(
                ioc, corral::anyOf(actual_chief(ioc, cfg),
                                   signals.async_wait(corral::asio_awaitable)));
            if (main_opt)
                {
                    return std::to_underlying(main_opt.value());
                }
            return std::to_underlying(
                gyou::ReturnCode::ReceivedCancellationSignal);
        }
    catch (OmegaException<std::filesystem::path>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
This is an Omega exception. Maybe from checking whether a file exists.

Here's .what():
{}

Here's data: 
{}

Here's trace:
{}

Here's line where something failed:
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace: 
{}
)",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(gyou::ReturnCode::FailParsePromptResult);
        }
    catch (OmegaException<std::string>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
This is a generic Omega exception.

Here's .what():
{}

Here's data:
{}

Here's trace:
{}

Here's line where something failed:
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace:
{}
)",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(gyou::ReturnCode::FailParsePromptResult);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                R"(Oohh, look at you, who got an exception, my cutie lovely guy. 
You know that standard exceptions sucks. 

Here's .what():
{}

Here's an attempt to get backtrace of it via boost::stacktrace and libbacktrace... Hope it works or kill youself debugging this shit:
{}
)",
                e.what(), boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(gyou::ReturnCode::FailStandardException);
        }
}
