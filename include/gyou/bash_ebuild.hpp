#ifndef INCLUDE_GYOU_BASH_EBUILD_HPP_
#define INCLUDE_GYOU_BASH_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <fmt/format.h>

#include "gyou/structs/config.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<
        std::expected<std::filesystem::path, std::string>>
    bash_ebuild(auto& ioc, gyou::Config const& cfg,
                std::filesystem::path const& path_to_ebuild,
                std::string_view const pv)
    {
        std::string pkg_full_name = path_to_ebuild.filename().stem().string();
        std::string pkg_name = path_to_ebuild.parent_path().filename().string();
        std::string category
            = path_to_ebuild.parent_path().parent_path().filename().string();

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

        std::unordered_map<boost::process::environment::key,
                           boost::process::environment::value>
            env_for_ebuild = {
                {"EBUILD", path_to_ebuild.string()},
                {"T", temp_folder.string()},
                {"PORTAGE_BIN_PATH", cfg.path_to_portage_bin.string()},
                {"PORTAGE_PYM_PATH", cfg.path_to_portage_pym.string()},
                {"PORTAGE_ECLASS_LOCATIONS_STR",
                 cfg.path_to_gentoo_repo.string() + ":"
                     + cfg.path_to_repo.string()},
                {"EBUILD_PHASE", "_internal_test"},
                {"CATEGORY", category},
                {"PF", pkg_full_name},
                {"PN", pkg_name},
                {"PV", pv},
            };

        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        LOG_DEBUG("Presumably running next command: '{}'",
                  (cfg.path_to_portage_bin / "ebuild.sh").string() + " "
                      + "_internal_test" + " " + path_to_ebuild.string());

        auto const path_to_ebuild_sh = cfg.path_to_portage_bin / "ebuild.sh";

        auto proc = boost::process::process(
            ioc, path_to_ebuild_sh.string(),
            {"_internal_test", path_to_ebuild.string()},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_environment{env_for_ebuild});

        LOG_DEBUG("Doing sth in bash, probably");

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

        co_return temp_folder;
    }
}  // namespace gyou

#endif  // INCLUDE_GYOU_BASH_EBUILD_HPP_
