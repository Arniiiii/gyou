#include "gyou/git_worktree.hpp"

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
#include <fmt/std.h>
#include <quill/std/FilesystemPath.h>

#include "gyou/async_read_with_custom_log.hpp"
#include "gyou/structs/config.hpp"
#include "quill_usage/overwrite_log_macros.hpp"

namespace gyou
{
    [[nodiscard]] corral::Task<std::expected<void, std::string>>
    git_create_worktree(boost::asio::io_context& ioc, gyou::Config const& cfg,
                        std::filesystem::path const& path_to_git,
                        std::filesystem::path const folder_path,
                        std::string const branch_name)
    {
        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        std::string const exe_representation
            = fmt::format("{} worktree add -b {} {} {}", path_to_git,
                          branch_name, folder_path, cfg.main_branch_name);

        LOG_DEBUG("Presumably running next command: '{}'", exe_representation);

        auto proc = boost::process::process(
            ioc, path_to_git.string(),
            {"worktree", "add", "-b", branch_name, folder_path.string(),
             cfg.main_branch_name},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_start_dir(cfg.path_to_repo.string()));

        LOG_DEBUG("Waiting until git does it job, probably");

        auto [proc_tuple, stdout_s, stderr_s] = co_await corral::allOf(
            proc.async_wait(corral::asio_nothrow_awaitable),
            gyou::read_loop("git_worktree_out", rp_stdout),
            gyou::read_loop("git_worktree_err", rp_stderr));
        auto&& [_, errc_proc] = proc_tuple;

        LOG_TRACE_L2("`{}`\nstdout ``:\n{}\n\nstderr:\n{}", exe_representation,
                     stdout_s, stderr_s);
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
