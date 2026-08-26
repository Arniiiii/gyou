#include "gyou/bash_ebuild_manifest.hpp"

#include <filesystem>
#include <vector>

#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/wait.h>
#include <fmt/format.h>
#include <quill/std/Vector.h>

#include "gyou/utils/boost_process_fmt.hpp"  // IWYU pragma: keep
#include "overwrite_log_macros.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>>
    ebuild_manifest_update(boost::asio::io_context& ioc,
                           gyou::Config const& cfg,
                           std::filesystem::path const& path_to_ebuild_file)
    {
        std::string pkg_full_name
            = path_to_ebuild_file.filename().stem().string();
        std::string pkg_name
            = path_to_ebuild_file.parent_path().filename().string();
        std::string category = path_to_ebuild_file.parent_path()
                                   .parent_path()
                                   .filename()
                                   .string();

        std::filesystem::path temp_folder
            = (cfg.path_to_tmp / category / pkg_full_name).concat("/");

        std::error_code errc_mkdir_p;
        std::filesystem::create_directories(temp_folder, errc_mkdir_p);
        if (errc_mkdir_p)
            {
                co_return std::unexpected(fmt::format(
                    "Failed to create temp directory for a ebuild: {}",
                    errc_mkdir_p.message()));
            };

        std::filesystem::path distfiles_folder = cfg.path_to_tmp / "distfiles";

        std::filesystem::create_directories(distfiles_folder, errc_mkdir_p);
        if (errc_mkdir_p)
            {
                co_return std::unexpected(fmt::format(
                    "Failed to create temp directory for a ebuild: {}",
                    errc_mkdir_p.message()));
            };

        boost::process::v2::filesystem::path bzip2_path_boosty
            = boost::process::environment::find_executable("bzip2");

        auto cur_env = boost::process::environment::current();

        std::vector<boost::process::environment::key_value_pair> env_for_ebuild{
            cur_env.begin(), cur_env.end()};

        env_for_ebuild.emplace_back(
            fmt::format("{}={}", "EBUILD", path_to_ebuild_file.string()));
        env_for_ebuild.emplace_back(
            fmt::format("{}={}", "T", temp_folder.string()));
        env_for_ebuild.emplace_back(
            fmt::format("{}={}", "DISTDIR", distfiles_folder.string()));
        env_for_ebuild.emplace_back(fmt::format(
            "{}={}", "PORTAGE_BIN_PATH", cfg.path_to_portage_bin.string()));
        env_for_ebuild.emplace_back(fmt::format(
            "{}={}", "PORTAGE_PYM_PATH", cfg.path_to_portage_pym.string()));
        env_for_ebuild.emplace_back(
            fmt::format("{}={}", "PORTAGE_ECLASS_LOCATIONS_STR",
                        cfg.path_to_gentoo_repo.string() + ":"
                            + path_to_ebuild_file.parent_path()
                                  .parent_path()
                                  .parent_path()
                                  .string()));
        env_for_ebuild.emplace_back(fmt::format(
            "{}={}", "PORTAGE_BZIP2_COMMAND", bzip2_path_boosty.string()));
        ;

        LOG_TRACE_L1("env: {}", env_for_ebuild);

        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        LOG_TRACE_L1("Presumably running next command: '{}'",
                     (cfg.path_to_portage_bin / "ebuild").string() + " "
                         + path_to_ebuild_file.string() + " manifest");

        auto const path_to_ebuild_py_exe = cfg.path_to_portage_bin / "ebuild";

        auto proc = boost::process::process(
            ioc, path_to_ebuild_py_exe.string(),
            {path_to_ebuild_file.string(), "manifest"},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_environment{env_for_ebuild});

        LOG_DEBUG("Doing sth in python, probably");

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
                co_return std::unexpected(fmt::format(
                    "Failed to do ebuild: ec: {}\nstderr: {}\nstdout: {}",
                    errc_proc, stderr_s, stdout_s));
            }

        co_return {};
    }
}  // namespace gyou
