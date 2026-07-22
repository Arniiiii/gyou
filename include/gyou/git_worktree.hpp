#ifndef INCLUDE_GYOU_GIT_WORKTREE_HPP_
#define INCLUDE_GYOU_GIT_WORKTREE_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/start_dir.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <fmt/format.h>

#include "gyou/structs/config.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{
    [[nodiscard]] corral::Task<std::expected<void, std::string>>
    git_create_worktree(auto& ioc, gyou::Config const& cfg,
                        std::filesystem::path const& path_to_git,
                        std::filesystem::path const folder_path,
                        std::string const branch_name)
    {
        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        LOG_DEBUG("Presumably running next command: '{}'",
                  fmt::format("{} worktree add -b {} {} {}", path_to_git,
                              branch_name, folder_path, cfg.main_branch_name));

        auto proc = boost::process::process(
            ioc, path_to_git.string(),
            {"worktree", "add", "-b", branch_name, folder_path.string(),
             cfg.main_branch_name},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_start_dir(cfg.path_to_repo.string()));

        LOG_DEBUG("Waiting until git does it job, probably");

        std::string stdout_s;
        std::string stderr_s;

        auto [proc_tuple, _, _] = co_await corral::allOf(
            proc.async_wait(corral::asio_nothrow_awaitable),
            boost::asio::async_read(rp_stdout,
                                    boost::asio::dynamic_buffer(stdout_s),
                                    corral::asio_nothrow_awaitable),
            boost::asio::async_read(rp_stderr,
                                    boost::asio::dynamic_buffer(stderr_s),
                                    corral::asio_nothrow_awaitable));
        auto&& [_, errc_proc] = proc_tuple;

        if (errc_proc != 0)
            {
                co_return std::unexpected(
                    fmt::format("Failed to create a worktree: ec: {}\nstderr: "
                                "{}\nstdout: {}",
                                errc_proc, stderr_s, stdout_s));
            }

        LOG_DEBUG("Presumably finished OK for next worktree path: {}",
                  folder_path);
        co_return {};
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_GIT_WORKTREE_HPP_
