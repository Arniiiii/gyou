#include "gyou/git_fetch.hpp"

#include <filesystem>
#include <vector>

#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/wait.h>
#include <fmt/format.h>
#include <quill/std/Vector.h>

#include "gyou/async_read_with_custom_log.hpp"
#include "gyou/utils/boost_process_fmt.hpp"  // IWYU pragma: keep
#include "quill_usage/overwrite_log_macros.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>> git_fetch(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        boost::process::v2::filesystem::path const& git_exe_path)
    {
        auto cur_env = boost::process::environment::current();

        std::vector<boost::process::environment::key_value_pair> env_for_ebuild{
            cur_env.begin(), cur_env.end()};

        LOG_TRACE_L1("env: {}", env_for_ebuild);

        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        LOG_TRACE_L1("Presumably running next command: '{}'",
                     (git_exe_path).string() + " fetch --all --prune");

        auto proc = boost::process::process(
            ioc, git_exe_path, {"fetch", "--all", "--prune"},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_environment{env_for_ebuild});

        LOG_DEBUG("Doing `git fetch --all --prune`, probably");

        auto [stdout_s, stderr_s] = co_await corral::allOf(
            gyou::read_loop("git_fetch_out", rp_stdout),
            gyou::read_loop("git_fetch_err", rp_stderr));
        auto&& [_, status_code_proc]
            = co_await proc.async_wait(corral::asio_nothrow_awaitable);
        ;

        LOG_TRACE_L2("stdout `git fetch --all`:\n{}\n\nstderr:\n{}", stdout_s,
                     stderr_s);

        if (status_code_proc != 0)
            {
                co_return std::unexpected(fmt::format(
                    "Failed to do ebuild: ec: {}\nstderr: {}\nstdout: {}",
                    status_code_proc, stderr_s, stdout_s));
            }

        co_return {};
    }
}  // namespace gyou
