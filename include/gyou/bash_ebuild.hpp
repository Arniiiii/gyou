#ifndef INCLUDE_GYOU_BASH_EBUILD_HPP_
#define INCLUDE_GYOU_BASH_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/structs/config.hpp"
#include "gyou/structs/portage_ver.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<
        std::expected<std::filesystem::path, std::string>>
    bash_ebuild_generate_environment_file(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        std::filesystem::path const& path_to_ebuild,
        gyou::ParsedEbuildStem const& ebuild_name_parts);
}  // namespace gyou

#endif  // INCLUDE_GYOU_BASH_EBUILD_HPP_
