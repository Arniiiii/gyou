#include <array>
#include <expected>
#include <filesystem>
#include <optional>
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
#include <corral/utility.h>
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

#include "gyou/apply_change.hpp"
#include "gyou/file_to_string.hpp"
#include "gyou/get_what_to_change.hpp"
#include "gyou/git_worktree.hpp"
#include "gyou/parsing_groupsci.hpp"
#include "gyou/string_to_file.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/consts.hpp"
#include "gyou/structs/result_of_parsing.hpp"
#include "gyou/structs/return_code.hpp"
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

    [[nodiscard]] corral::Task<gyou::ReturnCode> chief_logic(
        auto& ioc, gyou::Config const& cfg, auto& semaphores,
        gyou::CommonContext& common_ctx)
    {
        gyou::PackagesToUpdate const changes
            = co_await gyou::get_what_to_change(ioc, cfg, semaphores,
                                                common_ctx);

        for (auto const& diff : changes.what_to_change)
            {
                if (std::holds_alternative<gyou::EditCommit>(
                        diff.data_for_how_to_change))
                    {
                        gyou::EditCommit commits_diff
                            = std::get<gyou::EditCommit>(
                                diff.data_for_how_to_change);
                        LOG_INFO(
                            "Edit: path: '{}' old date: '{}' new date: '{}' "
                            "old commit: '{}' new commit: '{}'",
                            diff.path_to_ebuild, commits_diff.old_ver.date,
                            commits_diff.new_ver.date,
                            commits_diff.old_ver.commit,
                            commits_diff.new_ver.commit);
                    }
                else if (std::holds_alternative<gyou::EditVerOrTag>(
                             diff.data_for_how_to_change))
                    {
                        gyou::EditVerOrTag commits_diff
                            = std::get<gyou::EditVerOrTag>(
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
                                    auto res = co_await gyou::apply_change(
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
