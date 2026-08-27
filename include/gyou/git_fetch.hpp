#ifndef INCLUDE_GYOU_GIT_FETCH_HPP_
#define INCLUDE_GYOU_GIT_FETCH_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/process/v2/process.hpp>
#include <corral/Task.h>

#include "gyou/structs/config.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>> git_fetch(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        boost::process::v2::filesystem::path const& git_exe_path);

}

#endif  // INCLUDE_GYOU_GIT_FETCH_HPP_
